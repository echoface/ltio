#ifndef TIMEOUT_BITOPS_H
#define TIMEOUT_BITOPS_H

#include <stdint.h>
#ifdef _MSC_VER
#include <intrin.h> /* _BitScanForward, _BitScanReverse */
#endif

/* First define ctz and clz functions */

#if defined(__GNUC__)
/* On GCC and clang and some others, we can use __builtin functions. They are
 * not defined for n==0, but timeout.c never calls them with n==0.
 */
#define ctz64(n) __builtin_ctzll(n)
#define clz64(n) __builtin_clzll(n)
#if LONG_MAX == INT32_MAX
#define ctz32(n) __builtin_ctzl(n)
#define clz32(n) __builtin_clzl(n)
#else
#define ctz32(n) __builtin_ctz(n)
#define clz32(n) __builtin_clz(n)
#endif

#elif defined(_MSC_VER)
/* On MSVC, we have these handy functions. We can ignore their return values,
 * since we will never supply val == 0.
 */

static __inline int ctz32(unsigned long val)
{
    DWORD zeros = 0;
    _BitScanForward(&zeros, val);
    return zeros;
}
static __inline int clz32(unsigned long val)
{
    DWORD zeros = 0;
    _BitScanReverse(&zeros, val);
    return zeros;
}
#ifdef _WIN64
/* According to the documentation, these only exist on Win64. */
static __inline int ctz64(uint64_t val)
{
    DWORD zeros = 0;
    _BitScanForward64(&zeros, val);
    return zeros;
}
static __inline int clz64(uint64_t val)
{
    DWORD zeros = 0;
    _BitScanReverse64(&zeros, val);
    return zeros;
}
#else
static __inline int ctz64(uint64_t val)
{
    uint32_t lo = (uint32_t) val;
    uint32_t hi = (uint32_t)(val >> 32);
    return lo ? ctz32(lo) : 32 + ctz32(hi);
}
static __inline int clz64(uint64_t val)
{
    uint32_t lo = (uint32_t) val;
    uint32_t hi = (uint32_t)(val >> 32);
    return hi ? clz32(hi) : 32 + clz32(lo);
}
#endif

#else /* Generic */
/* TODO: There are more clever ways to do this in the generic case. */
#define process_(one, cz_bits, bits)     \
    if (x < (one << (cz_bits - bits))) { \
        rv += bits;                      \
        x <<= bits;                      \
    }

#define process64(bits) process_((UINT64_C(1)), 64, (bits))
static inline int clz64(uint64_t x)
{
    int rv = 0;
    process64(32);
    process64(16);
    process64(8);
    process64(4);
    process64(2);
    process64(1);
    return rv;
}

#define process32(bits) process_((UINT32_C(1)), 32, (bits))
static inline int clz32(uint32_t x)
{
    int rv = 0;
    process32(16);
    process32(8);
    process32(4);
    process32(2);
    process32(1);
    return rv;
}

#undef process_
#undef process32
#undef process64

#define process_(one, bits)                 \
    if ((x & ((one << (bits)) - 1)) == 0) { \
        rv += bits;                         \
        x >>= bits;                         \
    }

#define process64(bits) process_((UINT64_C(1)), bits)
static inline int ctz64(uint64_t x)
{
    int rv = 0;
    process64(32);
    process64(16);
    process64(8);
    process64(4);
    process64(2);
    process64(1);
    return rv;
}

#define process32(bits) process_((UINT32_C(1)), bits)
static inline int ctz32(uint32_t x)
{
    int rv = 0;

    process32(16);
    process32(8);
    process32(4);
    process32(2);
    process32(1);
    return rv;
}

#undef process32
#undef process64
#undef process_

#endif /* generic case */

/* bitwise rotation */

/* We need to find a rotate idiom that accepts 0-sized rotates.  So we can't
 * use the "normal" (v<<c) | (v >> (bits-c)), since if c==0, then v >>
 * (bits-c) will be undefined.
 *
 * The definitions below will also work okay when c is greater than the
 * bit width, though we don't actually require those.
 */

#if defined(_MSC_VER)
/* MSVC provides these declarations in stdlib.h and intrin.h */

#define rotr8(v, c) _rotr8((v), (c) &7)
#define rotl8(v, c) _rotl8((v), (c) &7)
#define rotr16(v, c) _rotr16((v), (c) &15)
#define rotl16(v, c) _rotl16((v), (c) &15)
#define rotr32(v, c) _rotr32((v), (c) &31)
#define rotl32(v, c) _rotl32((v), (c) &31)
#define rotr64(v, c) _rotr64((v), (c) &63)
#define rotl64(v, c) _rotl64((v), (c) &63)

#define DECLARE_ROTATE_(bits, type)

#else
/* Many modern compilers recognize the idiom here as equivalent to rotr/rotl,
 * and emit a single instruction.
 */
#define DECLARE_ROTATE_(bits, type)                    \
    static inline type rotl##bits(const type v, int c) \
    {                                                  \
        const int mask = (bits) -1;                    \
        c &= mask;                                     \
                                                       \
        return (v << c) | (v >> (-c & mask));          \
    } /* rotl() */                                     \
                                                       \
    static inline type rotr##bits(const type v, int c) \
    {                                                  \
        const int mask = (bits) -1;                    \
        c &= mask;                                     \
                                                       \
        return (v >> c) | (v << (-c & mask));          \
    } /* rotr() */
#endif

#define DECLARE_ROTATE(bits) DECLARE_ROTATE_(bits, uint##bits##_t)
DECLARE_ROTATE(64);
DECLARE_ROTATE(32);
DECLARE_ROTATE(16);
DECLARE_ROTATE(8);

#endif /* TIMEOUT_BITOPS_H */
