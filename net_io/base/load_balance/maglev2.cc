/*
 * Copyright 2021 <name of copyright holder>
 * Author: Huan.Gong <gonghuan.dev@gmail.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "maglev2.h"

#include "murmurhash/MurmurHash3.h"

#define MAGLEVE_USE_128BIT_HASH

namespace {
constexpr uint32_t kHashSeed0 = 0;
constexpr uint32_t kHashSeed1 = 2307;
constexpr uint32_t kHashSeed2 = 42;
constexpr uint32_t kHashSeed3 = 2718281828;

using Endpoint = lt::net::lb::Endpoint;
using IndexIdList = std::vector<uint32_t>;

void GenMaglevPermutation(IndexIdList& permutation,
                          const Endpoint& endpoint,
                          const uint32_t ep_index,
                          const uint32_t ring_size) {
#ifdef MAGLEVE_USE_128BIT_HASH
  uint64_t hash_result[2] = {0};  //[offsethash << 64 | skip_hash]
  MurmurHash3_x64_128(&endpoint.hash, sizeof(endpoint.hash), kHashSeed3,
                      hash_result);
  auto offset = hash_result[0] % ring_size;
  auto skip = (hash_result[1] % (ring_size - 1)) + 1;
#else
  uint32_t offset_hash = 0;
  MurmurHash3_x86_32(&endpoint.hash, sizeof(endpoint.hash), kHashSeed2,
                     &offset_hash);
  auto offset = offset_hash % ring_size;
  uint32_t skip_hash = 0;
  MurmurHash3_x86_32(&endpoint.hash, sizeof(endpoint.hash), kHashSeed3,
                     &skip_hash);
  auto skip = (skip_hash % (ring_size - 1)) + 1;
#endif
  permutation[2 * ep_index] = offset;
  permutation[2 * ep_index + 1] = skip;
}

}  // namespace

namespace lt {
namespace net {
namespace lb {

LookupTable MaglevV2::GenerateHashRing(EndpointList endpoints,
                                       const uint32_t ring_size) {
  if (endpoints.size() == 0) {
    return LookupTable(ring_size, -1);
  } else if (endpoints.size() == 1) {
    return LookupTable(ring_size, endpoints.front().num);
  }

  auto max_weight = 0;
  for (const auto& endpoint : endpoints) {
    if (endpoint.weight > max_weight) {
      max_weight = endpoint.weight;
    }
  }

  std::vector<int> result(ring_size, -1);

  uint32_t runs = 0;
  std::vector<uint32_t> permutation(endpoints.size() * 2, 0);

  std::vector<uint32_t> next(endpoints.size(), 0);
  std::vector<uint32_t> cum_weight(endpoints.size(), 0);

  for (int i = 0; i < endpoints.size(); i++) {
    GenMaglevPermutation(permutation, endpoints[i], i, ring_size);
  }

  for (;;) {
    for (int i = 0; i < endpoints.size(); i++) {
      cum_weight[i] += endpoints[i].weight;
      if (cum_weight[i] >= max_weight) {
        cum_weight[i] -= max_weight;
        auto offset = permutation[2 * i];
        auto skip = permutation[2 * i + 1];
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
    }
  }

  // NOTE: here should not reached
  return LookupTable(ring_size, -1);
}

}  // namespace lb
}  // namespace net
}  // namespace lt
