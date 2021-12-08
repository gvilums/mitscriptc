#include "gc.h"
#include "objects.h"

#include <vector>
#include <iostream>
#include <string>

size_t Object::allocated = 0;
size_t Object::deallocated = 0;

int main() {
	CollectedHeap heap;
  Integer* root = heap.allocate<Integer>(0);
  Integer* last = root;
  for (int i = 1; i < 1000; ++i) {
    last->rhs = heap.allocate<Integer>(i);
    last = (Integer*)last->rhs;
  }
  last->rhs = root;
  if (Object::getAlive() != 1000) {
    std::cerr << "Expected 1000 alive objects but got " << Object::getAlive() << std::endl;
    return 1;
  }
  for (int i = 0; i < 1000; ++i, root = (Integer*)root->rhs) {
    if (root->val != i) {
      std::cerr << "Expected " << i << " but got " << root->val << std::endl;
      return 1;
    }
  }
  std::vector<Object*> rootset;
  heap.gc(rootset.begin(), rootset.end());
  if (Object::getAlive() != 0) {
    std::cerr << "Expected 0 alive object but got " << Object::getAlive() << std::endl;
    return 1;
  }
  return 0;
}
