# libdivide C++ API

The entire content of ```libdivide.h``` is wrapped inside the ```libdivide``` namespace,
for clarity the ```libdivide``` namespace is omitted in the code sections below.

## divider class

```C++
// This is the main divider class for use by the user (C++ API).
// The actual division algorithm is selected using the dispatcher struct
// based on the integer and algorithm template parameters.
template<typename T, Branching ALGO = BRANCHFULL>
class divider {
public:
    // Generate a libdivide divisor for d
    divider(T d);
    // Recover the original divider
    T recover() const;
    bool operator==(const divider<T, ALGO>& other) const;
    bool operator!=(const divider<T, ALGO>& other) const;
    // ...
private:
    // Storage for the actual divisor
    dispatcher<T, ALGO> div;
};
```

## branchfree_divider

```branchfree_divider``` is a convenience typedef which redirects to the divider class:

```C++
template <typename T>
using branchfree_divider = divider<T, BRANCHFREE>;
```

## Operator ```/``` and ```/=```

```C++
// Overload of operator /
template<typename T, Branching ALGO>
T operator/(T n, const divider<T, ALGO>& div);

// Overload of operator /=
template<typename T, Branching ALGO>
T& operator/=(T& n, const divider<T, ALGO>& div);
```

## NEON vector division

```C++
// Overload of operator /
template <Branching ALGO>
uint32x4_t operator/(uint32x4_t n, const divider<uint32_t, ALGO> &div)

template <Branching ALGO>
int32x4_t operator/(int32x4_t n, const divider<int32_t, ALGO> &div)

template <Branching ALGO>
uint64x2_t operator/(uint64x2_t n, const divider<uint64_t, ALGO> &div)

template <Branching ALGO>
int64x2_t operator/(int64x2_t n, const divider<int64_t, ALGO> &div)


// Overload of operator /=
template <Branching ALGO>
uint32x4_t operator/=(uint32x4_t &n, const divider<uint32_t, ALGO> &div)

template <Branching ALGO>
int32x4_t operator/=(int32x4_t &n, const divider<int32_t, ALGO> &div)

template <Branching ALGO>
uint64x2_t operator/=(uint64x2_t &n, const divider<uint64_t, ALGO> &div);

template <Branching ALGO>
int64x2_t operator/=(int64x2_t &n, const divider<int64_t, ALGO> &div)
```

You need to define ```LIBDIVIDE_NEON``` to enable SSE2 vector division.


## SSE2 vector division

```C++
// Overload of operator /
template<typename T, Branching ALGO>
__m128i operator/(__m128i n, const divider<T, ALGO>& div);

// Overload of operator /=
template<typename T, Branching ALGO>
__m128i& operator/=(__m128i& n, const divider<T, ALGO>& div);
```

You need to define ```LIBDIVIDE_SSE2``` to enable SSE2 vector division.

## AVX2 vector division

```C++
// Overload of operator /
template<typename T, Branching ALGO>
__m256i operator/(__m256i n, const divider<T, ALGO>& div);

// Overload of operator /=
template<typename T, Branching ALGO>
__m256i& operator/=(__m256i& n, const divider<T, ALGO>& div);
```

You need to define ```LIBDIVIDE_AVX2``` to enable AVX2 vector division.

## AVX512 vector division

```C++
// Overload of operator /
template<typename T, Branching ALGO>
__m512i operator/(__m512i n, const divider<T, ALGO>& div);

// Overload of operator /=
template<typename T, Branching ALGO>
__m512i& operator/=(__m512i& n, const divider<T, ALGO>& div);
```

You need to define ```LIBDIVIDE_AVX512``` to enable AVX512 vector division.
