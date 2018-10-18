#include "bitmap_posting_list.h"

namespace component {

void BitMapPostingList::SetBit(uint64_t idx) {
	CHECK(idx < ids_bitmap_.size() * 64);
	int64_t bytes = idx / 64;
	int16_t bits  = idx % 64;
	ids_bitmap_[bytes] |= (1ul << (63 - bits));
}

void BitMapPostingList::ClearBit(uint64_t idx) {
	CHECK(idx < ids_bitmap_.size() * 64);
	int64_t bytes = idx / 64;
	int16_t bits  = idx % 64;
	ids_bitmap_[bytes] &= ~(1ul << (63 - bits));
}

bool BitMapPostingList::IsBitSet(uint64_t idx) const {
	CHECK(idx < ids_bitmap_.size() * 64);
  return (ids_bitmap_[idx/64] & (1ul << (63 - (idx%64)))) != 0;
}

void BitMapPostingList::Intersect(const BitMapPostingList* other) {
  uint64_t i = 0;
  for (;i < ids_bitmap_.size() && i < other->ids_bitmap_.size(); i++) {
    ids_bitmap_[i] &= other->ids_bitmap_[i];
  }
  if (i < ids_bitmap_.size()) {
    memset(&ids_bitmap_[i], 0, ids_bitmap_.size() - i);
  }
}

void BitMapPostingList::Substract(const BitMapPostingList* other) {
  uint64_t i = 0;
  for (;i < ids_bitmap_.size() && i < other->ids_bitmap_.size(); i++) {
    ids_bitmap_[i] &= ~(other->ids_bitmap_[i]);
  }
}

} //end namespace