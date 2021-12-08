#include "gc.h"
#include "objects.h"

#include <vector>
#include <iostream>
#include <string>

size_t Object::allocated = 0;
size_t Object::deallocated = 0;

int main() {
	CollectedHeap heap;
  Integer* node = heap.allocate<Integer>(-1);
  std::vector<Integer*> rootset;
  for (int k = 0; k < 100; ++k) {
    Integer* root = heap.allocate<Integer>(k);
    root->lhs = root->rhs = node;
    rootset.push_back(root);
  }
  for (int k = 0; k < 100; ++k) {
    heap.gc(rootset.begin(), rootset.end() - k);
    if (Object::getAlive() != 101 - k) {
      std::cerr << "Expected " << (101 - k) << " alive objects but got " << Object::getAlive() << std::endl;
      return 1;
    }
    for (int i = 0; i < 100 - k; ++i) {
      if (rootset[i]->val != i) {
        std::cerr << "Expected " << i << " but got " << rootset[i]->val << std::endl;
        return 1;
      }
    }
    if (node->val != -1) {
      std::cerr << "Expected -1 but got " << node->val << std::endl;
    }
  }
  {
    std::vector<Object*> rootset;
    heap.gc(rootset.begin(), rootset.end());
  }
  if (Object::getAlive() != 0) {
    std::cerr << "Expected 0 alive object but got " << Object::getAlive() << std::endl;
    return 1;
  }
  return 0;
}
