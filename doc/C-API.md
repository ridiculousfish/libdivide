# libdivide C API

Note that all of libdivide's public API functions are declared as ```static inline```
for performance reasons, however ```static inline``` is omitted in the code sections
below in order to increase readability.

## Generate libdivide divider

```C
/* Generate a libdivide divider */
struct libdivide_s16_t libdivide_s16_gen(int16_t d);
struct libdivide_u16_t libdivide_u16_gen(uint16_t d);
struct libdivide_s32_t libdivide_s32_gen(int32_t d);
struct libdivide_u32_t libdivide_u32_gen(uint32_t d);
struct libdivide_s64_t libdivide_s64_gen(int64_t d);
struct libdivide_u64_t libdivide_u64_gen(uint64_t d);

/* Generate a branchfree libdivide divider */
struct libdivide_s16_branchfree_t libdivide_s16_branchfree_gen(int16_t d);
struct libdivide_u16_branchfree_t libdivide_u16_branchfree_gen(uint16_t d);
struct libdivide_s32_branchfree_t libdivide_s32_branchfree_gen(int32_t d);
struct libdivide_u32_branchfree_t libdivide_u32_branchfree_gen(uint32_t d);
struct libdivide_s64_branchfree_t libdivide_s64_branchfree_gen(int64_t d);
struct libdivide_u64_branchfree_t libdivide_u64_branchfree_gen(uint64_t d);
```

## libdivide division

```C
/* libdivide division */
int16_t  libdivide_s16_do(int16_t numer, const struct libdivide_s16_t *denom);
uint16_t libdivide_u16_do(uint16_t numer, const struct libdivide_u16_t *denom);
int32_t  libdivide_s32_do(int32_t numer, const struct libdivide_s32_t *denom);
uint32_t libdivide_u32_do(uint32_t numer, const struct libdivide_u32_t *denom);
int64_t  libdivide_s64_do(int64_t numer, const struct libdivide_s64_t *denom);
uint64_t libdivide_u64_do(uint64_t numer, const struct libdivide_u64_t *denom);

/* libdivide branchfree division */
int16_t  libdivide_s16_branchfree_do(int16_t numer, const struct libdivide_s16_branchfree_t *denom);
uint16_t libdivide_u16_branchfree_do(uint16_t numer, const struct libdivide_u16_branchfree_t *denom);
int32_t  libdivide_s32_branchfree_do(int32_t numer, const struct libdivide_s32_branchfree_t *denom);
uint32_t libdivide_u32_branchfree_do(uint32_t numer, const struct libdivide_u32_branchfree_t *denom);
int64_t  libdivide_s64_branchfree_do(int64_t numer, const struct libdivide_s64_branchfree_t *denom);
uint64_t libdivide_u64_branchfree_do(uint64_t numer, const struct libdivide_u64_branchfree_t *denom);
```

## libdivide NEON vector division

```C
/* libdivide NEON division */
uint32x4_t libdivide_u32_do_vec128(uint32x4_t numers, const struct libdivide_u32_t *denom);
int32x4_t libdivide_s32_do_vec128(int32x4_t numers, const struct libdivide_s32_t *denom);
uint64x2_t libdivide_u64_do_vec128(uint64x2_t numers, const struct libdivide_u64_t *denom);
int64x2_t libdivide_s64_do_vec128(int64x2_t numers, const struct libdivide_s64_t *denom);

/* libdivide NEON branchfree division */
uint32x4_t libdivide_u32_branchfree_do_vec128(uint32x4_t numers, const struct libdivide_u32_branchfree_t *denom);
int32x4_t libdivide_s32_branchfree_do_vec128(int32x4_t numers, const struct libdivide_s32_branchfree_t *denom);
uint64x2_t libdivide_u64_branchfree_do_vec128(uint2x4_t numers, const struct libdivide_u64_branchfree_t *denom);
int64x2_t libdivide_s64_branchfree_do_vec128(int64x2_t numers, const struct libdivide_s64_branchfree_t *denom);
```

You need to define ```LIBDIVIDE_NEON``` to enable NEON vector division.

## libdivide SSE2 vector division

