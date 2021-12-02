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

bool is_heap_type(ValueType type) {
    return type == ValueType::HeapString || type == ValueType::Record || type == ValueType::Closure
           || type == ValueType::Reference;
}

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

Value value_add_nonint(ProgramContext* rt, Value lhs, Value rhs) {
    lhs = value_to_string(rt, lhs);
    rhs = value_to_string(rt, rhs);
    ValueType lhs_kind = value_get_type(lhs);
    ValueType rhs_kind = value_get_type(rhs);
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

Value value_add(ProgramContext* rt, Value lhs, Value rhs) {
    auto lhs_kind = value_get_type(lhs);
    auto rhs_kind = value_get_type(rhs);
    if (lhs_kind == ValueType::Int && rhs_kind == ValueType::Int) {
        return value_add_int32(lhs, rhs);
    }
    return value_add_nonint(rt, lhs, rhs);
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
    if (lhs == rhs && lhs_type != ValueType::Closure) {
        return true;
    }
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
    if (type == ValueType::InlineString || type == ValueType::HeapString) {
        return val;
    }
    if (type == ValueType::Int) {
        int n = value_get_int32(val);
        if (n > 0 && n < 10000000) {
            uint64_t out = 0;
            uint8_t i = 0;
            while (n > 0) {
                out <<= 8;
                out |= (uint64_t)((n % 10) + '0');
                n /= 10;
                i = i + 1;
            }
            out <<= 8;
            out |= (i << 4) | INLINE_STRING_TAG;
            return out;
        }
        return to_value(rt, std::to_string(value_get_int32(val)));
    }
    if (type == ValueType::None) {
        return rt->none_string;
    }
    if (type == ValueType::Bool) {
        if (value_get_bool(val)) {
            return rt->true_string;
        } else {
            return rt->false_string;
        }
    }
    if (type == ValueType::Record) {
        return to_value(rt, value_get_std_string(rt, val));
    }
    if (type == ValueType::Closure) {
        return rt->function_string;
    }
    return 0;
}

auto value_get_std_string(ProgramContext* ctx, Value val) -> std::string {
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
        std::vector<std::pair<Value, Value>> entries;
        if (record->dynamic_fields != nullptr) {
            entries.insert(entries.end(), record->dynamic_fields->begin(), record->dynamic_fields->end());
        }
        const std::vector<Value>& layout = ctx->layouts[record->layout_index];
        for (int i = 0; i < record->static_field_count; ++i) {
            entries.emplace_back(layout[i], record->static_fields[i]);
        }
        std::sort(entries.begin(), entries.end(),
                  [=](const std::pair<Value, Value>& l, const std::pair<Value, Value>& r) {
                      return value_get_std_string(ctx, l.first) < value_get_std_string(ctx, r.first);
                  });
        for (auto [key, field_value] : entries) {
            out.append(value_get_std_string(ctx, key));
            out.push_back(':');
            out.append(value_get_std_string(ctx, field_value));
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
    if (type == ValueType::Reference) {
        return "REF TO: " + value_get_std_string(ctx, *value_get_ref(val));
    }
    return "<< INVALID >>";
}

Value extern_alloc_ref(ProgramContext* rt) {
    return to_value(rt->alloc_ref());
}

Value extern_alloc_string(ProgramContext* rt, size_t length) {
    return to_value(rt->alloc_string(length));
}

Value extern_alloc_record(ProgramContext* rt, size_t num_static, size_t layout_index) {
    return to_value(rt->alloc_record(num_static, layout_index));
}

Value extern_alloc_closure(ProgramContext* rt, size_t num_free) {
    return to_value(rt->alloc_closure(num_free));
}

void extern_print(ProgramContext* rt, Value val) {
    std::cout << value_get_std_string(rt, val) << '\n';
};

auto extern_intcast(ProgramContext* rt, Value val) -> Value {
    auto type = value_get_type(val);
    if (type == ValueType::HeapString || type == ValueType::InlineString) {
        try {
            int32_t out = std::stoi(value_get_std_string(rt, val));
            return to_value(out);
        } catch (...) {}
    }
    return 0b10000;
}

auto extern_input(ProgramContext* rt) -> Value {
    std::string input;
    std::getline(std::cin, input);
    return to_value(rt, input);
}

auto extern_rec_load_name(ProgramContext* ctx, Value rec, Value name) -> Value {
    Record* rec_ptr = value_get_record(rec);
    uint32_t static_field_count = rec_ptr->static_field_count;
    const auto& layout = ctx->layouts[rec_ptr->layout_index];
    // can start at 4 because 0 through 3 are checked in assembly
    for (int i = 4; i < static_field_count; ++i) {
        if (name == layout[i]) {
            return rec_ptr->static_fields[i];
        }
//        if (value_eq_bool(name, layout[i])) {
//            return rec_ptr->static_fields[i];
//        }
    }
    if (rec_ptr->dynamic_fields != nullptr) {
        auto iter = rec_ptr->dynamic_fields->find(name);
        if (iter != rec_ptr->dynamic_fields->end()) {
            return iter->second;
        }
    }
    return 0;
}

void extern_rec_store_name(ProgramContext* ctx, Value rec, Value name, Value val) {
    Record* rec_ptr = value_get_record(rec);
    uint32_t static_field_count = rec_ptr->static_field_count;
    const auto& layout = ctx->layouts[rec_ptr->layout_index];
    for (int i = 0; i < static_field_count; ++i) {
        if (value_eq_bool(name, layout[i])) {
            rec_ptr->static_fields[i] = val;
            return;
        }
    }
    if (rec_ptr->dynamic_fields == nullptr) {
        rec_ptr->init_map(ctx);
    }
    rec_ptr->dynamic_fields->operator[](name) = val;
}

auto extern_rec_load_index(ProgramContext* ctx, Value rec, Value index_val) -> Value {
    Record* rec_ptr = value_get_record(rec);
    Value name = value_to_string(ctx, index_val);
    uint32_t static_field_count = rec_ptr->static_field_count;
    const auto& layout = ctx->layouts[rec_ptr->layout_index];
    for (int i = 0; i < static_field_count; ++i) {
        if (value_eq_bool(name, layout[i])) {
            return rec_ptr->static_fields[i];
        }
    }
    if (rec_ptr->dynamic_fields != nullptr) {
        auto iter = rec_ptr->dynamic_fields->find(name);
        if (iter != rec_ptr->dynamic_fields->end()) {
            return iter->second;
        }
    }
    return 0;
}

void extern_rec_store_index(ProgramContext* ctx, Value rec, Value index_val, Value val) {
    Record* rec_ptr = value_get_record(rec);
    Value name = value_to_string(ctx, index_val);
    uint32_t static_field_count = rec_ptr->static_field_count;
    const auto& layout = ctx->layouts[rec_ptr->layout_index];
    for (int i = 0; i < static_field_count; ++i) {
        if (value_eq_bool(name, layout[i])) {
            rec_ptr->static_fields[i] = val;
            return;
        }
    }
    if (rec_ptr->dynamic_fields == nullptr) {
        rec_ptr->init_map(ctx);
    }
    rec_ptr->dynamic_fields->operator[](name) = val;
}

ProgramContext::ProgramContext(size_t heap_size) {
    // align heap size
    heap_size &= ~0b1111;
    this->heap = static_cast<char*>(malloc(heap_size + 8));
    this->region_size = heap_size / 2;
    this->gc_threshold = region_size - (1 << 14);
    this->write_head = heap + 8;

    this->none_string = to_value(this, "None");
    this->true_string = to_value(this, "true");
    this->false_string = to_value(this, "false");
    this->function_string = to_value(this, "FUNCTION");
}

auto ProgramContext::alloc_ref() -> Value* {
    HeapObject* obj = this->alloc_traced(sizeof(Value));
    return reinterpret_cast<Value*>(&obj->data);
}

auto ProgramContext::alloc_string(size_t length) -> String* {
    HeapObject* obj = this->alloc_traced(sizeof(String) + length);
    auto* str = reinterpret_cast<String*>(&obj->data);
    str->len = length;
    return str;
}

auto ProgramContext::alloc_record(uint32_t num_static, uint32_t layout) -> Record* {
    HeapObject* obj = this->alloc_traced(sizeof(Record) + sizeof(Value) * num_static);
    auto* rec = reinterpret_cast<Record*>(&obj->data);
    rec->layout_offset = layout_offsets[layout]; // TODO check
    rec->layout_index = layout;
    rec->static_field_count = num_static;
    rec->dynamic_fields = nullptr;
    for (int i = 0; i < num_static; ++i) {
        rec->static_fields[i] = 0;
    }
    return rec;
}

auto ProgramContext::alloc_closure(size_t num_free) -> Closure* {
    HeapObject* obj = this->alloc_traced(sizeof(Closure) + sizeof(Value) * num_free);
    auto* closure = reinterpret_cast<Closure*>(&obj->data);
    closure->n_free_vars = num_free;
    return closure;
}

auto ProgramContext::alloc_traced(size_t data_size) -> HeapObject* {
    size_t allocation_size = sizeof(HeapObject) + data_size;
    HeapObject* ptr;
    if (this->current_region == 2) {
        ptr = static_cast<HeapObject*>(std::malloc(allocation_size));
        this->static_allocations.push_back(ptr);
    } else {
        // align to multiple of 8
        size_t aligned_size = ((allocation_size - 1) | 0b1111) + 1;
        current_alloc += aligned_size;
        if (current_alloc >= region_size) {
            std::cout << "out of memory" << std::endl;
            std::exit(1);
        }
        ptr = reinterpret_cast<HeapObject*>(write_head);
        write_head += aligned_size;
    }
    ptr->region = current_region;
    return ptr;
}

ProgramContext::~ProgramContext() {
    std::free(this->heap);
    std::free(this->globals);
    for (void* ptr : this->static_allocations) {
        std::free(ptr);
    }
}

void ProgramContext::start_dynamic_alloc() {
    if (this->current_region == 2) {
        this->current_region = 0;
    }
}

void ProgramContext::init_globals(size_t num_globals) {
    if (this->globals != nullptr) {
        assert(false && "cannot reinitialize globals");
    }
    this->globals_size = num_globals;
    this->globals = static_cast<Value*>(std::malloc(sizeof(Value) * num_globals));
    for (size_t i = 0; i < num_globals; ++i) {
        this->globals[i] = 0b10000;
    }
}

void ProgramContext::switch_region() {
    // switch region
    current_region = 1 - current_region;
    // reset write head
    write_head = heap + 8 + current_region * region_size;
    current_alloc = 0;
}

void ProgramContext::reset_globals() {
    for (int i = 0; i < globals_size; ++i) {
        // reset to uninit
        globals[globals_size] = 0b10000;
    }
}

void ProgramContext::init_layouts(std::vector<std::vector<Value>> field_layouts) {
    this->layouts = std::move(field_layouts);
}

auto ProgramContext::alloc_raw(size_t num_bytes) -> void* {
    void* ptr;
    if (current_region == 2) {
        ptr = malloc(num_bytes);
        this->static_allocations.push_back(ptr);
    } else {
        // align to 8 bytes
        size_t aligned_size = ((num_bytes - 1) | 0b1111) + 1;
        current_alloc += aligned_size;
        if (current_alloc >= region_size) {
            std::cout << "out of memory" << std::endl;
            std::exit(1);
        }
        ptr = write_head;
        write_head += aligned_size;
    }
    return ptr;
}

void trace_collect(ProgramContext* ctx, const uint64_t* rbp, uint64_t* rsp) {
    ctx->switch_region();
    // TODO CHECK
    // base rbp is pointing two slots above saved rsp on stack
//    std::cout << "--- tracing stack ---" << std::endl;
    auto* base_rsp = reinterpret_cast<uint64_t*>(ctx->saved_rsp);
    while (rsp != base_rsp) {
        while (rsp != rbp) {
            trace_value(ctx, rsp);
            rsp += 1;
        }
        rbp = reinterpret_cast<uint64_t*>(*rsp);
        rsp += 2;
    }
//    std::cout << "--- tracing globals ---" << std::endl;
    for (int i = 0; i < ctx->globals_size; ++i) {
        trace_value(ctx, ctx->globals + i);
    }
//    std::cout << "----- finished collecting -----" << std::endl;
    // TODO DEBUG
//    char* cleared_region = ctx->heap + 8 + (1 - ctx->current_region) * ctx->region_size;
//    std::memset(cleared_region, 0, ctx->region_size);
}

void trace_value(ProgramContext* ctx, Value* ptr) {
    auto type = value_get_type(*ptr);
    if (is_heap_type(type)) {
        auto* heap_obj = reinterpret_cast<HeapObject*>((*ptr & DATA_MASK) - sizeof(HeapObject));
        // check for statically allocated objects
        if (heap_obj->region == 2) {
            return;
        }
        if (heap_obj->region == ctx->current_region) {
            // if moved, new value is in data field
            //! IMPORTANT note: data already has tag bit set
            *ptr = heap_obj->data[0];
            return;
        }
        // heap object has not been moved
        if (type == ValueType::Reference) {
            // allocate new value
            Value* new_ref = ctx->alloc_ref();
            // copy previous data to new data
            *new_ref = *value_get_ref(*ptr);
            // set forward
            heap_obj->region = ctx->current_region;
            Value new_value = to_value(new_ref);
            heap_obj->data[0] = new_value;
            *ptr = new_value;
            // trace reference
            trace_value(ctx, new_ref);
        } else if (type == ValueType::HeapString) {
            String* old_string = value_get_string_ptr(*ptr);
            size_t len = old_string->len;
            // copy data
            String* new_string = ctx->alloc_string(len);
            std::memcpy(&new_string->data, &old_string->data, len);
            // set forward
            heap_obj->region = ctx->current_region;
            Value new_value = to_value(new_string);
            heap_obj->data[0] = new_value;
            *ptr = new_value;
            // nothing to trace
        } else if (type == ValueType::Record) {
            Record* old_record = value_get_record(*ptr);
            // copy data
            Record* new_record = ctx->alloc_record(
                old_record->static_field_count, old_record->layout_index);
            // set forward
            heap_obj->region = ctx->current_region;
            Value new_value = to_value(new_record);
            heap_obj->data[0] = new_value;
            *ptr = new_value;
            // trace dynamic_fields of record, if they exist
            if (old_record->dynamic_fields != nullptr) {
                new_record->init_map(ctx);
                for (auto& elem : *old_record->dynamic_fields) {
                    Value key = elem.first;
                    Value val = elem.second;
                    trace_value(ctx, &key);
                    trace_value(ctx, &val);
                    new_record->dynamic_fields->operator[](key) = val;
                }
            }
            for (int i = 0; i < new_record->static_field_count; ++i) {
                Value val = old_record->static_fields[i];
                trace_value(ctx, &val);
                new_record->static_fields[i] = val;
            }
            // map allocates in gc heap and hence needs no dealloc
        } else if (type == ValueType::Closure) {
            Closure* old_closure = value_get_closure(*ptr);
            size_t num_free = old_closure->n_free_vars;
            // copy data
            Closure* new_closure = ctx->alloc_closure(num_free);
            std::memcpy(new_closure, old_closure, sizeof(Closure) + 8 * num_free);
            //set forward
            heap_obj->region = ctx->current_region;
            Value new_value = to_value(new_closure);
            heap_obj->data[0] = new_value;
            *ptr = new_value;
            // trace captures
            for (int i = 0; i < num_free; ++i) {
                trace_value(ctx, &new_closure->free_vars[i]);
            }
        } else {
            assert(false);
        }
    }
}


std::size_t ValueHash::operator()(const Value& val) const noexcept {
    ValueType tag = value_get_type(val);
    if (tag == ValueType::HeapString) {
        auto* str = reinterpret_cast<String*>(val & DATA_MASK);
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

void Record::init_map(ProgramContext* ctx) {
    auto* map_ptr = static_cast<Record::map_type*>(
        ctx->alloc_raw(sizeof(Record::map_type)));
    ::new (map_ptr) Record::map_type{ProgramAllocator<Record::alloc_type>{ctx}};
    this->dynamic_fields = map_ptr;
}

};  // namespace runtime