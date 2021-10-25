#pragma once

#include <exception>
#include <map>
#include <string>
#include <unordered_map>
#include <utility>

#include <iostream>

// #include "../gc/gc.h"
#include "types.h"
#include "allocator.h"

namespace VM {
    

class Value;
class HeapObject;

using Allocation::TrackedString;

using TrackedMap = std::map<
    TrackedString,
    Value,
    std::less<>,
    Allocation::TrackingAlloc<std::pair<const TrackedString, Value>>
>;


enum class FnType { DEFAULT,
                    PRINT,
                    INPUT,
                    INTCAST };

struct Record {
    TrackedMap fields;
};


struct Closure {
    using TrackedVec = std::vector<HeapObject*, Allocation::TrackingAlloc<HeapObject*>>;

    FnType type;
    struct Function* fn;
    TrackedVec refs;

    Closure(FnType type, struct Function* fn)
        : type{type}, fn{fn} {}
    Closure(FnType type, struct Function* fn, TrackedVec refs)
        : type{type}, fn{fn}, refs{std::move(refs)} {}

};


class Value {
   public:
    enum ValueTag { NONE,
           NUM,
           BOOL,
           STRING,
           HEAP_REF,
           FN_PTR,
           USIZE };

   private:
    ValueTag tag{NONE};
    union {
        None none;
        int num;
        bool boolean;
        TrackedString str;
        HeapObject* heap_ref;
        struct Function* fnptr;
        size_t usize;
    };

   public:
    Value()
        : none{None{}} {}
    Value(None none_val)
        : none{None{}} {}
    Value(int n)
        : tag{NUM}, num{n} {}
    Value(bool b)
        : tag{BOOL}, boolean{b} {}
    Value(HeapObject* ref)
        : tag{HEAP_REF}, heap_ref{ref} {}
    Value(struct Function* ptr)
        : tag{FN_PTR}, fnptr{ptr} {}
    Value(size_t u)
        : tag{USIZE}, usize{u} {}
    Value(TrackedString s)
        : tag{STRING} {
        ::new (&this->str) auto(std::move(s));
    }
    Value(Constant c);

    Value(const Value& other);
    Value(Value&& other) noexcept;

    ~Value();
    
    void destroy_contents();

    auto operator=(const Value& other) -> Value&;
    auto operator=(Value&& other) noexcept -> Value&;

    friend auto operator+(const Value& lhs, const Value& rhs) -> Value;
    friend auto operator==(const Value& lhs, const Value& rhs) -> bool;
    friend auto operator>=(Value const& lhs, Value const& rhs) -> bool;
    friend auto operator>(Value const& lhs, Value const& rhs) -> bool;

    [[nodiscard]] auto to_string() const -> TrackedString;
    
    void trace();
    
    auto get_tag() -> ValueTag;
    auto get_bool() -> bool;
    auto get_int() -> int;
    auto get_string() -> TrackedString;
    auto get_heap_ref() -> HeapObject*;
    auto get_record() -> Record&;
    auto get_val_ref() -> Value&;
    auto get_closure() -> Closure&;
    auto get_fnptr() -> struct Function*;
    auto get_usize() -> size_t;

};

class VirtualMachine;

class HeapObject {
   public:
    enum HeapTag {VALUE, RECORD, CLOSURE};

   private:
    HeapTag tag;
    union {
        Value val;
        Record rec;
        Closure closure;
    };
    
    bool marked{false};
    HeapObject* next{nullptr};

   public:
    HeapObject(Value v) : tag{VALUE} {
        // std::cout << "allocated value " << v.to_string() <<std::endl;
        ::new (&this->val) auto(std::move(v));
    }
    HeapObject(Record r) : tag{RECORD} {
        // std::cout << "allocated record" << std::endl;
        ::new (&this->rec) auto(std::move(r));
    }
    HeapObject(Closure c) : tag{CLOSURE} {
        // std::cout << "allocated closure of type " << (int)c.type << std::endl;
        ::new (&this->closure) auto(std::move(c));
    }
    
    ~HeapObject() {
        if (this->tag == VALUE) {
            this->val.~Value();
        } else if (this->tag == RECORD) {
            this->rec.~Record();
        } else if (this->tag == CLOSURE) {
            this->closure.~Closure();
        }
    }
    
    void trace();
    
    auto get_value() -> Value&;
    auto get_record() -> Record&;
    auto get_closure() -> Closure&;
    
    friend VirtualMachine;
    friend Value;
    friend auto operator==(const Value& lhs, const Value& rhs) -> bool;
};

};  // namespace VM