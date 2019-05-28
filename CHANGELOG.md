# Changelog

This is a list of notable changes to libdivide.

## [1.1](https://github.com/ridiculousfish/libdivide/releases/tag/v1.1) - 2019-05-27
* BUG FIXES
  * Fix bug in ```libdivide_128_div_64_to_64()``` ([#45](https://github.com/ridiculousfish/libdivide/issues/45))
  * Fix ```-Wshift-count-overflow``` warning on avr CPU architecture ([#41](https://github.com/ridiculousfish/libdivide/pull/41))
  * Fix ```-Wshadow``` warning in ```libdivide_s32_do()```
  * Fix ```-Wignored-attributes``` warnings when compiling SSE2 code using GCC 9
* ENHANCEMENT
  * Add ```LIBDIVIDE_VERSION``` macro to ```libdivide.h```
  * Clean up SSE2 code in ```libdivide.h```
  * Clean up macros in ```libdivide.h```
  * Increase runtime of test cases in ```primes_benchmark.cpp```
* BUILD
  * Remove windows directory with legacy Visual Studio project files
  * Move test programs to test directory

## [1.0](https://github.com/ridiculousfish/libdivide/releases/tag/v1.0) - 2018-01-21
* BREAKING
  * Branchfull divider must not be ```0``` ([#38](https://github.com/ridiculousfish/libdivide/pull/38))
  * Branchfree divider must not be ```-1```, ```0```, ```1``` ([#38](https://github.com/ridiculousfish/libdivide/pull/38))
* ENHANCEMENT
  * Add proper error handling ([#38](https://github.com/ridiculousfish/libdivide/pull/38))
  * Add C++ support for ```/=``` operator
  * Speedup 64-bit divisor recovery by up to 30%
  * Simplify C++ templates
  * Add include guards to ```libdivide.h```!
  * Get rid of ```goto``` in ```libdivide_128_div_64_to_64()```
  * Use ```#if defined(MACRO)``` instead of ```#if MACRO```
  * Silence compiler warnings from crash functions
* TESTING
  * Tests should ```exit(1)``` on error, required by ```make test```
  * Silence unused parameter warnings
  * Silence GCC 7.2.0 maybe uninitialized warnings
  * Silence unused return value warning
* BUILD
  * Port build system from ```make``` to ```CMake```
  * Automatically detect if the CPU and compiler support SSE2
  * Automatically enable C++11
* DOCS
  * Update build instructions in ```README.md```
  * Update benchmark section with branchfree divider
  * Add C example section
  * Add C++ example section
  * Add "Branchfull vs branchfree" section
  * Add section about unswitching
  * New ```CHANGELOG.md```file
