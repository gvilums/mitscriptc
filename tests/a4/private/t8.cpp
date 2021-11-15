#include "gc.h"
#include "objects.h"

#include <vector>
#include <iostream>
#include <string>

size_t Object::allocated = 0;
size_t Object::deallocated = 0;

int main() {
	CollectedHeap heap;
  Integer* root;
  for (int k = 0; k < 100; ++k) {
    root = heap.allocate<Integer>(0);
    Integer* last = root;
    for (int i = 1; i < 100; ++i) {
      last->rhs = heap.allocate<Integer>(i);
      last = (Integer*)last->rhs;
    }
    last->rhs = root;
    if (Object::getAlive() != (100 * (k + 1))) {
      std::cerr << "Expected " << (100 * (k + 1)) << " alive objects but got " << Object::getAlive() << std::endl;
      return 1;
    }
    for (int i = 0; i < 100; ++i, root = (Integer*)root->rhs) {
      if (root->val != i) {
        std::cerr << "Expected " << i << " but got " << root->val << std::endl;
        return 1;
      }
    }
  }
  std::vector<Object*> rootset = {root};
  heap.gc(rootset.begin(), rootset.end());
  if (Object::getAlive() != 100) {
    std::cerr << "Expected 100 alive object but got " << Object::getAlive() << std::endl;
    return 1;
  }
  for (int i = 0; i < 100; ++i, root = (Integer*)root->rhs) {
    if (root->val != i) {
      std::cerr << "Expected " << i << " but got " << root->val << std::endl;
      return 1;
    }
  }
  return 0;
}
