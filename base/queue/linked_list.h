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

#ifndef _LT_LINKED_LIST_IMPL_H_
#define _LT_LINKED_LIST_IMPL_H_

#include "base/lt_macro.h"

namespace base {

template <typename T>
class LinkedList {
public:
  class Node {
    public:
      Node() : pre_(nullptr), next_(nullptr) {}

      Node(Node* pre, Node* next)
        : pre_(pre), next_(next) {}

      Node(Node&& rhs) {
        next_ = rhs.next_;
        rhs.next_ = nullptr;
        pre_ = rhs.pre_;
        rhs.pre_ = nullptr;

        // If the node belongs to a list,
        // next_ and pre_ are both non-null.
        // Otherwise, they are both null.
        if (next_) {
          next_->pre_ = this;
          pre_->next_ = this;
        }
      }

      bool Attatched() const { return pre_ != nullptr && next_ != nullptr; }

    private:
      friend class LinkedList<T>;
      // Insert |this| into the linked list, before |e|.
      void InsertBefore(Node* e) {
        this->next_ = e;
        this->pre_ = e->pre_;
        e->pre_->next_ = this;
        e->pre_ = this;
      }

      // Insert |this| into the linked list, after |e|.
      void InsertAfter(Node* e) {
        this->next_ = e->next_;
        this->pre_ = e;
        e->next_->pre_ = this;
        e->next_ = this;
      }

      // Remove |this| from the linked list.
      void RemoveFromList() {
        this->pre_->next_ = this->next_;
        this->next_->pre_ = this->pre_;
        // next() and previous() return non-null if and only this node is not in any
        // list.
        this->next_ = nullptr;
        this->pre_ = nullptr;
      }

      Node* previous() const { return pre_; }

      Node* next() const { return next_; }

      T* value() { return static_cast<T*>(this); }

      const T* value() const { return static_cast<const T*>(this); }

    private:
      Node* pre_ = nullptr;
      Node* next_ = nullptr;
      DISALLOW_COPY_AND_ASSIGN(Node);
  };
public:
  // The "root" node is self-referential, and forms the basis of a circular
  // list (root_.next() will point back to the start of the list,
  // and root_->previous() wraps around to the end of the list).
  LinkedList() : root_(&root_, &root_) {}

  // Appends |e| to the end of the linked list.
  void Append(T* e) {
    size_++;
    e->InsertBefore(&root_);
  }

  void Remove(T* e) {
    size_--;
    e->RemoveFromList();
  }

  T* First() { return empty() ? nullptr : head()->value();}

  T* PopAndRemove() {
    T* e = First();
    if (e) {
      e->RemoveFromList();
    }
    return e;
  }

  bool empty() const { return head() == end(); }

  Node* head() const { return root_.next(); }

  Node* tail() const { return root_.previous(); }

  const Node* end() const { return &root_; }

  const size_t size() const {return size_; }
private:
  Node root_;
  size_t size_ = 0;
  DISALLOW_COPY_AND_ASSIGN(LinkedList);
};

}  // namespace base
#endif
