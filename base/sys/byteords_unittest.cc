#include "base/sys/byteorders.h"

#include <stdint.h>

#include "base/build_config.h"
#include <catch2/catch_test_macros.hpp>

namespace {

#ifndef EXPECT_EQ
#define EXPECT_EQ(l, r) REQUIRE((l) == (r))
#endif

const uint16_t k16BitTestData = 0xaabb;
const uint16_t k16BitSwappedTestData = 0xbbaa;
const uint32_t k32BitTestData = 0xaabbccdd;
const uint32_t k32BitSwappedTestData = 0xddccbbaa;
const uint64_t k64BitTestData = 0xaabbccdd44332211;
const uint64_t k64BitSwappedTestData = 0x11223344ddccbbaa;

}  // namespace

TEST_CASE("byteorder.swap16", "[byteorder swap16 test]") {
  uint16_t swapped = base::ByteSwap(k16BitTestData);
  REQUIRE(k16BitSwappedTestData == swapped);
  uint16_t reswapped = base::ByteSwap(swapped);
  REQUIRE(k16BitTestData == reswapped);
}

TEST_CASE("byteorder.swap32", "[byteorder swap32 test]") {
  uint32_t swapped = base::ByteSwap(k32BitTestData);
  EXPECT_EQ(k32BitSwappedTestData, swapped);
  uint32_t reswapped = base::ByteSwap(swapped);
  EXPECT_EQ(k32BitTestData, reswapped);
}

TEST_CASE("byteorder.swap64", "[byteorder swap64 test]") {
  uint64_t swapped = base::ByteSwap(k64BitTestData);
  EXPECT_EQ(k64BitSwappedTestData, swapped);
  uint64_t reswapped = base::ByteSwap(swapped);
  EXPECT_EQ(k64BitTestData, reswapped);
}

TEST_CASE("byteorder.uintptr", "[byteorder uintptr test]") {
#if defined(ARCH_CPU_64_BITS)
  const uintptr_t test_data = static_cast<uintptr_t>(k64BitTestData);
  const uintptr_t swapped_test_data =
      static_cast<uintptr_t>(k64BitSwappedTestData);
#elif defined(ARCH_CPU_32_BITS)
  const uintptr_t test_data = static_cast<uintptr_t>(k32BitTestData);
  const uintptr_t swapped_test_data =
      static_cast<uintptr_t>(k32BitSwappedTestData);
#else
#error architecture not supported
#endif

  uintptr_t swapped = base::ByteSwapUintPtrT(test_data);
  EXPECT_EQ(swapped_test_data, swapped);
  uintptr_t reswapped = base::ByteSwapUintPtrT(swapped);
  EXPECT_EQ(test_data, reswapped);
}

TEST_CASE("byteorder.tole16", "[byteorder tole16 test]") {
  uint16_t le = base::ByteSwapToLE16(k16BitTestData);
#if defined(ARCH_CPU_LITTLE_ENDIAN)
  EXPECT_EQ(k16BitTestData, le);
#else
  EXPECT_EQ(k16BitSwappedTestData, le);
#endif
}

TEST_CASE("byteorder.tole32", "[byteorder tole32 test]") {
  uint32_t le = base::ByteSwapToLE32(k32BitTestData);
#if defined(ARCH_CPU_LITTLE_ENDIAN)
  EXPECT_EQ(k32BitTestData, le);
#else
  EXPECT_EQ(k32BitSwappedTestData, le);
#endif
}

TEST_CASE("byteorder.tole64", "[byteorder tole64 test]") {
  uint64_t le = base::ByteSwapToLE64(k64BitTestData);
#if defined(ARCH_CPU_LITTLE_ENDIAN)
  EXPECT_EQ(k64BitTestData, le);
#else
  EXPECT_EQ(k64BitSwappedTestData, le);
#endif
}

TEST_CASE("byteorder.tohost16", "[byteorder tohost16 test]") {
  uint16_t h = base::NetToHost16(k16BitTestData);
#if defined(ARCH_CPU_LITTLE_ENDIAN)
  EXPECT_EQ(k16BitSwappedTestData, h);
#else
  EXPECT_EQ(k16BitTestData, h);
#endif
}

TEST_CASE("byteorder.tohost32", "[byteorder tohost32 test]") {
  uint32_t h = base::NetToHost32(k32BitTestData);
#if defined(ARCH_CPU_LITTLE_ENDIAN)
  EXPECT_EQ(k32BitSwappedTestData, h);
#else
  EXPECT_EQ(k32BitTestData, h);
#endif
}

TEST_CASE("byteorder.tohost64", "[byteorder tohost64 test]") {
  uint64_t h = base::NetToHost64(k64BitTestData);
#if defined(ARCH_CPU_LITTLE_ENDIAN)
  EXPECT_EQ(k64BitSwappedTestData, h);
#else
  EXPECT_EQ(k64BitTestData, h);
#endif
}

TEST_CASE("byteorder.tonet16", "[byteorder tonet16 test]") {
  uint16_t n = base::HostToNet16(k16BitTestData);
#if defined(ARCH_CPU_LITTLE_ENDIAN)
  EXPECT_EQ(k16BitSwappedTestData, n);
#else
  EXPECT_EQ(k16BitTestData, n);
#endif
}

TEST_CASE("byteorder.tonet32", "[byteorder tonet32 test]") {
  uint32_t n = base::HostToNet32(k32BitTestData);
#if defined(ARCH_CPU_LITTLE_ENDIAN)
  EXPECT_EQ(k32BitSwappedTestData, n);
#else
  EXPECT_EQ(k32BitTestData, n);
#endif
}

TEST_CASE("byteorder.tonet64", "[byteorder tonet64 test]") {
  uint64_t n = base::HostToNet64(k64BitTestData);
#if defined(ARCH_CPU_LITTLE_ENDIAN)
  EXPECT_EQ(k64BitSwappedTestData, n);
#else
  EXPECT_EQ(k64BitTestData, n);
#endif
}
