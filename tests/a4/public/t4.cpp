#include "gc.h"
#include "objects.h"

#include <vector>
#include <iostream>
#include <string>

size_t Object::allocated = 0;
size_t Object::deallocated = 0;

int main() {
	CollectedHeap heap;
  std::vector<Integer*> rootset;
  for (int k = 0; k < 100; ++k) {
    Integer* root = heap.allocate<Integer>(0);
    Integer* last = root;
    for (int i = 1; i < 4; ++i) {
      last->rhs = heap.allocate<Integer>(i);
      last = (Integer*)last->rhs;
    }
    last->rhs = root;
    rootset.push_back(root);
  }
  heap.gc(rootset.begin(), rootset.end());
  if (Object::getAlive() != 400) {
    std::cerr << "Expected 400 alive objects but got " << Object::getAlive() << std::endl;
    return 1;
  }
  for (int k = 0; k < 100; ++k) {
    Integer* root = rootset[k];
    for (int i = 0; i < 4; ++i, root = (Integer*)root->rhs) {
      if (root->val != i) {
        std::cerr << "Expected " << i << " but got " << root->val << std::endl;
        return 1;
      }
    }
  }
  rootset = {};
  heap.gc(rootset.begin(), rootset.end());
  if (Object::getAlive() != 0) {
    std::cerr << "Expected 0 alive object but got " << Object::getAlive() << std::endl;
    return 1;
  }
  return 0;
}
