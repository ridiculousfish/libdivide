#pragma once

#include <time.h>

#if defined(__AVR__)
int freeRAM()
{
	extern int __heap_start, *__brkval;
	int v;
	return (int) &v - (__brkval == 0 ? (int) &__heap_start: (int) __brkval);
}
#endif

// A READONLY buffer of RANDOM LENGTH and RANDOM CONTENT
template <typename T>
class random_numerators {
private:

    uint32_t *_pData;
    uint32_t _length;

    uint32_t *allocate(uint32_t size) {
#if defined(__AVR__)
        return (uint32_t *)malloc(size);
#elif defined(LIBDIVIDE_WINDOWS)
        /* Align memory to 64 byte boundary for AVX512 */
        return (uint32_t *)_aligned_malloc(size, 64);
#else
        /* Align memory to 64 byte boundary for AVX512 */
        void *ptr = NULL;
        int failed = posix_memalign(&ptr, 64, size);
        if (failed) {
            printf("Failed to align memory!\n");
            exit(1);
        }
        return  (uint32_t *)ptr;
#endif
    }

    static void deallocate(uint32_t *pData) {
#if defined(LIBDIVIDE_WINDOWS)
        _aligned_free((void *)pData);
#else
        free((void *)pData);
#endif        
    }

    struct random_state {
        uint32_t hi;
        uint32_t lo;
    };

    static uint32_t my_random(struct random_state *state) {
        state->hi = (state->hi << 16) + (state->hi >> 16);
        state->hi += state->lo;
        state->lo += state->hi;
        return state->hi;
    }

    void randomize_buffer() {
        size_t size = (length() * sizeof(T)) / sizeof(*_pData);
        struct random_state state = { 2147483563, 2147483563 ^ 0x49616E42 };
        for (size_t i = 0; i < size; i++) {
            _pData[i] = my_random(&state);
        }
    }

public:

    using const_pointer   = const T *;
    using const_reference = const T&;
    using const_iterator  = const_pointer;

    random_numerators() {
#if defined(__AVR__)
        // Using 1/8 of free RAM should be enough to fool the 
        // compiler optimizer and give enough test iterations on
        // AVR hardware
        _length = (freeRAM()/8)/sizeof(T);
#else
        _length = 1 << 19;
        // Make sure that the number of iterations is not
        // known at compile time to prevent the compiler
        // from magically calculating results at compile
        // time and hence falsifying the benchmark.        
        srand((unsigned)time(NULL));
        _length += (rand() % 3) * (1 << 10);
#endif 
        _pData = allocate(_length * sizeof(T));

        randomize_buffer();
    }

    ~random_numerators() {
        deallocate(_pData);
    }

    const_iterator begin() const { return (const_pointer)_pData; }
    const_iterator end() const { return begin()+length(); }

    uint32_t length() const { return _length; }
};