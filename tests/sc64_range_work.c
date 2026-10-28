#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

typedef uint8_t u8;
#include "../starcraft/sc64_range_work.h"

#define MAX_N (128 * 128)
#define GRID_BYTES ((MAX_N + 7) / 8)
#define FIELD_BYTES ((MAX_N * 3 + 7) / 8)
#define SCRATCH_BYTES (2 * GRID_BYTES)

static u8 walk_store[GRID_BYTES + 2];
static u8 gradient_store[FIELD_BYTES + 2];
static u8 frontier_store[SCRATCH_BYTES + 2];
static unsigned char seen[MAX_N];
static unsigned short distance_map[MAX_N];
static unsigned short queue[MAX_N];

static void put_walk(int c, int value)
{
    unsigned bit = (unsigned)c;
    u8 *p = walk_store + 1;
    if (value) p[bit >> 3] |= (u8)(1u << (bit & 7u));
    else p[bit >> 3] &= (u8)~(1u << (bit & 7u));
}

static int get_walk(int c)
{
    const u8 *p = walk_store + 1;
    return (p[(unsigned)c >> 3] >> ((unsigned)c & 7u)) & 1u;
}

static unsigned oracle(int cols, int rows, int tx, int ty, int lo, int hi)
{
    int n = cols * rows, head = 0, tail = 0;
    memset(seen, 0, (size_t)n);
    for (int c = 0; c < n; ++c) {
        int col = c % cols, row = c / cols;
        int dx = col * 16 + 8 - tx, dy = row * 16 + 8 - ty;
        int d2 = dx * dx + dy * dy;
        distance_map[c] = 0xffffu;
        if (get_walk(c) && d2 >= lo * lo && d2 <= hi * hi) {
            seen[c] = 1;
            distance_map[c] = 0;
            queue[tail++] = (unsigned short)c;
        }
    }
    while (head < tail) {
        int c = queue[head++], col = c % cols, row = c / cols;
        int next[4] = { col ? c - 1 : -1, col + 1 < cols ? c + 1 : -1,
                        row ? c - cols : -1, row + 1 < rows ? c + cols : -1 };
        for (int i = 0; i < 4; ++i) if (next[i] >= 0 && get_walk(next[i]) && !seen[next[i]]) {
            seen[next[i]] = 1;
            distance_map[next[i]] = (unsigned short)(distance_map[c] + 1u);
            queue[tail++] = (unsigned short)next[i];
        }
    }
    return (unsigned)tail;
}

static void prepare(int cols, int rows, unsigned char walk_value,
                    unsigned char gradient_value, unsigned char frontier_value)
{
    memset(walk_store, 0xa7, sizeof(walk_store));
    memset(walk_store + 1, 0, sizeof(walk_store) - 2);
    memset(gradient_store, gradient_value, sizeof(gradient_store));
    memset(frontier_store, frontier_value, sizeof(frontier_store));
    for (int c = 0; c < cols * rows; ++c) put_walk(c, walk_value != 0);
}

static unsigned finish(Sc64RangeWork *work, unsigned budget, unsigned bound)
{
    unsigned total = 0;
    for (unsigned calls = 0; calls < bound + 100u; ++calls) {
        unsigned used = 99;
        int status = sc64_range_work_pump(work, budget, &used);
        assert(used <= budget);
        total += used;
        assert(total <= bound);
        if (status == SC64_RANGE_WORK_READY) return total;
        assert(status == SC64_RANGE_WORK_PENDING);
    }
    assert(!"worker did not finish within finite pump bound");
    return total;
}

