# libdivide
[![Build Status](https://ci.appveyor.com/api/projects/status/github/ridiculousfish/libdivide?branch=master&svg=true)](https://ci.appveyor.com/project/kimwalisch/libdivide)

```libdivide.h```  is a header-only C/C++ library for optimizing integer division, it has both
a C API and a C++ API. This is a summary of how to use libdivide's testing tools to develop
on libdivide itself.

See http://libdivide.com for more information on libdivide.

libdivide has 2 test tools:

* A verification utility **tester** used to help ensure that the division algorithm is correct.
* A benchmarking utility **benchmark** used to measure the speed increase.

# Build instructions

The tester and benchmark programs can be built using cmake and a recent C++ compiler
that supports C++11 or later. Optionally ```libdivide.h``` can also be installed to
```/usr/local/include```.

```bash
cmake .
make -j
sudo make install
```

# Tester binary

You can pass the **tester** binary one or more of the following arguments: ```u32```,
```s32```, ```u64```, ```s64``` to test the four cases (signed, unsigned, 32 bit, or 64 bit), or
run it with no arguments to test all four. The tester is multithreaded so it can test multiple
cases simultaneously. The tester will verify the correctness of libdivide via a set of randomly
chosen denominators, by comparing the result of libdivide's division to hardware division. It
may take a long time to run, but it will output as soon as it finds a discrepancy.

# Benchmark binary

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
so **benchmark** is valuable for its verification as well.

# C++ Usage example

The first code snippet divides all integers in a vector using integer division. This is slow as
integer division is at least one order of magnitude slower than any other integer arithmetic
operation on current CPUs.

```C++
void divide(std::vector<int64_t>& vect, int64_t divisor)
{
    // Slow, uses integer division
    for (auto& n : vect)
        n /= divisor;
}
```

The second code snippet runs much faster, it uses libdivide to compute the integer division
using multiplication and bit shifts hence avoiding the slow integer divison operation.

```C++
void divide(std::vector<int64_t>& vect, int64_t divisor)
{
    libdivide::divider<int64_t> fast_d(divisor);

    // Fast, computes division using libdivide
    for (auto& n : vect)
        n /= fast_d;
}
```

Generally libdivide will give at significant speedup if:

* The divisor is only known at runtime
* The divisor is reused multiple times e.g. in a loop

# Branchfull vs Branchfree

The default libdivide divider type makes use of
[branches](https://en.wikipedia.org/wiki/Branch_(computer_science)) to compute the integer
division. When the same divider is used inside a hot loop as in the C++ example section the
CPU will accurately predict the branches and there will be no performance slowdown. Often
the compiler is even able to move the branches outside the body of the loop hence
completely eliminating the branches, this is called loop-invariant code motion.

If however you are e.g. iterating over an array of dividers the CPU will not accurately predict
the branches and this will deteriorate performance. For this use case the branchfree divider
type will often run significantly faster, it computes the integer division without use of any
branches.

```C++
// Use branchfree divider
using branchfree_t = libdivide::branchfree_divider<uint64_t>;

uint64_t divide(uint64_t x, std::vector<branchfree_t>& vect)
{
    uint64_t sum = 0;

    for (auto& fast_d : vect)
        sum += x / fast_d;

    return sum;
}
```

Caveats of branchfree divider:

* Branchfree divider cannot be ```-1```, ```0```, ```1```
* Faster for unsigned types than for signed types

# Contributing

Before sending in patches to libdivide, please run the tester to completion with all four types,
and the benchmark utility for a reasonable period, to ensure that you have not introduced a
regression.

Happy hacking!