```C
/* libdivide SSE2 division */
__m128i libdivide_u32_do_vec128(__m128i numers, const struct libdivide_u32_t *denom);
__m128i libdivide_s32_do_vec128(__m128i numers, const struct libdivide_s32_t *denom);
__m128i libdivide_u64_do_vec128(__m128i numers, const struct libdivide_u64_t *denom);
__m128i libdivide_s64_do_vec128(__m128i numers, const struct libdivide_s64_t *denom);

/* libdivide SSE2 branchfree division */
__m128i libdivide_u32_branchfree_do_vec128(__m128i numers, const struct libdivide_u32_branchfree_t *denom);
__m128i libdivide_s32_branchfree_do_vec128(__m128i numers, const struct libdivide_s32_branchfree_t *denom);
__m128i libdivide_u64_branchfree_do_vec128(__m128i numers, const struct libdivide_u64_branchfree_t *denom);
__m128i libdivide_s64_branchfree_do_vec128(__m128i numers, const struct libdivide_s64_branchfree_t *denom);
```

You need to define ```LIBDIVIDE_SSE2``` to enable SSE2 vector division.

## libdivide AVX2 vector division

```C
/* libdivide AVX2 division */
__m256i libdivide_u32_do_vec256(__m256i numers, const struct libdivide_u32_t *denom);
__m256i libdivide_s32_do_vec256(__m256i numers, const struct libdivide_s32_t *denom);
__m256i libdivide_u64_do_vec256(__m256i numers, const struct libdivide_u64_t *denom);
__m256i libdivide_s64_do_vec256(__m256i numers, const struct libdivide_s64_t *denom);

/* libdivide AVX2 branchfree division */
__m256i libdivide_u32_branchfree_do_vec256(__m256i numers, const struct libdivide_u32_branchfree_t *denom);
__m256i libdivide_s32_branchfree_do_vec256(__m256i numers, const struct libdivide_s32_branchfree_t *denom);
__m256i libdivide_u64_branchfree_do_vec256(__m256i numers, const struct libdivide_u64_branchfree_t *denom);
__m256i libdivide_s64_branchfree_do_vec256(__m256i numers, const struct libdivide_s64_branchfree_t *denom);
```

You need to define ```LIBDIVIDE_AVX2``` to enable AVX2 vector division.

## libdivide AVX512 vector division

```C
/* libdivide AVX512 division */
__m512i libdivide_u32_do_vec512(__m512i numers, const struct libdivide_u32_t *denom);
__m512i libdivide_s32_do_vec512(__m512i numers, const struct libdivide_s32_t *denom);
__m512i libdivide_u64_do_vec512(__m512i numers, const struct libdivide_u64_t *denom);
__m512i libdivide_s64_do_vec512(__m512i numers, const struct libdivide_s64_t *denom);

/* libdivide AVX512 branchfree division */
__m512i libdivide_u32_branchfree_do_vec512(__m512i numers, const struct libdivide_u32_branchfree_t *denom);
__m512i libdivide_s32_branchfree_do_vec512(__m512i numers, const struct libdivide_s32_branchfree_t *denom);
__m512i libdivide_u64_branchfree_do_vec512(__m512i numers, const struct libdivide_u64_branchfree_t *denom);
__m512i libdivide_s64_branchfree_do_vec512(__m512i numers, const struct libdivide_s64_branchfree_t *denom);
```

You need to define ```LIBDIVIDE_AVX512``` to enable AVX512 vector division.

## Recover divider

```C
/* Recover the original divider */
int16_t  libdivide_s16_recover(const struct libdivide_s16_t *denom);
uint16_t libdivide_u16_recover(const struct libdivide_u16_t *denom);
int32_t  libdivide_s32_recover(const struct libdivide_s32_t *denom);
uint32_t libdivide_u32_recover(const struct libdivide_u32_t *denom);
int64_t  libdivide_s64_recover(const struct libdivide_s64_t *denom);
uint64_t libdivide_u64_recover(const struct libdivide_u64_t *denom);

/* Recover the original divider */
int16_t  libdivide_s16_branchfree_recover(const struct libdivide_s16_branchfree_t *denom);
uint16_t libdivide_u16_branchfree_recover(const struct libdivide_u16_branchfree_t *denom);
int32_t  libdivide_s32_branchfree_recover(const struct libdivide_s32_branchfree_t *denom);
uint32_t libdivide_u32_branchfree_recover(const struct libdivide_u32_branchfree_t *denom);
int64_t  libdivide_s64_branchfree_recover(const struct libdivide_s64_branchfree_t *denom);
uint64_t libdivide_u64_branchfree_recover(const struct libdivide_u64_branchfree_t *denom);
```
