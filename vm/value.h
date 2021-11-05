#pragma once

#include <exception>
#include <map>
#include <string>
#include <unordered_map>
#include <parallel_hashmap/phmap.h>
#include <utility>

#include <iostream>

// #include "../gc/gc.h"
#include "allocator.h"
#include "types.h"

struct String {
    size_t size;
    char data[];
    
    String(size_t len) : size{len} {}
};

template<>
struct std::hash<Allocation::TrackedString> {
    std::size_t operator()(const Allocation::TrackedString& str) const noexcept {
        const size_t l = str.size();
        const size_t p = 1000000007;
        size_t current_p = p;
        size_t output = 0;
        for (size_t i = 0; i < l; ++i) {
            output += str[i] * current_p;
            current_p *= p;
        }
        return output;
    }
};

namespace VM {

class VirtualMachine;

class Value;
class HeapObject;

using Allocation::TrackedString;

using TrackedMap = phmap::flat_hash_map<
    TrackedString,
    Value,
    std::hash<TrackedString>,
    std::equal_to<TrackedString>,
    Allocation::TrackingAlloc<std::pair<const TrackedString, Value>>>;

enum class FnType { DEFAULT,
                    PRINT,
                    INPUT,
                    INTCAST };


struct Record;

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
    enum ValueTag {
        NONE,
        NUM,
        BOOL,
        STRING_PTR,
        RECORD_PTR,
        CLOSURE_PTR,
        VALUE_PTR,
        FN_PTR,
        USIZE
    };

   private:
    ValueTag tag{NONE};
    union {
        None none;
        int num;
        bool boolean;
        String* string;
        Record* record;
        Closure* closure;
        Value* value;
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
    Value(String* ptr)
        : tag{STRING_PTR}, string{ptr} {}
    Value(Record* ptr)
        : tag{RECORD_PTR}, record{ptr} {}
    Value(Closure* ptr)
        : tag{CLOSURE_PTR}, closure{ptr} {}
    Value(Value* ptr)
        : tag{VALUE_PTR}, value{ptr} {}
    Value(struct Function* ptr)
        : tag{FN_PTR}, fnptr{ptr} {}
    Value(size_t u)
        : tag{USIZE}, usize{u} {}
    
    static auto from_constant(Constant c, VirtualMachine& vm) -> Value;

    // Value(const Value& other);
    // Value(Value&& other) noexcept;

    // auto operator=(const Value& other) -> Value&;
    // auto operator=(Value&& other) noexcept -> Value&;

    friend auto operator+(const Value& lhs, const Value& rhs) -> Value;
    friend auto operator==(const Value& lhs, const Value& rhs) -> bool;
    friend auto operator>=(Value const& lhs, Value const& rhs) -> bool;
    friend auto operator>(Value const& lhs, Value const& rhs) -> bool;

    [[nodiscard]] auto to_string() const -> TrackedString;

    void trace();

    auto get_tag() -> ValueTag;
    auto get_bool() -> bool;
    auto get_int() -> int;
    auto get_string() -> String&;
    auto get_record() -> Record&;
    auto get_closure() -> Closure&;
    auto get_val_ref() -> Value&;
    auto get_fnptr() -> struct Function*;
    auto get_usize() -> size_t;
};

struct Record {
    TrackedMap fields;
};


class HeapObject {
   public:
    enum HeapTag { VALUE,
                   RECORD,
                   CLOSURE };

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
    HeapObject(Value v)
        : tag{VALUE} {
        // std::cout << "allocated value " << v.to_string() <<std::endl;
        ::new (&this->val) auto(std::move(v));
    }
    HeapObject(Record r)
        : tag{RECORD} {
        // std::cout << "allocated record" << std::endl;
        ::new (&this->rec) auto(std::move(r));
    }
    HeapObject(Closure c)
        : tag{CLOSURE} {
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