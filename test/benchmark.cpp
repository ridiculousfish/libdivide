// Usage: benchmark [OPTIONS]
//
// You can pass the benchmark program one or more of the following
// options: u32, s32, u64, s64 to compare libdivide's speed against
// hardware division. If benchmark is run without any options u64
// is used as default option. benchmark tests a simple function that
// inputs an array of random numerators and a single divisor, and
// returns the sum of their quotients. It tests this using both
// hardware division, and the various division approaches supported
// by libdivide, including vector division.

// Silence MSVC sprintf unsafe warnings
#define _CRT_SECURE_NO_WARNINGS

#include "benchmark.h"

#include <string.h>

int main(int argc, char *argv[]) {
    // Disable printf buffering.
    // This is mainly required for Windows.
    setbuf(stdout, NULL);

    bool u16 = 0;
    bool s16 = 0;
    bool u32 = 0;
    bool s32 = 0;
    bool u64 = 0;
    bool s64 = 0;

    if (argc == 1) {
        // By default test only u64
        u64 = 1;
    } else {
        for (int i = 1; i < argc; i++) {
            if (!strcmp(argv[i], type_tag<uint16_t>::get_tag()))
                u16 = true;
            else if (!strcmp(argv[i], type_tag<uint32_t>::get_tag()))
                u32 = true;
            else if (!strcmp(argv[i], type_tag<uint64_t>::get_tag()))
                u64 = true;
            else if (!strcmp(argv[i], type_tag<int16_t>::get_tag()))
                s16 = true;
            else if (!strcmp(argv[i], type_tag<int32_t>::get_tag()))
                s32 = true;
            else if (!strcmp(argv[i], type_tag<int64_t>::get_tag()))
                s64 = true;
            else {
                printf(
                    "Usage: benchmark [OPTIONS]\n"
                    "\n"
                    "You can pass the benchmark program one or more of the following\n"
                    "options: u16, s16, u32, s32, u64, s64 to compare libdivide's speed against\n"
                    "hardware division. If benchmark is run without any options u64\n"
                    "is used as default option. benchmark tests a simple function that\n"
                    "inputs an array of random numerators and a single divisor, and\n"
                    "returns the sum of their quotients. It tests this using both\n"
                    "hardware division, and the various division approaches supported\n"
                    "by libdivide, including vector division.\n");
                exit(1);
            }
        }
    }

    if (u16) test_many<uint16_t>();
    if (s16) test_many<int16_t>();
    if (u32) test_many<uint32_t>();
    if (s32) test_many<int32_t>();
    if (u64) test_many<uint64_t>();
    if (s64) test_many<int64_t>();

    return 0;
}
