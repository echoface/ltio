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

#include "base/base_micro.h"

namespace base {

template<class T>
class LinkedNode {
public:
  LinkedNode() : pre_(nullptr), next_(nullptr) {}
  LinkedNode(LinkedNode<T>* pre, LinkedNode<T>* next)
      : pre_(pre), next_(next) {}

  LinkedNode(LinkedNode<T>&& rhs) {
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

  bool Attatched() const {
    return pre_ != nullptr && next_ != nullptr;
  }

  // Insert |this| into the linked list, before |e|.
  void InsertBefore(LinkedNode<T>* e) {
    this->next_ = e;
    this->pre_ = e->pre_;
    e->pre_->next_ = this;
    e->pre_ = this;
  }

  // Insert |this| into the linked list, after |e|.
  void InsertAfter(LinkedNode<T>* e) {
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

  LinkedNode<T>* previous() const {
    return pre_;
  }

  LinkedNode<T>* next() const {
    return next_;
  }

  const T* value() const {
    return static_cast<const T*>(this);
  }

  T* value() {
    return static_cast<T*>(this);
  }
private:
  LinkedNode<T>* pre_ = nullptr;
  LinkedNode<T>* next_ = nullptr;
  DISALLOW_COPY_AND_ASSIGN(LinkedNode);
};

template <typename T>
class LinkedList {
 public:
  // The "root" node is self-referential, and forms the basis of a circular
  // list (root_.next() will point back to the start of the list,
  // and root_->previous() wraps around to the end of the list).
  LinkedList() : root_(&root_, &root_) {}

  // Appends |e| to the end of the linked list.
  void Append(LinkedNode<T>* e) {
    e->InsertBefore(&root_);
  }

  LinkedNode<T>* head() const {
    return root_.next();
  }

  LinkedNode<T>* tail() const {
    return root_.previous();
  }

  const LinkedNode<T>* end() const {
    return &root_;
  }

  bool empty() const { return head() == end(); }

 private:
  LinkedNode<T> root_;
  DISALLOW_COPY_AND_ASSIGN(LinkedList);
};

}
#endif
