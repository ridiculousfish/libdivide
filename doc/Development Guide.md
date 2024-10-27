# libdivide Development Guide

Before sending in patches, build and run at least the ```tester``` and ```benchmark``` using the supplied ```cmake``` scripts on at least ```MSVC``` and ```GCC``` (or ```Clang```).

See [here](../README.md) for an overview of libdivide.

See [here](../test/avr/readme.md) for building & testing on a microcontroller.

## Building

### Summary

libdivide uses the same [```CMake```](https://cmake.org/) [script](./CMakeLists.txt) for local & [CI](#continuous-integration) builds. 

Sample build commands from [CI runs](https://github.com/ridiculousfish/libdivide/actions/workflows/canary_build.yml):
* Clang
  ```bash
  $ cmake -B ./build -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ -DLIBDIVIDE_BUILD_TESTS=ON -DCMAKE_BUILD_TYPE=Release
  $ cmake --build ./build --config Release
  ```
* MSVC (v2022)
  ```pwsh
  PS> cmake.exe -B ./build -G "Visual Studio 17 2022" -DCMAKE_C_COMPILER=cl.exe -DCMAKE_CXX_COMPILER=cl.exe -DLIBDIVIDE_BUILD_TESTS=ON -DCMAKE_BUILD_TYPE=Release
  PS> cmake.exe --build ./build --config Sanitize
  ```

### Detail

The script supports 5 target binaries (```--target``` CMake parameter):

| Target               | Purpose                       |
|----------------------|-------------------------------|
| benchmark            | [Benchmarking](#benchmarking) |
| benchmark_branchfree | [Benchmarking](#benchmarking) |
| tester               | [Testing](#testing)           |
| tester_c99           | [Testing](#testing)           |
| fast_div_generator   | Code Gen                      |

The CMake scripts support 4 build types (```-DCMAKE_BUILD_TYPE``` and ```--config``` CMake parameters):

* Debug
* Release
* RelWithDebInfo (Release + Debug Info, useful for debugging)
* Sanitizers (Release + Debug Info + [Sanitizers](https://github.com/google/sanitizers))

Build are configured to compile with:
  * All warnings on; and
  * Warnings as errors

Optionally ```libdivide.h``` can also be installed to ```/usr/local/include```:

```bash
~$ cmake .
~$ make -j
~$ sudo make install
```

## Testing

### Summary

The CMake script supports CTest, so running tests is as simple as:

```pwsh
PS> cd ./build
PS> ctest --build-config Sanitizers --verbose
```

### Detail

* Tester.exe: pass one or more of the following arguments: ```u16```, ```s16```, ```u32```, ```s32```, ```u64```, ```s64``` to test the six cases (signed, unsigned, 16-bit, 32-bit, or 64-bit), or run it with no arguments to test all six. The tester will verify the correctness of libdivide via a set of randomly chosen numerators and denominators, by comparing the result of libdivide's division to hardware division. It will stop with an error message as soon as it finds a discrepancy.
* Tester_c99: this is a parameterless safety net program to endure libdivide remains C99 compatible. I.e. any use of C++ language features is limited to C++ hosts.

## Benchmarking

### benchmark.exe

Pass one or more of the following arguments: ```u16```, ```s16```, ```u32```, ```s32```, ```u64```, ```s64``` to compare libdivide's speed against hardware division.

**benchmark** tests a simple function that inputs an array of random numerators and a single divisor, and returns the sum of their quotients. It tests this using both hardware division, and the various division approaches supported by libdivide, including vector division.

It will output data like this:

```bash
 #   system  scalar  scl_bf  vector  vec_bf   gener   algo
 1   9.684   0.792   0.783   0.426   0.426    1.346   0
 2   9.078   0.781   1.609   0.426   1.529    1.346   0
 3   9.078   1.355   1.609   1.334   1.531   29.045   1
 4   9.076   0.787   1.609   0.426   1.529    1.346   0
 5   9.074   1.349   1.609   1.334   1.531   29.045   1
 6   9.078   1.349   1.609   1.334   1.531   29.045   1
...
```

It will keep going as long as you let it, so it's best to stop it when you are happy with the denominators tested. These columns have the following significance. All times are in nanoseconds, lower is better.

```
     #:  The divisor that is tested
system:  Hardware divide time
scalar:  libdivide time, using scalar division
scl_bf:  libdivide time, using scalar branchfree division
vector:  libdivide time, using vector division
vec_bf:  libdivide time, using vector branchfree division
 gener:  Time taken to generate the divider struct
  algo:  The algorithm used.
```

The **benchmark** program will also verify that each function returns the same value, so benchmark is valuable for verification as well.

### benchmark_branchfree

The branchfree benchmark iterates over an array of dividers and computes divisions. This is the use case where the branchfree divider generally shines and where the default branchfull divider performs poorly because the CPU is not able to correctly predict the branches of the many different dividers.

## Continuous Integration

CI is implemented using [GitHub actions](https://github.com/ridiculousfish/libdivide/actions):

* Uses the ```cmake``` script described above.
* Builds all targets using various combinations of OS (Windows, Linux), compilers (GCC, Clang, MSVC), build types (Release, Sanitize).
* Runs ```tester```, ```test_c99``` and ```benchmark_branchfree``` in all build configurations.

## Releasing

Releases are semi-automated using GitHub actions:

1. Manually run the [Create draft release](https://github.com/ridiculousfish/libdivide/actions/workflows/prepare_release.yml) workflow/action: this does some codebase housekeeping (generating a new commit) and creates a draft release. 
2. Follow the output link in the action summary to the generated draft release. E.g. ![image](https://github.com/user-attachments/assets/7e8393f7-f204-4b3a-af37-de5e187479dc)
3. Edit the generated release notes as needed & publish

Note that PRs with the ```ignore-for-release``` label are excluded from the generated release notes.

### Happy hacking!