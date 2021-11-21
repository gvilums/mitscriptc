#include <sys/types.h>
#include <cassert>
#include <cstdint>
#include <cstring>
#include <memory>
#include <string>
#include <vector>
#include <algorithm>
#include <iostream>

#include "value.h"

namespace runtime {

ValueType value_get_type(Value val) {
    return static_cast<ValueType>(val & TAG_MASK);
}

bool value_get_bool(Value val) {
    return static_cast<bool>(val >> 4);
}

int value_get_int32(Value val) {
    return static_cast<int32_t>(val >> 4);
}

Value* value_get_ref(Value val) {
    return reinterpret_cast<Value*>(val & DATA_MASK);
}

String* value_get_string_ptr(Value val) {
    return reinterpret_cast<String*>(val & DATA_MASK);
}

Record* value_get_record(Value val) {
    return reinterpret_cast<Record*>(val & DATA_MASK);
}

Closure* value_get_closure(Value val) {
    return reinterpret_cast<Closure*>(val & DATA_MASK);
}

Value to_value(bool b) {
    return (static_cast<uint64_t>(b) << 4) | static_cast<uint64_t>(ValueType::Bool);
}

Value to_value(int32_t i) {
    return (static_cast<uint64_t>(i) << 4) | static_cast<uint64_t>(ValueType::Int);
}

Value to_value(ProgramContext* rt, const std::string& str) {
    // heap allocated
    if (str.size() > 7) {
        String* str_ptr = rt->alloc_string(str.size());
        std::memcpy(&str_ptr->data, str.data(), str.size());
        return to_value(str_ptr);
    } else {
        std::uint64_t str_data{0};
        std::memcpy(&str_data, str.data(), str.size());
        str_data <<= 8;
        str_data |= (str.size() << 4);
        str_data |= static_cast<uint64_t>(ValueType::InlineString);
        return str_data;
    }
}

// null terminated string
Value to_value(ProgramContext* rt, const char* str) {
    return to_value(rt, std::string{str});
}

Value to_value(Value* ref) {
    return reinterpret_cast<uint64_t>(ref) | static_cast<uint64_t>(ValueType::Reference);
}

Value to_value(String* str) {
    return reinterpret_cast<uint64_t>(str) | static_cast<uint64_t>(ValueType::HeapString);
}

Value to_value(Record* rec) {
    return reinterpret_cast<uint64_t>(rec) | static_cast<uint64_t>(ValueType::Record);
}

Value to_value(Closure* closure) {
    return reinterpret_cast<uint64_t>(closure) | static_cast<uint64_t>(ValueType::Closure);
}

Value value_add(ProgramContext* rt, Value lhs, Value rhs) {
    auto lhs_kind = value_get_type(lhs);
    auto rhs_kind = value_get_type(rhs);
    if (lhs_kind == ValueType::Int && rhs_kind == ValueType::Int) {
        return value_add_int32(lhs, rhs);
    }
    if (lhs_kind != ValueType::InlineString || lhs_kind != ValueType::HeapString) {
        lhs = value_to_string(rt, lhs);
    }
    if (rhs_kind != ValueType::InlineString || rhs_kind != ValueType::HeapString) {
        rhs = value_to_string(rt, rhs);
    }
    // update types
    lhs_kind = value_get_type(lhs);
    rhs_kind = value_get_type(rhs);
    size_t lhs_size{0};
    size_t rhs_size{0};
    if (lhs_kind == ValueType::InlineString) {
        lhs_size = (lhs >> 4) & 0b1111;
    } else if (lhs_kind == ValueType::HeapString) {
        lhs_size = value_get_string_ptr(lhs)->len;
    } else {
        assert(false);
    }
    if (rhs_kind == ValueType::InlineString) {
        rhs_size = (rhs >> 4) & 0b1111;
    } else if (rhs_kind == ValueType::HeapString) {
        rhs_size = value_get_string_ptr(rhs)->len;
    } else {
        assert(false);
    }
    size_t total_size = lhs_size + rhs_size;
    // allocate new inline string
    if (total_size < 8) {
        // here we now that both lhs and rhs are inline strings
        Value out = lhs;
        // zero out current size
        out &= (~0 << 8) | 0b1111;
        // write new size
        out |= (total_size << 4);

        Value src = rhs;
        // write data output
        std::memcpy(reinterpret_cast<char*>(&out) + lhs_size + 1, reinterpret_cast<char*>(&src) + 1, rhs_size);

        return out;
    } else {
        String* str = rt->alloc_string(total_size);
        if (lhs_kind == ValueType::InlineString) {
            Value src = lhs;
            std::memcpy(&str->data, reinterpret_cast<char*>(&src) + 1, lhs_size);
        } else {  // lhs_kind == ValueKind::HeapString
            std::memcpy(&str->data, &value_get_string_ptr(lhs)->data, lhs_size);
        }
        if (rhs_kind == ValueType::InlineString) {
            Value src = rhs;
            std::memcpy(reinterpret_cast<char*>(&str->data) + lhs_size, reinterpret_cast<char*>(&src) + 1, rhs_size);
        } else {  // rhs_kind == ValueKind::HeapString
            std::memcpy(reinterpret_cast<char*>(&str->data) + lhs_size, &value_get_string_ptr(rhs)->data, rhs_size);
        }
        return to_value(str);
    }
}

Value value_add_int32(Value lhs, Value rhs) {
    return to_value(value_get_int32(lhs) + value_get_int32(rhs));
}

// below assume integer values
Value value_sub(Value lhs, Value rhs) {
    return to_value(value_get_int32(lhs) - value_get_int32(rhs));
}

Value value_mul(Value lhs, Value rhs) {
    return to_value(value_get_int32(lhs) * value_get_int32(rhs));
}

Value value_div(Value lhs, Value rhs) {
    int32_t denom = value_get_int32(rhs);
    if (denom == 0) {
        std::exit(1);
    }
    return to_value(value_get_int32(lhs) / denom);
}

auto value_eq_bool(Value lhs, Value rhs) -> bool {
    ValueType lhs_type = value_get_type(lhs);
    ValueType rhs_type = value_get_type(rhs);
    if (lhs_type != rhs_type) {
        return false;
    }
    ValueType type = lhs_type;
    if (type == ValueType::None || type == ValueType::Int 
        || type == ValueType::Bool || type == ValueType::InlineString 
        || type == ValueType::Record) {
        // bitwise comparison
        return lhs == rhs;
    }
    if (type == ValueType::HeapString) {
        auto lhs_str = value_get_string_ptr(lhs);
        auto rhs_str = value_get_string_ptr(rhs);
        size_t len = lhs_str->len;
        if (rhs_str->len != len) {
            return false;
        }
        for (size_t i = 0; i < len; ++i) {
            if (lhs_str->data[i] != rhs_str->data[i]) {
                return false;
            }
        }
        return true;
    }
    return false;
}

Value value_eq(Value lhs, Value rhs) {
    return to_value(value_eq_bool(lhs, rhs));
}

Value value_geq(Value lhs, Value rhs) {
    return to_value(value_get_int32(lhs) >= value_get_int32(rhs));
}

Value value_gt(Value lhs, Value rhs) {
    return to_value(value_get_int32(lhs) > value_get_int32(rhs));
}

Value value_and(Value lhs, Value rhs) {
    return to_value(value_get_bool(lhs) && value_get_bool(rhs));
}

Value value_or(Value lhs, Value rhs) {
    return to_value(value_get_bool(lhs) || value_get_bool(rhs));
}

Value value_not(Value val) {
    return to_value(!value_get_bool(val));
}

Value value_to_string(ProgramContext* rt, Value val) {
    auto type = value_get_type(val);
    if (type == ValueType::None) {
        return rt->none_string;
    }
    if (type == ValueType::InlineString || type == ValueType::HeapString) {
        return val;
    }
    if (type == ValueType::Bool) {
        if (value_get_bool(val)) {
            return rt->true_string;
        } else {
            return rt->false_string;
        }
    }
    if (type == ValueType::Int) {
        // TODO make more efficient
        return to_value(rt, std::to_string(value_get_int32(val)));
    }
    if (type == ValueType::Record) {
        return to_value(rt, value_get_std_string(val));
    }
    if (type == ValueType::Closure) {
        return rt->function_string;
    }
    return 0;
}

auto value_get_std_string(Value val) -> std::string {
    auto type = value_get_type(val);
    if (type == ValueType::None) {
        return "None";
    }
    if (type == ValueType::Bool) {
        if (value_get_bool(val)) {
            return "true";
        } else {
            return "false";
        }
    }
    if (type == ValueType::Int) {
        return std::to_string(value_get_int32(val));
    }
    if (type == ValueType::Record) {
        Record* record = value_get_record(val);
        std::string out{"{"};
        std::vector<std::pair<Value, Value>> vals{record->fields.begin(), record->fields.end()};
        std::sort(vals.begin(), vals.end(),
                  [](const std::pair<Value, Value>& l, const std::pair<Value, Value>& r) {
                      return value_get_std_string(l.first) < value_get_std_string(r.first);
                  });
        for (auto [key, field_value] : vals) {
            out.append(value_get_std_string(key));
            out.push_back(':');
            out.append(value_get_std_string(field_value));
            out.push_back(' ');
        }
        out.push_back('}');
        return out;
    }
    if (type == ValueType::Closure) {
        return "FUNCTION";
    }
    if (type == ValueType::InlineString) {
        char data[7];
        size_t len = (val >> 4) & 0b1111;
        Value v = val >> 8;
        std::memcpy(data, &v, len);
        return {data, len};
    }
    if (type == ValueType::HeapString) {
        String* str = value_get_string_ptr(val);
        return {str->data, str->len};
    }
    return "<<INVALID>>";
}

Value extern_alloc_ref(ProgramContext* rt) {
    return to_value(rt->alloc_ref());
}

Value extern_alloc_string(ProgramContext* rt, size_t length) {
    return to_value(rt->alloc_string(length));
}

Value extern_alloc_record(ProgramContext* rt) {
    return to_value(rt->alloc_record());
}

Value extern_alloc_closure(ProgramContext* rt, size_t num_free) {
    return to_value(rt->alloc_closure(num_free));
}

void extern_print(Value val) {
    std::cout << value_get_std_string(val) << '\n';
};

auto extern_intcast(Value val) -> Value {
    // TODO implement
    return 0b10000;
}

auto extern_input(ProgramContext* rt) -> Value {
    // TODO implement
    return 0b10000;
}

auto extern_rec_load_name(Value rec, Value name) -> Value {
    Record* rec_ptr = value_get_record(rec);
    if (rec_ptr->fields.contains(name)) {
        return rec_ptr->fields[name];
    }
    return 0;
}

void extern_rec_store_name(Value rec, Value name, Value val) {
    Record* rec_ptr = value_get_record(rec);
    rec_ptr->fields[name] = val;
}

auto extern_rec_load_index(ProgramContext* rt, Value rec, Value index_val) -> Value {
    Record* rec_ptr = value_get_record(rec);
    Value name = value_to_string(rt, index_val);
    if (rec_ptr->fields.contains(name)) {
        return rec_ptr->fields[name];
    }
    return 0;
}

void extern_rec_store_index(ProgramContext* rt, Value rec, Value index_val, Value val) {
    Record* rec_ptr = value_get_record(rec);
    Value name = value_to_string(rt, val);
    rec_ptr->fields[name] = val;
}

ProgramContext::ProgramContext() {
    this->none_string = to_value(this, "None");
    this->true_string = to_value(this, "true");
    this->false_string = to_value(this, "false");
    this->function_string = to_value(this, "FUNCTION");
}

auto ProgramContext::alloc_ref() -> Value* {
    HeapObject* obj = this->alloc_tracked(sizeof(Value));
    obj->type = HeapObject::REF;
    return reinterpret_cast<Value*>(&obj->data);
}

auto ProgramContext::alloc_string(size_t length) -> String* {
    HeapObject* obj = this->alloc_tracked(sizeof(String) + length);
    obj->type = HeapObject::STR;
    String* str = reinterpret_cast<String*>(&obj->data);
    str->len = length;
    return str;
}

auto ProgramContext::alloc_record() -> Record* {
    HeapObject* obj = this->alloc_tracked(sizeof(Record));
    obj->type = HeapObject::REC;
    Record* rec = reinterpret_cast<Record*>(&obj->data);
    new (rec) Record{};
    return rec;
}

auto ProgramContext::alloc_closure(size_t num_free) -> Closure* {
    HeapObject* obj = this->alloc_tracked(sizeof(Closure) + sizeof(Value) * num_free);
    obj->type = HeapObject::CLOSURE;
    Closure* closure = reinterpret_cast<Closure*>(&obj->data);
    closure->n_free_vars = num_free;
    return closure;
}

auto ProgramContext::alloc_tracked(size_t data_size) -> HeapObject* {
    size_t allocation_size = sizeof(HeapObject) + data_size;
    this->total_alloc += allocation_size;
    HeapObject* ptr = static_cast<HeapObject*>(malloc(allocation_size));
    ptr->marked = false;
    ptr->next = this->heap_head;
    this->heap_head = ptr;
    if (ptr == nullptr) {
        assert(false && "out of memory");
    }
    return ptr;
}

void ProgramContext::dealloc_tracked(HeapObject* obj) {
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

void ProgramContext::collect() {
    // no current allocations
    if (this->heap_head == nullptr) {
        return;
    }

    // free unmarked heads
    while (this->heap_head != nullptr && !this->heap_head->marked) {
        HeapObject* next = this->heap_head->next;
        this->dealloc_tracked(this->heap_head);
        this->heap_head = next;
    }

    // if we just cleared the entire heap, return
    if (this->heap_head == nullptr) {
        return;
    }

    // unmark head as we'll skip it in the following
    this->heap_head->marked = false;
    HeapObject* prev = this->heap_head;
    HeapObject* current = this->heap_head->next;

    while (current != nullptr) {
        if (!current->marked) {
            HeapObject* next = current->next;
            this->dealloc_tracked(current);
            current = next;
            prev->next = current;
        } else {
            current->marked = false;
            prev = current;
            current = current->next;
        }
    }
}

ProgramContext::~ProgramContext() {
    this->collect();
    std::free(this->globals);
}

void ProgramContext::init_globals(size_t num_globals) {
    if (this->globals != nullptr) {
        std::free(this->globals);
    }
    this->globals = static_cast<Value*>(malloc(8 * num_globals));
    for (size_t i = 0; i < num_globals; ++i) {
        this->globals[i] = 0b10000;
    }
}

std::size_t ValueHash::operator()(const Value& val) const noexcept {
    ValueType tag = static_cast<ValueType>(val & TAG_MASK);
    if (tag == ValueType::HeapString) {
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

bool ValueEq::operator()(const Value& lhs, const Value& rhs) const noexcept {
    return value_eq_bool(lhs, rhs);
}

};  // namespace runtime