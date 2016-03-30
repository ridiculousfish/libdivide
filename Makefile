UNAME := $(shell uname)

ifeq ($(UNAME),Darwin)
# Mac OS X
ARCH_386 = -arch i386
ARCH_x64 = -arch x86_64
CC = clang
LINKFLAGS = 
else
# Just build natively on Linux et. al.
ARCH_386 =
ARCH_x64 =
CC = cc
LINKFLAGS = -lpthread
endif

CPP = c++

DEBUG_FLAGS = -fstrict-aliasing -g -O0 -DLIBDIVIDE_ASSERTIONS_ON=1 -msse2 -DLIBDIVIDE_USE_SSE2=1 -W -Wall
RELEASE_FLAGS = -fstrict-aliasing -W -Wall -g -O3 -msse2 -DLIBDIVIDE_USE_SSE2=1

test: release
	./tester

tester: debug

debug: libdivide_test.cpp libdivide.h
	$(CPP) $(DEBUG_FLAGS) $(ARCH_386) $(ARCH_x64) -g -o tester libdivide_test.cpp $(LINKFLAGS)

i386: libdivide_test.cpp libdivide.h
	$(CPP) $(DEBUG_FLAGS) $(ARCH_386) -o tester libdivide_test.cpp $(LINKFLAGS)

x86_64: libdivide_test.cpp libdivide.h
	$(CPP) $(DEBUG_FLAGS) $(ARCH_x64) -o tester libdivide_test.cpp $(LINKFLAGS)

release: libdivide_test.cpp libdivide.h
	$(CPP) $(RELEASE_FLAGS) $(ARCH_x64) $(ARCH_386) -o tester libdivide_test.cpp $(LINKFLAGS)

benchmark: libdivide_benchmark.c libdivide.h
	$(CC) $(RELEASE_FLAGS) $(ARCH_x64) $(ARCH_386) -o benchmark libdivide_benchmark.c $(LINKFLAGS)

primes: primes_benchmark.cpp libdivide.h
	$(CPP) -O3 -std=c++14 primes_benchmark.cpp -o $@

clean:
	rm -Rf primes tester tester.dSYM benchmark benchmark.dSYM

install:
	@echo "libdivide does not install! Just copy the header libdivide.h into your projects."
