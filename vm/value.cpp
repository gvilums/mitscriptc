#include "value.h"

#include "types.h"

#include <string>
#include <variant>

namespace VM {

auto value_from_constant(Constant c) -> Value {
    return std::visit([](auto x) -> Value { return x; }, c);
}

Value::Value(const Value& other)
    : tag{other.tag} {
    if (other.tag == NONE) {
        this->none = None{};
    } else if (other.tag == NUM) {
        this->num = other.num;
    } else if (other.tag == BOOL) {
        this->boolean = other.boolean;
    } else if (other.tag == STRING) {
        ::new (&this->str) auto(other.str);
    } else if (other.tag == HEAP_REF) {
        this->heap_ref = other.heap_ref;
    } else if (other.tag == FN_PTR) {
        this->fnptr = other.fnptr;
    } else if (other.tag == USIZE) {
        this->usize = other.usize;
    } else {
        std::terminate();
    }
}

Value::Value(Value&& other) noexcept
    : tag{other.tag} {
    if (other.tag == NONE) {
        this->none = None{};
    } else if (other.tag == NUM) {
        this->num = other.num;
    } else if (other.tag == BOOL) {
        this->boolean = other.boolean;
    } else if (other.tag == STRING) {
        ::new (&this->str) auto(std::move(other.str));
    } else if (other.tag == HEAP_REF) {
        this->heap_ref = other.heap_ref;
    } else if (other.tag == FN_PTR) {
        this->fnptr = other.fnptr;
    } else if (other.tag == USIZE) {
        this->usize = other.usize;
    } else {
        std::terminate();
    }
}

auto Value::operator=(const Value& other) -> Value& {
    // this->~Value();
    this->tag = other.tag;
    if (other.tag == NONE) {
        this->none = None{};
    } else if (other.tag == NUM) {
        this->num = other.num;
    } else if (other.tag == BOOL) {
        this->boolean = other.boolean;
    } else if (other.tag == STRING) {
        ::new (&this->str) auto(other.str);
    } else if (other.tag == HEAP_REF) {
        this->heap_ref = other.heap_ref;
    } else if (other.tag == FN_PTR) {
        this->fnptr = other.fnptr;
    } else if (other.tag == USIZE) {
        this->usize = other.usize;
    } else {
        std::terminate();
    }
    return *this;
}

auto Value::operator=(Value&& other) noexcept -> Value& {
    // this->~Value();
    this->tag = other.tag;
    if (other.tag == NONE) {
        this->none = None{};
    } else if (other.tag == NUM) {
        this->num = other.num;
    } else if (other.tag == BOOL) {
        this->boolean = other.boolean;
    } else if (other.tag == STRING) {
        ::new (&this->str) auto(std::move(other.str));
    } else if (other.tag == HEAP_REF) {
        this->heap_ref = other.heap_ref;
    } else if (other.tag == FN_PTR) {
        this->fnptr = other.fnptr;
    } else if (other.tag == USIZE) {
        this->usize = other.usize;
    } else {
        std::terminate();
    }
    return *this;
}

auto operator+(const Value& lhs, const Value& rhs) -> Value {
    if (lhs.tag == Value::NUM && rhs.tag == Value::NUM) {
        return {lhs.num + rhs.num};
    }
    if (lhs.tag == Value::STRING && rhs.tag == Value::STRING) {
        return {lhs.str + rhs.str};
    }
    if (lhs.tag == Value::STRING) {
        return {lhs.str + rhs.to_string()};
    }
    if (rhs.tag == Value::STRING) {
        return {lhs.to_string() + rhs.str};
    }
    throw std::string{"ERROR: invalid cast in '+' operation"};
}

auto operator==(const Value& lhs, const Value& rhs) -> bool {
    if (lhs.tag == rhs.tag) {
        if (lhs.tag == Value::NONE) {
            return true;
        }
        if (lhs.tag == Value::NUM) {
            return lhs.num == rhs.num;
        }
        if (lhs.tag == Value::BOOL) {
            return lhs.boolean == rhs.boolean;
        }
        if (lhs.tag == Value::STRING) {
            return lhs.str == rhs.str;
        }
        if (lhs.tag == Value::HEAP_REF) {
            if (lhs.heap_ref->tag == HeapObject::RECORD && rhs.heap_ref->tag == HeapObject::RECORD) {
                return lhs.heap_ref == rhs.heap_ref;
            }
            // comparing non-record allocations...
        }
    }
    // TODO think about throwing error on comparison of non-program-variables
    return false;
}

auto operator>=(Value const& lhs, Value const& rhs) -> bool {
    if (lhs.tag == Value::NUM && rhs.tag == Value::NUM) {
        return lhs.num >= rhs.num;
    }
    throw std::string{"ERROR: invalid cast to int in comparison"};
}

auto operator>(Value const& lhs, Value const& rhs) -> bool {
    if (lhs.tag == Value::NUM && rhs.tag == Value::NUM) {
        return lhs.num > rhs.num;
    }
    throw std::string{"ERROR: invalid cast to int in comparison"};
}

auto Value::to_string() const -> std::string {
    if (this->tag == NONE) {
        return "None";
    }
    if (this->tag == BOOL) {
        return this->boolean ? "true" : "false";
    }
    if (this->tag == NUM) {
        return std::to_string(this->num);
    }
    if (this->tag == STRING) {
        return this->str;
    }
    if (this->tag == HEAP_REF) {
        if (this->heap_ref->tag == HeapObject::RECORD) {
            std::string out{"{"};
            for (const auto& p : this->heap_ref->rec.fields) {
                out.append(p.first);
                out.push_back(':');
                out.append(p.second.to_string());
                out.push_back(' ');
            }
            out.push_back('}');
            return out;
        }
        if (this->heap_ref->tag == HeapObject::CLOSURE) {
            return "FUNCTION";
        }
    }
    throw std::string{"ERROR: trying to convert non-program-value to string"};
}

auto Value::get_tag() -> ValueTag {
    return this->tag;
}

auto Value::get_bool() -> bool {
    if (this->tag == BOOL) {
        return this->boolean;
    }
    throw std::string{"ERROR: invalid cast to bool"};
}

auto Value::get_int() -> int {
    if (this->tag == NUM) {
        return this->num;
    }
    throw std::string{"ERROR: invalid cast to int"};
}

auto Value::get_string() -> std::string {
    if (this->tag == STRING) {
        return this->str;
    }
    throw std::string{"ERROR: invalid cast to string"};
}

auto Value::get_heap_ref() -> HeapObject* {
    if (this->tag == HEAP_REF) {
        return this->heap_ref;
    }
    throw std::string{"ERROR: invalid cast to reference"};
}

auto Value::get_record() -> Record& {
    if (this->tag == HEAP_REF) {
        return this->heap_ref->get_record();
    }
    throw std::string{"ERROR: invalid cast to record"};
}

auto Value::get_closure() -> Closure& {
    if (this->tag == HEAP_REF) {
        return this->heap_ref->get_closure();
    }
    throw std::string{"ERROR: invalid cast to closure"};
}

auto Value::get_val_ref() -> Value& {
    if (this->tag == HEAP_REF) {
        return this->heap_ref->get_value();
    }
    throw std::string{"ERROR: invalid cast to reference"};
}

auto Value::get_fnptr() -> struct Function* {
    if (this->tag == FN_PTR) {
        return this->fnptr;
    }
    throw std::string{"ERROR: invalid cast to function pointer"};
}

auto Value::get_usize() -> size_t {
    if (this->tag == USIZE) {
        return this->usize;
    }
    throw std::string{"ERROR: invalid cast to usize"};
}

void Value::trace() {
    if (this->tag == HEAP_REF) {
        this->heap_ref->trace();
    }
}

auto HeapObject::get_value() -> Value& {
    if (this->tag == VALUE) {
        return this->val;
    }
    throw std::string{"ERROR: invalid cast to value"};
}

auto HeapObject::get_record() -> Record& {
    if (this->tag == RECORD) {
        return this->rec;
    }
    throw std::string{"ERROR: invalid cast to record"};
}

auto HeapObject::get_closure() -> Closure& {
    if (this->tag == CLOSURE) {
        return this->closure;
    }
    throw std::string{"ERROR: invalid cast to closure"};
}

void HeapObject::trace() {
    // early return if already visited
    if (this->marked) {
        return;
    }
    this->marked = true;
    // TODO think if need to gc value
    if (this->tag == RECORD) {
        for (auto& [key, val] : this->rec.fields) {
            val.trace();
        }
    }
    if (this->tag == CLOSURE) {
        for (auto* ref : this->closure.refs) {
            ref->trace();
        }
    }
}

// void Value::follow(CollectedHeap& heap) {
//     if (this->tag == RECORD) {
//         heap.markSuccessors(this->record.internal);
//     }
//     if (this->tag == CLOSURE) {
//         heap.markSuccessors(this->closure.closure);
//     }
//     if (this->tag == REFERENCE) {
//         heap.markSuccessors(this->reference.ref);
//     }
// }

// void Record::follow(CollectedHeap &heap) {
//     for (auto& [l, r] : this->fields) {
//         heap.markSuccessors(&r);
//     }
// }


// void Closure::follow(CollectedHeap &heap) {
//     for (auto& val_ref : this->refs) {
//         heap.markSuccessors(val_ref.ref);
//     }
// }

// auto Value::size() -> size_t {
//     return sizeof(Value);
// }

// auto Record::size() -> size_t {
//     return sizeof(Record);
// }

// auto Closure::size() -> size_t {
//     return sizeof(Closure);
// }
};  // namespace VM