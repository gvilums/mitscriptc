#include <sys/types.h>
#include <cstdint>
#include <cstring>
#include <memory>

#include "value.h"

namespace runtime {

std::size_t ValueHash::operator()(const Value& val) const noexcept {
    ValueKind tag = static_cast<ValueKind>(val & TAG_MASK);
    if (tag == ValueKind::HeapString) {
        String* str = reinterpret_cast<String*>(val & DATA_MASK);
        const size_t l = str->len;
        const size_t p = 1000000007;
        size_t current_p = p;
        size_t output = 0;
        for (size_t i = 0; i < l; ++i) {
            output += str->data[i] * current_p;
            current_p *= p;
        }
        return output;
    }
    return val;
}

ValueKind value_get_kind(Value val) {
    return static_cast<ValueKind>(val & TAG_MASK);
}

Value from_bool(bool b) {
    return (static_cast<uint64_t>(b) << 4) | static_cast<uint64_t>(ValueKind::Bool);
}

Value from_int32(int32_t i) {
    return (static_cast<uint64_t>(i) << 4) | static_cast<uint64_t>(ValueKind::Int);
}

Value from_std_string(const std::string &str) {
    // heap allocated
    if (str.size() > 7) {
        String* str_ptr = alloc_string(str.size());
        std::memcpy(&str_ptr->data, str.data(), str.size());
        return from_string_ptr(str_ptr);
    } else {
        std::uint64_t str_data{0};
        std::memcpy(&str_data, str.data(), str.size());
        str_data <<= 8;
        str_data |= (str.size() << 4);
        str_data |= static_cast<uint64_t>(ValueKind::InlineString);
        return str_data;
    }
}

Value from_ref(Value *ref) {
    return reinterpret_cast<uint64_t>(ref) | static_cast<uint64_t>(ValueKind::Reference);
}

Value from_string_ptr(String *str) {
    return reinterpret_cast<uint64_t>(str) | static_cast<uint64_t>(ValueKind::HeapString);
}

Value from_record_ptr(Record *rec) {
    return reinterpret_cast<uint64_t>(rec) | static_cast<uint64_t>(ValueKind::Record);
}
    
Value from_closure_ptr(Closure *closure) {
    return reinterpret_cast<uint64_t>(closure) | static_cast<uint64_t>(ValueKind::Closure);
}

Value value_add(Value lhs, Value rhs) {
    
}

// below assume integer values
Value value_sub(Value lhs, Value rhs) {
    return (((lhs >> 4) - (rhs >> 4)) << 4) | static_cast<uint64_t>(ValueKind::Int);
}

Value value_mul(Value lhs, Value rhs) {
    return (((lhs >> 4) * (rhs >> 4)) << 4) | static_cast<uint64_t>(ValueKind::Int);
}

Value value_div(Value lhs, Value rhs) {
    if (rhs >> 4 == 0) {
        std::exit(1);
    }
    return (((lhs >> 4) / (rhs >> 4)) << 4) | static_cast<uint64_t>(ValueKind::Int);
}


Value* alloc_ref() {
    HeapObject* obj = reinterpret_cast<HeapObject*>(malloc(sizeof(HeapObject) + sizeof(Value)));
    obj->type = HeapObject::REF;
    return reinterpret_cast<Value*>(&obj->data);
}

String* alloc_string(size_t length) {
    HeapObject* obj = reinterpret_cast<HeapObject*>(malloc(sizeof(HeapObject) + sizeof(String) + sizeof(char) * length));
    obj->type = HeapObject::STR;
    String* str = reinterpret_cast<String*>(&obj->data);
    str->len = length;
    return str;
}

Record* alloc_record() {
    HeapObject* obj = reinterpret_cast<HeapObject*>(malloc(sizeof(HeapObject) + sizeof(Record)));
    obj->type = HeapObject::REC;
    Record* rec = reinterpret_cast<Record*>(&obj->data);
    new (rec) Record{};
    return rec;
}

Closure* alloc_closure(size_t num_free) {
    HeapObject* obj = reinterpret_cast<HeapObject*>(malloc(sizeof(HeapObject) + sizeof(Closure) + sizeof(Value) * num_free));
    obj->type = HeapObject::CLOSURE;
    Closure* closure = reinterpret_cast<Closure*>(&obj->data);
    return closure;
}

};