/* SPDX-License-Identifier: GPL-3.0-or-later
 * Port-free mGBA 0.10.5 backend for the controller replay tools.
 * Build against the matching private libmgba.a with USE_DEBUGGERS enabled.
 * The line protocol exposes only reads, writes, hardware breakpoints, continue
 * and the 64-bit emulated cycle clock. It never modifies the ROM file. */
#include <mgba/core/core.h>
#include <mgba/core/log.h>
#include <mgba/core/timing.h>
#include <mgba/debugger/debugger.h>
#include <inttypes.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static color_t pixels[256 * 256];
static int stopped;
static uint32_t stop_address;
static void log_stderr(struct mLogger *l, int category, enum mLogLevel level,
                       const char *format, va_list args) {
    (void) l; (void) category; (void) level;
    vfprintf(stderr, format, args); fputc('\n', stderr);
}
static void entered(struct mDebugger *d, enum mDebuggerEntryReason reason,
                    struct mDebuggerEntryInfo *info) {
    (void) d;
    if (reason == DEBUGGER_ENTER_BREAKPOINT && info) {
        stopped = 1; stop_address = info->address;
    } else stopped = -1;
}
static int hexval(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    return -1;
}
/* No signs, whitespace, empty fields or arithmetic wrap. */
static int number(const char **cursor, unsigned base, uint32_t *out) {
    const char *p = *cursor; uint32_t n = 0; int digit, count = 0;
    while ((digit = hexval(*p)) >= 0 && (unsigned)digit < base) {
        if (n > (UINT32_MAX - (unsigned)digit) / base) return 0;
        n = n * base + (unsigned)digit; ++p; ++count;
    }
    if (!count) return 0;
    *cursor = p; *out = n; return 1;
}
static int in_readable_memory(uint32_t address, uint32_t length, size_t rom_size) {
    return length && ((address >= 0x02000000 && address < 0x02040000 && length <= 0x02040000 - address) ||
                      (address >= 0x03000000 && address < 0x03008000 && length <= 0x03008000 - address) ||
                      (address >= 0x08000000 && address < 0x0a000000 && length <= 0x0a000000 - address &&
                       (uint64_t)(address - 0x08000000) + length <= rom_size));
}
int main(int argc, char **argv) {
    if (argc != 12) return 2;
    struct { uint32_t start, size; } allowed[5];
    for (unsigned i=0; i<5; ++i) {
        const char *a=argv[2+i*2], *b=argv[3+i*2]; uint32_t start, size;
        if (!number(&a,16,&start) || *a || !number(&b,10,&size) || *b || !size ||
            start < 0x02000000 || start >= 0x02040000 || size > 0x02040000 - start) return 2;
        allowed[i].start=start; allowed[i].size=size;
        for (unsigned j=0; j<i; ++j)
            if (start < (uint64_t)allowed[j].start + allowed[j].size && allowed[j].start < start + size) return 2;
    }
    struct mLogger logger = {.log = log_stderr};
    mLogSetDefaultLogger(&logger);
    struct mCore *core = mCoreFind(argv[1]);
    if (!core || !core->init(core)) return 3;
    mCoreInitConfig(core, "sc64-headless");
    core->setVideoBuffer(core, pixels, 256);
    if (!mCoreLoadFile(core, argv[1])) return 4;
    mCoreLoadConfig(core); core->reset(core);
    struct mDebugger debugger = {0};
    debugger.type = DEBUGGER_CUSTOM; debugger.entered = entered;
    mDebuggerAttach(&debugger, core);
    struct { uint32_t address; ssize_t id; } points[8];
    unsigned point_count = 0;
    char line[4096];
    puts("READY"); fflush(stdout);
    while (fgets(line, sizeof(line), stdin)) {
        size_t size=strlen(line);
        if (!size || line[size-1] != '\n') {
            int ch; while ((ch=getchar()) != '\n' && ch != EOF) {}
            puts("ERR line"); fflush(stdout); continue;
        }
        line[--size]='\0';
        const char *p=line+1; uint32_t address=0, length=0;
        if (line[0] == 'm' && number(&p,16,&address) && *p++ == ',' &&
            number(&p,10,&length) && !*p && length <= 512 && in_readable_memory(address,length,core->romSize(core))) {
            for (unsigned i = 0; i < length; ++i)
                printf("%02x", (unsigned) core->rawRead8(core, address + i, -1) & 255);
            putchar('\n');
        } else if (line[0] == 'M' && number(&p,16,&address) && *p++ == ',' &&
                   number(&p,10,&length) && *p++ == ':' && length && length <= 512) {
            const char *data = p-1; int valid = strlen(p) == 2u * length;
            for (unsigned i = 0; valid && i < length; ++i)
                valid = hexval(data[1 + 2*i]) >= 0 && hexval(data[2 + 2*i]) >= 0;
            int permitted=0;
            for (unsigned i=0; i<5; ++i)
                if (address >= allowed[i].start && (uint64_t)address + length <= (uint64_t)allowed[i].start + allowed[i].size) permitted=1;
            if (!valid || !permitted) puts("ERR write");
            else {
                for (unsigned i = 0; i < length; ++i)
                    core->busWrite8(core, address + i, (uint8_t) (hexval(data[1+2*i])*16 + hexval(data[2+2*i])));
                puts("OK");
            }
        } else if (line[0] == 'Z' && number(&p,16,&address) && !*p && point_count < 8) {
            struct mBreakpoint bp = {.address=address, .segment=-1, .type=BREAKPOINT_HARDWARE};
            ssize_t id = debugger.platform->setBreakpoint(debugger.platform, &bp);
            if (id < 0) puts("ERR breakpoint");
            else { points[point_count].address=address; points[point_count++].id=id; puts("OK"); }
        } else if (line[0] == 'z' && number(&p,16,&address) && !*p) {
            unsigned i;
            for (i=0; i<point_count && points[i].address != address; ++i) {}
            if (i == point_count || !debugger.platform->clearBreakpoint(debugger.platform, points[i].id)) puts("ERR breakpoint");
            else { points[i]=points[--point_count]; puts("OK"); }
        } else if (!strcmp(line, "c")) {
            uint64_t start = mTimingGlobalTime(core->timing);
            stopped = 0; debugger.state = DEBUGGER_RUNNING;
            while (!stopped && mTimingGlobalTime(core->timing) - start < UINT64_C(17179869184))
                mDebuggerRun(&debugger);
            if (stopped == 1) printf("STOP %08x\n", stop_address);
            else puts("ERR execution");
        } else if (!strcmp(line, "t")) printf("%" PRIu64 "\n", mTimingGlobalTime(core->timing));
        else if (!strcmp(line, "q")) break;
        else puts("ERR command");
        fflush(stdout);
    }
    core->detachDebugger(core); core->unloadROM(core);
    mCoreConfigDeinit(&core->config); core->deinit(core);
    return 0;
}
