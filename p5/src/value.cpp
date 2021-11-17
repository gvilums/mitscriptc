#include <sys/types.h>
#include <cassert>
#include <cstdint>
#include <cstring>
#include <memory>

#include "value.h"

namespace runtime {

ValueKind value_get_kind(Value val) {
    return static_cast<ValueKind>(val & TAG_MASK);
}

bool value_get_bool(Value val) {
    return static_cast<bool>(val >> 4);
}

int value_get_int32(Value val) {
    return static_cast<int32_t>(val >> 4);
}

Value from_bool(bool b) {
    return (static_cast<uint64_t>(b) << 4) | static_cast<uint64_t>(ValueKind::Bool);
}

Value from_int32(int32_t i) {
    return (static_cast<uint64_t>(i) << 4) | static_cast<uint64_t>(ValueKind::Int);
}

Value from_std_string(const std::string& str) {
    // heap allocated
    if (str.size() > 7) {
        String* str_ptr = get_alloc().alloc_string(str.size());
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

Value from_ref(Value* ref) {
    return reinterpret_cast<uint64_t>(ref) | static_cast<uint64_t>(ValueKind::Reference);
}

Value from_string_ptr(String* str) {
    return reinterpret_cast<uint64_t>(str) | static_cast<uint64_t>(ValueKind::HeapString);
}

Value from_record_ptr(Record* rec) {
    return reinterpret_cast<uint64_t>(rec) | static_cast<uint64_t>(ValueKind::Record);
}

Value from_closure_ptr(Closure* closure) {
    return reinterpret_cast<uint64_t>(closure) | static_cast<uint64_t>(ValueKind::Closure);
}

Value value_add(Value lhs, Value rhs) {
    auto lhs_kind = value_get_kind(lhs);
    auto rhs_kind = value_get_kind(rhs);
    return 0;
}

Value value_add_int32(Value lhs, Value rhs) {
    return from_int32(value_get_int32(lhs) + value_get_int32(rhs));
}

// below assume integer values
Value value_sub(Value lhs, Value rhs) {
    return from_int32(value_get_int32(lhs) - value_get_int32(rhs));
}

Value value_mul(Value lhs, Value rhs) {
    return from_int32(value_get_int32(lhs) * value_get_int32(rhs));
}

Value value_div(Value lhs, Value rhs) {
    int32_t denom = value_get_int32(rhs);
    if (denom == 0) {
        std::exit(1);
    }
    return from_int32(value_get_int32(lhs) / denom);
}

Value value_to_string(Value val) {
    auto kind = value_get_kind(val);
    if (kind == ValueKind::InlineString || kind == ValueKind::HeapString) {
        return val;
    }
    if (kind == ValueKind::Bool) {
        if (value_get_bool(val)) {
            return get_alloc().true_string;
        } else {
            return get_alloc().false_string;
        }
    }
    return 0;
}

AllocationContext::AllocationContext() {
    // TODO
}

auto AllocationContext::alloc_ref() -> Value* {
    HeapObject* obj = reinterpret_cast<HeapObject*>(
        this->allocate(sizeof(HeapObject) + sizeof(Value)));
    obj->type = HeapObject::REF;
    return reinterpret_cast<Value*>(&obj->data);
}

auto AllocationContext::alloc_string(size_t length) -> String* {
    HeapObject* obj = reinterpret_cast<HeapObject*>(
        this->allocate(sizeof(HeapObject) + sizeof(String) + sizeof(char) * length));
    obj->type = HeapObject::STR;
    String* str = reinterpret_cast<String*>(&obj->data);
    str->len = length;
    return str;
}

auto AllocationContext::alloc_record() -> Record* {
    HeapObject* obj = reinterpret_cast<HeapObject*>(
        this->allocate(sizeof(HeapObject) + sizeof(Record)));
    obj->type = HeapObject::REC;
    Record* rec = reinterpret_cast<Record*>(&obj->data);
    new (rec) Record{};
    return rec;
}

auto AllocationContext::alloc_closure(size_t num_free) -> Closure* {
    HeapObject* obj = reinterpret_cast<HeapObject*>(
        this->allocate(sizeof(HeapObject) + sizeof(Closure) + sizeof(Value) * num_free));
    obj->type = HeapObject::CLOSURE;
    Closure* closure = reinterpret_cast<Closure*>(&obj->data);
    closure->n_free_vars = num_free;
    return closure;
}

auto AllocationContext::allocate(size_t n_bytes) -> void* {
    this->total_alloc += n_bytes;
    void* ptr = malloc(n_bytes);
    if (ptr == nullptr) {
        assert(false && "out of memory");
    } else {
        return ptr;
    }
}

void AllocationContext::deallocate(HeapObject* obj) {
    if (obj->type == HeapObject::REF) {
        this->total_alloc -= sizeof(HeapObject) + sizeof(Value);
    } else if (obj->type == HeapObject::STR) {
        this->total_alloc -= sizeof(HeapObject) + sizeof(String) + reinterpret_cast<String*>(&obj->data)->len;
    } else if (obj->type == HeapObject::REC) {
        this->total_alloc -= sizeof(HeapObject) + sizeof(Record);
    } else if (obj->type == HeapObject::CLOSURE) {
        this->total_alloc -=
            sizeof(HeapObject) + sizeof(Closure) + sizeof(Value) * reinterpret_cast<Closure*>(&obj->data)->n_free_vars;
    }
    free(obj);
}

auto get_alloc() -> AllocationContext& {
    static AllocationContext ctx;
    return ctx;
}

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

};  // namespace runtime