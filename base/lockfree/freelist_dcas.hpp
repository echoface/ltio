#include <atomic>

// If you know your target architecture supports a double-wide compare-and-swap
// instruction, you can avoid the reference counting dance and use the naïve algorithm
// with a tag to avoid ABA, while still having it be lock-free. (If the tag of the head
// pointer is incremented each time it's changed, then ABA can't happen because all
// head values appear unique even if an element is re-added.)

template <typename N>
struct FreeListNode
{
    FreeListNode() : freeListNext(nullptr) { }

    std::atomic<N*> freeListNext;
};

// N must inherit FreeListNode or have the same fields (and initialization)
template<typename N>
struct FreeList {
    FreeList()
        : head(HeadPtr()) {
        assert(head.is_lock_free() && "Your platform must support DCAS for this to be lock-free");
    }

    inline void add(N* node) {
        HeadPtr currentHead = head.load(std::memory_order_relaxed);
        HeadPtr newHead = { node };
        do {
            newHead.tag = currentHead.tag + 1;
            node->freeListNext.store(currentHead.ptr, std::memory_order_relaxed);
        } while (!head.compare_exchange_weak(currentHead, newHead,
                                             std::memory_order_release,
                                             std::memory_order_relaxed));
    }

    inline N* try_get()
    {
        HeadPtr currentHead = head.load(std::memory_order_acquire);
        HeadPtr newHead;
        while (currentHead.ptr != nullptr) {
            newHead.ptr = currentHead.ptr->freeListNext.load(std::memory_order_relaxed);
            newHead.tag = currentHead.tag + 1;
            if (head.compare_exchange_weak(currentHead, newHead,
                                           std::memory_order_release,
                                           std::memory_order_acquire)) {
                break;
            }
        }
        return currentHead.ptr;
    }

    // Useful for traversing the list when there's no contention (e.g. to destroy remaining nodes)
    N* head_unsafe() const { return head.load(std::memory_order_relaxed).ptr; }


private:
    struct HeadPtr {
        N* ptr;
        uintptr_t tag;
    };

    std::atomic<HeadPtr> head;
};

