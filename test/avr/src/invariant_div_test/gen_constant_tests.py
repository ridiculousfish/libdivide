import os
import random
from typing import Any, TYPE_CHECKING

# Tell type checkers about the construction environment that PIO injects
if TYPE_CHECKING:
    Import: Any = None
    env: Any = {}

Import("env")

is_unsigned = any((flag for flag in env['BUILD_FLAGS'] if 'TEST_UNSIGNED' in flag))
is_mod = any((flag for flag in env['BUILD_FLAGS'] if 'TEST_MOD' in flag))
# The number of denominators was chosen so that all tests fit on an AtMega2560
num_unsigned = 250 if is_mod else 340
num_denoms = 256 if not is_unsigned else num_unsigned
print(f'Generating test files. Unsigned {is_unsigned}, Mod {is_mod}, Num Denoms {num_denoms}')


def eratosthenes():
    '''Yields the sequence of prime numbers via the Sieve of Eratosthenes.'''
    D = {}  # map composite integers to primes witnessing their compositeness
    q = 2   # first integer to test for primality
    while 1:
        if q not in D:
            yield q         # not marked composite, must be prime
            D[q*q] = [q]    # first multiple of q not already marked
        else:
            for p in D[q]:  # move each witness to its next multiple
                D.setdefault(p+q, []).append(p)
            del D[q]        # no longer need D[q], free memory
        q += 1


def before_build():
    def get_denoms(count):
        # 14 signed, 15 unsigned
        range_end_bits = 15 if is_unsigned else 14
        # Small numbers
        denoms = set([i for i in range(2, 32)])
        # Powers of 2
        denoms = denoms.union([2**j for j in range(2, range_end_bits-1)])
        # Prime numbers
        denoms = denoms.union([p for i, p in zip(range(1, 100), eratosthenes())])
        # End of the range
        denoms.add((2**range_end_bits)-2)
        # Fill in the rest of the denominators with random
        while (len(denoms) < count):
            denoms.add(random.randint(33, (2**range_end_bits)-1))
        return sorted(denoms)

    # Build the denominators
    denoms = get_denoms(num_denoms)

    genfile = os.path.join(env['PROJECT_SRC_DIR'], 'invariant_div_test', 'test_declares.g.hpp')
    print(f'Generating {genfile}')
    with open(genfile, 'w') as f:
        for index in denoms:
            print(f'DEFINE_BOTH({index});', file=f)

    genfile = os.path.join(env['PROJECT_SRC_DIR'], 'invariant_div_test', 'test.g.hpp')
    print(f'Generating {genfile}')
    with open(genfile, 'w') as f:
        for index in denoms:
            print(f'RUN_TEST_BOTH({index});', file=f)


before_build()
