#pragma once

#include "gc.h"

#include <string>

class Object : public Collectable {
public:
  Object() {
    allocated++;
  }

  virtual ~Object() {
    deallocated++;
  }

  static int64_t getAlive() {
    return (allocated - deallocated);
  }

  Object* lhs = nullptr;
  Object* rhs = nullptr;

protected:
  virtual void follow(CollectedHeap& heap) override {
    if (lhs) heap.markSuccessors(lhs);
    if (rhs) heap.markSuccessors(rhs);
  };

private: 
  static size_t allocated;
  static size_t deallocated;
};

class Integer : public Object {
public:
  Integer(int val) : val(val) {}

  ~Integer() {
    val = -183534883;
  }

  int val;
};

class String : public Object {
public:
  String(std::string val) : val(val) {}

  ~String() {
    val = "jijewriower89u33ijioejidjoder";
  }

  std::string val;
};