static void check_map(int cols, int rows, int tx, int ty, int lo, int hi)
{
    Sc64RangeWork work;
    unsigned n = (unsigned)(cols * rows);
    unsigned grid_bytes = (n + 7u) / 8u;
    unsigned field_bytes = (n * 3u + 7u) / 8u;
    unsigned scratch_bytes = 2u * grid_bytes;
    unsigned queue_count;
    unsigned bound = field_bytes + scratch_bytes + n + n * grid_bytes;
    prepare(cols, rows, 0, 0x00, 0xff);
    queue_count = oracle(cols, rows, tx, ty, lo, hi);
    assert(sc64_range_work_begin(&work, cols, rows,
               walk_store + 1, grid_bytes,
               gradient_store + 1, field_bytes,
               frontier_store + 1, scratch_bytes,
               tx, ty, lo, hi) == SC64_RANGE_WORK_PENDING);
    /* Poisoned gradient/frontier cannot become visible before clear+seed. */
    {
        int nx = -10, ny = -10;
        unsigned mask = 0xdeadbeefu, used = 0;
        assert(sc64_range_work_query(&work, 0, 0, &nx, &ny, &mask) ==
               SC64_RANGE_WORK_PENDING);
        assert(sc64_range_work_pump(&work, 0, &used) == SC64_RANGE_WORK_PENDING);
        assert(used == 0);
        assert(sc64_range_work_query(&work, 0, 0, &nx, &ny, &mask) ==
               SC64_RANGE_WORK_PENDING);
    }
    /* Exercise quota=1 and larger interleaved slices. */
    finish(&work, 1, bound);
    assert(work.phase == SC64_RANGE_WORK_DONE);
    assert(walk_store[0] == 0xa7 && walk_store[sizeof(walk_store)-1] == 0xa7);
    assert(gradient_store[0] == 0x00 && gradient_store[sizeof(gradient_store)-1] == 0x00);
    assert(frontier_store[0] == 0xff && frontier_store[sizeof(frontier_store)-1] == 0xff);
    for (int c = 0; c < cols * rows; ++c) {
        int col = c % cols, row = c / cols, nx = -1, ny = -1;
        unsigned mask = 0;
        int status = sc64_range_work_query(&work, col, row, &nx, &ny, &mask);
        if (get_walk(c) && seen[c]) {
            assert(status == SC64_RANGE_WORK_READY);
            if (distance_map[c] == 0) {
                assert(nx == col && ny == row);
                {
                    int dx = nx * 16 + 8 - tx, dy = ny * 16 + 8 - ty;
                    int d2 = dx * dx + dy * dy;
                    assert(d2 >= lo * lo && d2 <= hi * hi);
                }
            } else {
                int nc = nx + ny * cols;
                assert(nc >= 0 && nc < cols * rows && get_walk(nc));
                assert(distance_map[nc] + 1u == distance_map[c]);
            }
        } else if (get_walk(c)) {
            assert(status == SC64_RANGE_WORK_NO_PATH);
        } else {
            int exits = (col > 0 && seen[c-1] && get_walk(c-1)) ||
                (col+1 < cols && seen[c+1] && get_walk(c+1)) ||
                (row > 0 && seen[c-cols] && get_walk(c-cols)) ||
                (row+1 < rows && seen[c+cols] && get_walk(c+cols));
            if (exits) assert(status == SC64_RANGE_WORK_READY && mask != 0);
            else assert(status == SC64_RANGE_WORK_NO_PATH);
        }
    }
    assert(queue_count <= n);
}

static unsigned random_state = 0x19c3a5u;
static unsigned random_u32(void)
{
    random_state ^= random_state << 13;
    random_state ^= random_state >> 17;
    random_state ^= random_state << 5;
    return random_state;
}

static void test_random_maps(void)
{
    for (int trial = 0; trial < 140; ++trial) {
        int cols = 3 + (int)(random_u32() % 10u);
        int rows = 3 + (int)(random_u32() % 9u);
        int tx = (int)(random_u32() % (unsigned)(cols * 16));
        int ty = (int)(random_u32() % (unsigned)(rows * 16));
        int lo = (int)(random_u32() % 48u);
        int hi = lo + (int)(random_u32() % 80u);
        prepare(cols, rows, 0, 0, 0);
        for (int c = 0; c < cols * rows; ++c)
            put_walk(c, (random_u32() % 100u) >= 28u);
        /* Keep the same prepared byte grid for the independent oracle. */
        {
            Sc64RangeWork work;
            unsigned n = (unsigned)(cols * rows), qb=(n+7u)/8u;
            unsigned gb=(n*3u+7u)/8u, sb=2u*qb;
            unsigned bound=gb+sb+n+n*qb;
            unsigned total=0, calls=0;
            oracle(cols,rows,tx,ty,lo,hi);
            assert(sc64_range_work_begin(&work,cols,rows,walk_store+1,qb,
                gradient_store+1,gb,frontier_store+1,sb,tx,ty,lo,hi)==0);
            while(work.phase!=SC64_RANGE_WORK_DONE){
                unsigned used=0, budget=1u+(random_u32()%13u);
                int status=sc64_range_work_pump(&work,budget,&used);
                assert(used<=budget); total+=used; calls++;
                assert(total<=bound && calls<=bound+1u);
                assert(status==SC64_RANGE_WORK_PENDING || status==SC64_RANGE_WORK_READY);
            }
            for(int c=0;c<(int)n;c++){
                int nx=-1,ny=-1;
                int status=sc64_range_work_query(&work,c%cols,c/cols,&nx,&ny,0);
                if(get_walk(c)&&seen[c]){
                    assert(status==SC64_RANGE_WORK_READY);
                    if(distance_map[c]){
                        int nc=nx+ny*cols;
                        assert(nc>=0&&nc<(int)n&&distance_map[nc]+1u==distance_map[c]);
                    } else {
                        int dx=(c%cols)*16+8-tx,dy=(c/cols)*16+8-ty,d2=dx*dx+dy*dy;
                        assert(nx==c%cols&&ny==c/cols&&d2>=lo*lo&&d2<=hi*hi);
                    }
                }else if(get_walk(c))assert(status==SC64_RANGE_WORK_NO_PATH);
            }
        }
    }
}

