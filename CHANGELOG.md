# Changelog

This is a list of notable changes to libdivide.

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
