#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include "../starcraft/sc64_p7_power_geometry.h"

/* The psi field's four horizontal bands in runtime pixels, expressed as
 * rectangles independently of the implementation's source-cell lookup. */
static int band_oracle(int x, int y)
{
    int width=128;
    if (x<-127 || x>127 || y<-79 || y>79) return 0;
    if (y>=32 || y<=-33) width=112;
    if (y>=48 || y<=-49) width=96;
    if (y>=64 || y<=-65) width=48;
    return x>=-width && x<width;
}

int main(void)
{
    for (int y=-100;y<=100;++y)
        for (int x=-150;x<=150;++x)
            assert(sc64_p7_in_psi_range(x,y)==band_oracle(x,y));
    /* Negative source-coordinate adjustment happens after the outer bound. */
    assert(sc64_p7_in_psi_range(-112,32));
    assert(!sc64_p7_in_psi_range(112,32));
    assert(sc64_p7_in_psi_range(112,-32));
    assert(!sc64_p7_in_psi_range(112,-33));
    assert(sc64_p7_in_psi_range(-48,64));
    assert(!sc64_p7_in_psi_range(48,64));
    assert(!sc64_p7_in_psi_range(-128,0));
    assert(!sc64_p7_in_psi_range(0,-80));
    assert(!sc64_p7_in_psi_range(INT_MIN,0));
    assert(!sc64_p7_in_psi_range(INT_MAX,0));
    assert(!sc64_p7_in_psi_range(0,INT_MIN));
    assert(!sc64_p7_in_psi_range(0,INT_MAX));
    puts("P7 psi-field geometry: ok");
    return 0;
}