static void test_partial_and_blocked(void)
{
    Sc64RangeWork work;
    int nx,ny;
    unsigned mask,used;
    prepare(16,3,1,0x55,0xaa);
    put_walk(13 + 1*16,0);
    assert(sc64_range_work_begin(&work,16,3,walk_store+1,6,
        gradient_store+1,18,frontier_store+1,12,248,24,0,0)==0);
    /* Reach seed phase, then expose a near ready origin while far remains pending. */
    while(work.phase!=SC64_RANGE_WORK_FLOOD)
        assert(sc64_range_work_pump(&work,64,&used)>=SC64_RANGE_WORK_PENDING);
    assert(sc64_range_work_query(&work,0,1,&nx,&ny,&mask)==SC64_RANGE_WORK_PENDING);
    for(int i=0;i<30;i++){
        assert(sc64_range_work_pump(&work,1,&used)>=SC64_RANGE_WORK_PENDING);
        if(sc64_range_work_query(&work,14,1,&nx,&ny,&mask)==SC64_RANGE_WORK_READY)break;
    }
    assert(sc64_range_work_query(&work,14,1,&nx,&ny,&mask)==SC64_RANGE_WORK_READY);
    assert(sc64_range_work_query(&work,0,1,&nx,&ny,&mask)==SC64_RANGE_WORK_PENDING);
    finish(&work,16,18+12+48+48*6);
    assert(sc64_range_work_query(&work,0,1,&nx,&ny,&mask)==SC64_RANGE_WORK_READY);
    /* Blocked origin adjacent to a known reachable tile gets a bounded mask. */
    assert(sc64_range_work_query(&work,13,1,&nx,&ny,&mask)==SC64_RANGE_WORK_READY);
    assert(mask & (1u << 0)); /* east exit */
}

static void test_disconnected_no_band(void)
{
    Sc64RangeWork work;
    int nx,ny;
    unsigned mask;
    prepare(7,5,0,0xcc,0x33);
    for(int y=0;y<5;y++)for(int x=0;x<7;x++)
        put_walk(x+y*7,(x<3 || x>3) && !(x==3));
    /* Place a wall between two walkable components; seed only right side. */
    assert(sc64_range_work_begin(&work,7,5,walk_store+1,5,
       gradient_store+1,14,frontier_store+1,10,6*16+8,2*16+8,0,0)==0);
    finish(&work,3,14+10+35+35*5);
    assert(sc64_range_work_query(&work,0,2,&nx,&ny,&mask)==SC64_RANGE_WORK_NO_PATH);
    assert(sc64_range_work_query(&work,6,2,&nx,&ny,&mask)==SC64_RANGE_WORK_READY);
    /* Empty annulus yields immediate finite no-path after seed scan. */
    assert(sc64_range_work_begin(&work,7,5,walk_store+1,5,
       gradient_store+1,14,frontier_store+1,10,3*16+8,2*16+8,500,600)==0);
    finish(&work,11,14+10+35+35*5);
    assert(sc64_range_work_query(&work,6,2,&nx,&ny,&mask)==SC64_RANGE_WORK_NO_PATH);
}

