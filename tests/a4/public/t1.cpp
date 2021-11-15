#include "gc.h"
#include "objects.h"

#include <vector>
#include <iostream>
#include <string>

size_t Object::allocated = 0;
size_t Object::deallocated = 0;

int main() {
	CollectedHeap heap;
  std::vector<Integer*> integers(1000);
  std::vector<String*> strings(1000);
  std::vector<Object*> rootset;
  for (int i = 0; i < 1000; ++i) {
    integers[i] = heap.allocate<Integer>(i);
    strings[i] = heap.allocate<String>(std::to_string(i));
    rootset.push_back(integers[i]);
    rootset.push_back(strings[i]);
  }
  heap.gc(rootset.begin(), rootset.end());
  if (Object::getAlive() != 2000) {
    std::cerr << "Expected 2000 alive objects but got " << Object::getAlive() << std::endl;
    return 1;
  }
  for (int i = 0; i < 1000; ++i) {
    if (integers[i]->val != i) {
      std::cerr << "Expected " << i << " but got " << integers[i]->val << std::endl;
      return 1;
    }
    if (strings[i]->val != std::to_string(i)) {
      std::cerr << "Expected " << i << " but got " << strings[i]->val << std::endl;
      return 1;
    }
  }
  return 0;
}
