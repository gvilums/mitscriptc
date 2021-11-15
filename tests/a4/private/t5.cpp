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
  for (int i = 0; i < 1000; ++i) {
    integers[i] = heap.allocate<Integer>(i);
    strings[i] = heap.allocate<String>(std::to_string(i));
  }
  if (Object::getAlive() != 2000) {
    std::cerr << "Expected 2000 alive objects but got " << Object::getAlive() << std::endl;
    return 1;
  }
  heap.gc(integers.begin(), integers.end());
  if (Object::getAlive() != 1000) {
    std::cerr << "Expected 1000 alive objects but got " << Object::getAlive() << std::endl;
    return 1;
  }
  for (int i = 0; i < 1000; ++i) {
    if (integers[i]->val != i) {
      std::cerr << "Expected " << i << " but got " << integers[i]->val << std::endl;
      return 1;
    }
  }
  return 0;
}