static void test_reset_and_bounds(void)
{
    Sc64RangeWork work;
    unsigned used;
    int nx,ny;
    unsigned mask;
    prepare(5,5,1,0x11,0x22);
    assert(sc64_range_work_begin(&work,5,5,walk_store+1,4,
       gradient_store+1,10,frontier_store+1,8,40,40,0,20)==0);
    assert(sc64_range_work_pump(&work,2,&used)==0 && used==2);
    sc64_range_work_reset(&work);
    assert(sc64_range_work_query(&work,0,0,&nx,&ny,&mask)==SC64_RANGE_WORK_ERROR);
    assert(sc64_range_work_pump(&work,3,&used)==SC64_RANGE_WORK_ERROR && used==0);
    assert(sc64_range_work_begin(&work,5,5,walk_store+1,4,
       gradient_store+1,10,frontier_store+1,8,40,40,0,20)==0);
    while(work.phase!=SC64_RANGE_WORK_SEED)
        assert(sc64_range_work_pump(&work,1,&used)>=SC64_RANGE_WORK_PENDING);
    assert(sc64_range_work_pump(&work,3,&used)==SC64_RANGE_WORK_PENDING && used==3);
    sc64_range_work_reset(&work);
    assert(sc64_range_work_begin(&work,5,5,walk_store+1,4,
       gradient_store+1,10,frontier_store+1,8,40,40,0,20)==0);
    while(work.phase!=SC64_RANGE_WORK_FLOOD)
        assert(sc64_range_work_pump(&work,1,&used)>=SC64_RANGE_WORK_PENDING);
    assert(sc64_range_work_pump(&work,3,&used)==SC64_RANGE_WORK_PENDING && used==3);
    sc64_range_work_reset(&work);
    assert(sc64_range_work_query(&work,2,2,&nx,&ny,&mask)==SC64_RANGE_WORK_ERROR);
    assert(sc64_range_work_begin(&work,2,1,walk_store+1,1,
       gradient_store+1,1,frontier_store+1,2,8,8,0,1)==SC64_RANGE_WORK_ERROR);
    assert(sc64_range_work_begin(&work,3,3,walk_store+1,1,
       gradient_store+1,4,frontier_store+1,3,8,8,0,1)==SC64_RANGE_WORK_ERROR);
    assert(sc64_range_work_begin(&work,3,3,walk_store+1,2,
       gradient_store+1,3,frontier_store+1,4,8,8,0,1)==SC64_RANGE_WORK_ERROR);
    assert(sc64_range_work_begin(&work,3,3,walk_store+1,2,
       gradient_store+1,4,frontier_store+1,3,8,8,0,1)==SC64_RANGE_WORK_ERROR);
    assert(sc64_range_work_begin(&work,3,3,walk_store+1,2,
       gradient_store+1,4,frontier_store+1,6,48,8,0,1)==SC64_RANGE_WORK_ERROR);
    assert(sc64_range_work_begin(&work,3,3,walk_store+1,2,
       gradient_store+1,4,frontier_store+1,6,8,8,-1,1)==SC64_RANGE_WORK_ERROR);
    assert(sc64_range_work_begin(&work,3,3,walk_store+1,2,
       gradient_store+1,4,frontier_store+1,6,8,8,1,0)==SC64_RANGE_WORK_ERROR);
}

static void test_capacity_rounding(void)
{
    Sc64RangeWork work;
    unsigned used;
    for (unsigned n = 3; n <= 128u * 128u; ++n)
        assert(2u * ((n + 7u) / 8u) <= (3u * n + 7u) / 8u);
    assert(2u * ((1u + 7u) / 8u) > (3u * 1u + 7u) / 8u);
    assert(2u * ((2u + 7u) / 8u) > (3u * 2u + 7u) / 8u);
    assert(sizeof(work) <= 96u);
    prepare(128,128,0,0x5a,0xa5);
    assert(sc64_range_work_begin(&work,128,128,walk_store+1,2048,
        gradient_store+1,6144,frontier_store+1,4096,8,8,0,32)==
        SC64_RANGE_WORK_PENDING);
    assert(sc64_range_work_pump(&work,7,&used)==SC64_RANGE_WORK_PENDING);
    assert(used==7);
}

int main(void)
{
    test_capacity_rounding();
    check_map(3,3,24,24,0,0);
    check_map(9,7,66,47,0,32);
    check_map(11,9,0,0,12,44);
    test_random_maps();
    test_partial_and_blocked();
    test_disconnected_no_band();
    test_reset_and_bounds();
    puts("sc64 range work: ok");
    return 0;
}
