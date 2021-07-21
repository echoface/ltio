#ifndef LT_THIRDPARTY_HASH_SHA1_H_H_
#define LT_THIRDPARTY_HASH_SHA1_H_H_

#include <stddef.h>

#include <array>
#include <string>

namespace base {

// Length in bytes of a SHA-1 hash.
constexpr size_t kSHA1Length = 20;

using Digest = std::array<uint8_t, kSHA1Length>;

// Usage example:
//
// SecureHashAlgorithm sha;
// while(there is data to hash)
//   sha.Update(moredata, size of data);
// sha.Final();
// memcpy(somewhere, sha.Digest(), 20);
//
// to reuse the instance of sha, call sha.Init();
class SecureHashAlgorithm {
public:
  SecureHashAlgorithm() { Init(); }

  void Init();
  void Update(const void* data, size_t nbytes);
  void Final();

  // 20 bytes of message digest.
  const unsigned char* Digest() const {
    return reinterpret_cast<const unsigned char*>(H);
  }

private:
  void Pad();
  void Process();

  uint32_t A, B, C, D, E;

  uint32_t H[5];

  union {
    uint32_t W[80];
    uint8_t M[64];
  };

  uint32_t cursor;
  uint64_t l;
};

// Returns the computed SHA1 of the input string |str|.
Digest SHA1HashString(const std::string& str);

}  // namespace base

#endif  // LT_BASE_HASH_SHA1_H_H_
