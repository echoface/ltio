// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

//------------------------------------------------------------------------------
// * This code is taken from base/sha1, with small changes.
//------------------------------------------------------------------------------

#include "sha1.h"

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#ifdef _MSC_VER

#include <stdlib.h>
#define bswap_32(x) _byteswap_ulong(x)
#define bswap_64(x) _byteswap_uint64(x)

#elif defined(__APPLE__)

// Mac OS X / Darwin features
#include <libkern/OSByteOrder.h>
#define bswap_32(x) OSSwapInt32(x)
#define bswap_64(x) OSSwapInt64(x)

#elif defined(__sun) || defined(sun)

#include <sys/byteorder.h>
#define bswap_32(x) BSWAP_32(x)
#define bswap_64(x) BSWAP_64(x)

#elif defined(__FreeBSD__)

#include <sys/endian.h>
#define bswap_32(x) bswap32(x)
#define bswap_64(x) bswap64(x)

#elif defined(__OpenBSD__)

#include <sys/types.h>
#define bswap_32(x) swap32(x)
#define bswap_64(x) swap64(x)

#elif defined(__NetBSD__)

#include <sys/types.h>
#include <machine/bswap.h>
#if defined(__BSWAP_RENAME) && !defined(__bswap_32)
#define bswap_32(x) bswap32(x)
#define bswap_64(x) bswap64(x)
#endif

#else

#include <byteswap.h>

#endif

namespace base {
namespace {

//------------------------------------------------------------------------------
// Private functions
//------------------------------------------------------------------------------

// Identifier names follow notation in FIPS PUB 180-3, where you'll
// also find a description of the algorithm:
// http://csrc.nist.gov/publications/fips/fips180-3/fips180-3_final.pdf

inline uint32_t f(uint32_t t, uint32_t B, uint32_t C, uint32_t D) {
  if (t < 20) {
    return (B & C) | ((~B) & D);
  } else if (t < 40) {
    return B ^ C ^ D;
  } else if (t < 60) {
    return (B & C) | (B & D) | (C & D);
  } else {
    return B ^ C ^ D;
  }
}

inline uint32_t S(uint32_t n, uint32_t X) {
  return (X << n) | (X >> (32 - n));
}

inline uint32_t K(uint32_t t) {
  if (t < 20) {
    return 0x5a827999;
  } else if (t < 40) {
    return 0x6ed9eba1;
  } else if (t < 60) {
    return 0x8f1bbcdc;
  } else {
    return 0xca62c1d6;
  }
}

// Computes the SHA-1 hash of the |len| bytes in |data| and puts the hash
// in |hash|. |hash| must be kSHA1Length bytes long.
void SHA1HashBytes(const unsigned char* data, size_t len, unsigned char* hash) {
  SecureHashAlgorithm sha;
  sha.Update(data, len);
  sha.Final();

  ::memcpy(hash, sha.Digest(), kSHA1Length);
}

}  // namespace

void SecureHashAlgorithm::Init() {
  A = 0;
  B = 0;
  C = 0;
  D = 0;
  E = 0;
  cursor = 0;
  l = 0;
  H[0] = 0x67452301;
  H[1] = 0xefcdab89;
  H[2] = 0x98badcfe;
  H[3] = 0x10325476;
  H[4] = 0xc3d2e1f0;
}

void SecureHashAlgorithm::Update(const void* data, size_t nbytes) {
  const uint8_t* d = reinterpret_cast<const uint8_t*>(data);
  while (nbytes--) {
    M[cursor++] = *d++;
    if (cursor >= 64)
      Process();
    l += 8;
  }
}

void SecureHashAlgorithm::Final() {
  Pad();
  Process();

  for (size_t t = 0; t < 5; ++t)
    H[t] = bswap_32(H[t]);
}

void SecureHashAlgorithm::Process() {
  uint32_t t;

  // Each a...e corresponds to a section in the FIPS 180-3 algorithm.

  // a.
  //
  // W and M are in a union, so no need to memcpy.
  // memcpy(W, M, sizeof(M));
  for (t = 0; t < 16; ++t)
    W[t] = bswap_32(W[t]);

  // b.
  for (t = 16; t < 80; ++t)
    W[t] = S(1, W[t - 3] ^ W[t - 8] ^ W[t - 14] ^ W[t - 16]);

  // c.
  A = H[0];
  B = H[1];
  C = H[2];
  D = H[3];
  E = H[4];

  // d.
  for (t = 0; t < 80; ++t) {
    uint32_t TEMP = S(5, A) + f(t, B, C, D) + E + W[t] + K(t);
    E = D;
    D = C;
    C = S(30, B);
    B = A;
    A = TEMP;
  }

  // e.
  H[0] += A;
  H[1] += B;
  H[2] += C;
  H[3] += D;
  H[4] += E;

  cursor = 0;
}

void SecureHashAlgorithm::Pad() {
  M[cursor++] = 0x80;

  if (cursor > 64 - 8) {
    // pad out to next block
    while (cursor < 64)
      M[cursor++] = 0;

    Process();
  }

  while (cursor < 64 - 8)
    M[cursor++] = 0;

  M[cursor++] = (l >> 56) & 0xff;
  M[cursor++] = (l >> 48) & 0xff;
  M[cursor++] = (l >> 40) & 0xff;
  M[cursor++] = (l >> 32) & 0xff;
  M[cursor++] = (l >> 24) & 0xff;
  M[cursor++] = (l >> 16) & 0xff;
  M[cursor++] = (l >> 8) & 0xff;
  M[cursor++] = l & 0xff;
}

//------------------------------------------------------------------------------
// Public functions
//------------------------------------------------------------------------------
Digest SHA1HashString(const std::string& str) {
  Digest digest;
  SHA1HashBytes(reinterpret_cast<const unsigned char*>(str.c_str()),
                str.length(),
                reinterpret_cast<unsigned char*>(&digest[0]));
  return digest;
}

}  // namespace base
