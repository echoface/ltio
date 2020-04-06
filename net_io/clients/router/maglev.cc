#include "maglev.h"
#include "thirdparty/murmurhash/MurmurHash3.h"

namespace lt {
namespace net {

static const uint32_t kHashSeed3 = 2718281828;

void MaglevHelper::genMaglevPermuation(IdList& permutation,
                                    const Endpoint endpoint,
                                    const uint32_t pos, 
                                    const uint32_t ring_size) {
#if 1
  uint64_t hash_result[2] = {0}; //[offsethash << 64 | skip_hash]
  MurmurHash3_x64_128(&endpoint.hash, sizeof(endpoint.hash), kHashSeed3, hash_result);
  auto offset = hash_result[0] % ring_size;
  auto skip   = (hash_result[1] % (ring_size - 1)) + 1;
#else
  auto offset_hash = MurmurHash3_x64_64(endpoint.hash, kHashSeed2, kHashSeed0);
  auto offset = offset_hash % ring_size;
  auto skip_hash = MurmurHash3_x64_64(endpoint.hash, kHashSeed3, kHashSeed1);
  auto skip = (skip_hash % (ring_size - 1)) + 1;
#endif
  permutation[2 * pos] = offset;
  permutation[2 * pos + 1] = skip;
}

LookupTable MaglevHelper::GenerateMaglevHash(NodeList endpoints,
                                             const uint32_t ring_size) {
  LookupTable result(ring_size, -1);

  switch(endpoints.size()) {
    case 0:
      return result;
    case 1:
      return LookupTable(ring_size, endpoints[0].num);
    default:
      break;
  }

  uint32_t runs = 0;
  IdList permutation(endpoints.size() * 2, 0);
  IdList next(endpoints.size(), 0);

  for (size_t i = 0; i < endpoints.size(); i++) {
    genMaglevPermuation(permutation, endpoints[i], i, ring_size);
  }

  for (;;) {
    for (size_t i = 0; i < endpoints.size(); i++) {
      auto offset = permutation[2 * i];
      auto skip = permutation[2 * i + 1];
      // our realization of "weights" for maglev's hash.
      for (size_t j = 0; j < endpoints[i].weight; j++) {
        auto cur = (offset + next[i] * skip) % ring_size;
        while (result[cur] >= 0) {
          next[i] += 1;
          cur = (offset + next[i] * skip) % ring_size;
        }
        result[cur] = endpoints[i].num;
        next[i] += 1;
        runs++;
        if (runs == ring_size) {
          return result;
        }
      }
      endpoints[i].weight = 1;
    }
  }
  return result;
}

}} //end lt::net
