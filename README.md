# libdivide
[![Build Status](https://ci.appveyor.com/api/projects/status/github/ridiculousfish/libdivide?branch=master&svg=true)](https://ci.appveyor.com/project/kimwalisch/libdivide)

```libdivide.h```  is a header-only C/C++ library for optimizing integer division, it has both
a C API and a C++ API. This is a summary of how to use libdivide's testing tools to develop on libdivide itself.

See http://libdivide.com for more information on libdivide.

libdivide has 2 test tools:

* A verification utility **tester** used to help ensure that the division algorithm is correct.
* A benchmarking utility **benchmark** used to measure the speed increase.

## Build instructions

The tester and benchmark programs can be built using cmake and a recent C++ compiler
that supports C++11 or later. Optionally ```libdivide.h``` can also be installed to
```/usr/local/include```.

```bash
cmake .
make -j
sudo make install
```

## Tester binary

You can pass the **tester** binary one or more of the following arguments: ```u32```,
```s32```, ```u64```, ```s64``` to test the four cases (signed, unsigned, 32 bit, or 64 bit), or
run it with no arguments to test all four. The tester is multithreaded so it can test multiple
cases simultaneously. The tester will verify the correctness of libdivide via a set of randomly
chosen denominators, by comparing the result of libdivide's division to hardware division. It
may take a long time to run, but it will output as soon as it finds a discrepancy.

## Benchmark binary

You can pass the **benchmark** binary one or more of the following arguments: ```u32```,
```s32```, ```u64```, ```s64``` to compare libdivide's speed against hardware division.
**benchmark** tests a simple function that inputs an array of random numerators and a single
divisor, and returns the sum of their quotients.  It tests this using both hardware division, and
the various division approaches supported by libdivide, including vector division.

It will output data like this:

```bash
#  system  scalar  scl_bf  scl_us  vector  vec_bf  vec_us   gener  algo
1   5.453   0.654   0.570   0.223   0.565   0.603   0.235   1.282     0
2   5.453   1.045   0.570   0.496   0.568   0.603   0.511  11.215     1
3   5.453   1.534   0.570   0.570   0.587   0.603   0.570  11.887     2
4   5.409   0.654   0.570   0.223   0.565   0.603   0.235   1.282     0
5   5.409   1.045   0.570   0.496   0.568   0.603   0.509  11.215     1
6   5.409   1.534   0.570   0.570   0.587   0.603   0.570  11.887     2

...
```

It will keep going as long as you let it, so it's best to stop it when you are happy with the
denominators tested.  These columns have the following significance.  All times are in
nanoseconds, and lower is better.

```bash
     #:  The divisor that is tested
system:  Hardware divide time
scalar:  libdivide time, using scalar functions
scl_bf:  libdivide time, using branchfree scalar functions
scl_us:  libdivide time, using scalar unswitching functions
vector:  libdivide time, using vector functions
vec_bf:  libdivide time, using branchfree vector functions
vec_us:  libdivide time, using vector unswitching
 gener:  Time taken to generate the divider struct
  algo:  The algorithm used. See libdivide_*_get_algorithm
```

The benchmarking utility will also verify that each function returns the same value,
so "benchmark" is valuable for its verification as well.

## Contributing

Before sending in patches to libdivide, please run the tester to completion with all four types,
and the benchmark utility for a reasonable period, to ensure that you have not introduced a
regression.

Happy hacking!
