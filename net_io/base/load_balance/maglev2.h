#ifndef LT_NET_BASE_LB_MAGLEVEV2_H_
#define LT_NET_BASE_LB_MAGLEVEV2_H_

/* 
 * HuanGong:
 * this is a modified version of facebook's katran implement
 * all Copyright And License guarantee flow orginal declare
 *
 * Copyright (C) 2018-present, Facebook, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include <cstdint>
#include <memory>
#include <vector>

namespace lt {
namespace net {
namespace lb {

#ifndef MAGLEVE_CH_RINGSIZE
#define MAGLEVE_CH_RINGSIZE 65537
#endif

/**
 * struct which describes backend, each backend would have unique number,
 * weight (the measurment of how often we would see this endpoint
 * on CH ring) and hash value, which will be used as a seed value
 * (it should be unique value per endpoint for CH to work as expected)
 *
 *  0 < num < uint32_t_MAX/2 and unique
 *  0 < weight < uint32_t_MAX/2 //NOTE: v1 need sum(weight) of all node about ring_size
 *  unique(hash) in all endpoints
 */
struct Endpoint {
  uint32_t num;
  uint32_t weight;
  uint64_t hash;
};

/**
 * ConsistentHash implements interface, which is used by CHFactory class to
 * generate hash ring
 */
using LookupTable = std::vector<int>;
using EndpointList = std::vector<Endpoint>;

class MaglevV2 {
 public:
  /**
   * @param std::vector<Endpoints>& endpoints, which will be used for CH
   * @param uint32_t ring_size size of the CH ring
   * @return std::vector<int> vector, which describe CH ring.
   */
   static LookupTable GenerateHashRing(EndpointList endpoints,
                                       const uint32_t ring_size = MAGLEVE_CH_RINGSIZE);

   static uint32_t RingSize() {return MAGLEVE_CH_RINGSIZE;}
};

}}}//end namespace lt::net::lb 

#endif //LT_NET_BASE_LB_MAGLEVEV2_H_
