/* Compare all generated T6 availability bits with an independent raw-record
 * lookup. The private framed input is supplied at runtime, never embedded.
 * This checks source preparation, not a playable production path. */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include "sc64_unit_availability.h"
#include "sc64_t6_availability_generated.h"

int main(int argc, char **argv)
{
    unsigned char record[SC64_PUNI_BYTES + 8];
    assert(argc == 2);
    FILE *input = fopen(argv[1], "rb");
    assert(input);
    assert(fread(record, 1, sizeof(record), input) == sizeof(record));
    assert(fgetc(input) == EOF && !ferror(input));
    assert(fclose(input) == 0);
    assert(record[0] == 'P' && record[1] == 'U' &&
           record[2] == 'N' && record[3] == 'I');
    uint32_t length = record[4] | ((uint32_t)record[5] << 8) |
                      ((uint32_t)record[6] << 16) | ((uint32_t)record[7] << 24);
    assert(length == SC64_PUNI_BYTES);
    const unsigned char *puni = record + 8;
    unsigned checked = 0;
    for (unsigned player = 0; player < 12; ++player) {
        for (unsigned raw = 0; raw < 228; ++raw) {
            unsigned index = player * 228 + raw;
            unsigned expected = (puni[2964 + index] ?
                                 puni[2736 + raw] : puni[index]) != 0;
            unsigned actual = 99;
            assert(sc64_availability_bitmap_get(sc64_t6_availability_bits,
                SC64_T6_AVAILABILITY_BIT_BYTES, player, raw, &actual));
            assert(actual == expected);
            ++checked;
        }
    }
    assert(checked == 2736);
    /* The source permits the human Dropship and its Starport/add-on route.
     * Actual construction, attachment, costs and acquisition need engine tests. */
    const unsigned required[] = {7, 11, 106, 109, 110, 111, 113, 114, 115};
    for (unsigned i = 0; i < sizeof(required) / sizeof(required[0]); ++i) {
        unsigned allowed = 0;
        assert(sc64_availability_bitmap_get(sc64_t6_availability_bits,
            SC64_T6_AVAILABILITY_BIT_BYTES, 0, required[i], &allowed));
        assert(allowed);
    }
    puts("T6 source availability: all 2736 effective bits match; Dropship route permitted");
    return 0;
}
