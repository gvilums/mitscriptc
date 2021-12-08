#include "gc.h"
#include "objects.h"

#include <vector>
#include <iostream>
#include <string>

size_t Object::allocated = 0;
size_t Object::deallocated = 0;

Integer* build(int depth, CollectedHeap& heap) {
  Integer* node = heap.allocate<Integer>(depth);
  if (depth == 1) {
    node->lhs = node->rhs = node;
  } else {
    node->lhs = build(depth - 1, heap);
    node->rhs = build(depth - 1, heap);
  }
  return node;
}

bool check(Integer* node, int depth) {
  if (node->val != depth) {
    std::cerr << "Expected " << depth << " but got " << node->val << std::endl;
    return false;
  }
  if (depth == 1) {
    return true;
  }
  return check((Integer*)node->lhs, depth - 1) && check((Integer*)node->rhs, depth - 1);
}

int main() {
	CollectedHeap heap;
  Integer* root = build(5, heap);
  if (Object::getAlive() != 31) {
    std::cerr << "Expected 31 alive objects but got " << Object::getAlive() << std::endl;
    return 1;
  }
  if (!check(root, 5)) {
    return 1;
  }
  Integer* lhsrhs = (Integer*)root->lhs->rhs;
  Integer* rhslhs = (Integer*)root->rhs->lhs;
  std::vector<Object*> rootset = {lhsrhs, rhslhs};
  heap.gc(rootset.begin(), rootset.end());
  if (Object::getAlive() != 14) {
    std::cerr << "Expected 14 alive objects but got " << Object::getAlive() << std::endl;
    return 1;
  }
  if (!check(lhsrhs, 3)) {
    return 1;
  }
  if (!check(rhslhs, 3)) {
    return 1;
  }
  return 0;
}
