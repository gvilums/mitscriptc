#include "value.h"

#include "types.h"

#include <algorithm>
#include <string>
#include <variant>

namespace VM {


// Value::Value(const Value& other)
//     : tag{other.tag} {
//     if (other.tag == NONE) {
//         this->none = None{};
//     } else if (other.tag == NUM) {
//         this->num = other.num;
//     } else if (other.tag == BOOL) {
//         this->boolean = other.boolean;
//     } else if (other.tag == STRING) {
//         ::new (&this->str) auto(other.str);
//     } else if (other.tag == HEAP_REF) {
//         this->heap_ref = other.heap_ref;
//     } else if (other.tag == FN_PTR) {
//         this->fnptr = other.fnptr;
//     } else if (other.tag == USIZE) {
//         this->usize = other.usize;
//     } else {
//         std::terminate();
//     }
// }

// Value::Value(Value&& other) noexcept
//     : tag{other.tag} {
//     if (other.tag == NONE) {
//         this->none = None{};
//     } else if (other.tag == NUM) {
//         this->num = other.num;
//     } else if (other.tag == BOOL) {
//         this->boolean = other.boolean;
//     } else if (other.tag == STRING) {
//         ::new (&this->str) auto(std::move(other.str));
//     } else if (other.tag == HEAP_REF) {
//         this->heap_ref = other.heap_ref;
//     } else if (other.tag == FN_PTR) {
//         this->fnptr = other.fnptr;
//     } else if (other.tag == USIZE) {
//         this->usize = other.usize;
//     } else {
//         std::terminate();
//     }
// }

// auto Value::operator=(const Value& other) -> Value& {
//     this->destroy_contents();
//     this->tag = other.tag;
//     if (other.tag == NONE) {
//         this->none = None{};
//     } else if (other.tag == NUM) {
//         this->num = other.num;
//     } else if (other.tag == BOOL) {
//         this->boolean = other.boolean;
//     } else if (other.tag == STRING) {
//         ::new (&this->str) auto(other.str);
//     } else if (other.tag == HEAP_REF) {
//         this->heap_ref = other.heap_ref;
//     } else if (other.tag == FN_PTR) {
//         this->fnptr = other.fnptr;
//     } else if (other.tag == USIZE) {
//         this->usize = other.usize;
//     } else {
//         std::terminate();
//     }
//     return *this;
// }

// auto Value::operator=(Value&& other) noexcept -> Value& {
//     this->destroy_contents();
//     this->tag = other.tag;
//     if (other.tag == NONE) {
//         this->none = None{};
//     } else if (other.tag == NUM) {
//         this->num = other.num;
//     } else if (other.tag == BOOL) {
//         this->boolean = other.boolean;
//     } else if (other.tag == STRING) {
//         ::new (&this->str) auto(std::move(other.str));
//     } else if (other.tag == HEAP_REF) {
//         this->heap_ref = other.heap_ref;
//     } else if (other.tag == FN_PTR) {
//         this->fnptr = other.fnptr;
//     } else if (other.tag == USIZE) {
//         this->usize = other.usize;
//     } else {
//         std::terminate();
//     }
//     return *this;
// }

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
    throw std::string{"RuntimeException"};
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
    throw std::string{"IllegalCastException"};
}

auto operator>(Value const& lhs, Value const& rhs) -> bool {
    if (lhs.tag == Value::NUM && rhs.tag == Value::NUM) {
        return lhs.num > rhs.num;
    }
    throw std::string{"IllegalCastException"};
}

auto Value::to_string() const -> TrackedString {
    if (this->tag == NONE) {
        return "None";
    }
    if (this->tag == BOOL) {
        return this->boolean ? "true" : "false";
    }
    if (this->tag == NUM) {
        return TrackedString{std::to_string(this->num)};
    }
    if (this->tag == STRING) {
        return this->str;
    }
    if (this->tag == HEAP_REF) {
        if (this->heap_ref->tag == HeapObject::RECORD) {
            TrackedString out{"{"};
            std::vector<std::pair<TrackedString, Value>> vals{this->heap_ref->rec.fields.begin(), this->heap_ref->rec.fields.end()};
            std::sort(vals.begin(), vals.end(),
                      [](const std::pair<TrackedString, Value>& l, const std::pair<TrackedString, Value>& r) { return l.first < r.first; });
            for (const auto& p : vals) {
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
    throw std::string{"RuntimeException"};
}

auto Value::get_tag() -> ValueTag {
    return this->tag;
}

auto Value::get_bool() -> bool {
    if (this->tag == BOOL) {
        return this->boolean;
    }
    throw std::string{"IllegalCastException"};
}

auto Value::get_int() -> int {
    if (this->tag == NUM) {
        return this->num;
    }
    throw std::string{"IllegalCastException"};
}

auto Value::get_string() -> String& {
    if (this->tag == STRING_PTR) {
        return *this->str;
    }
    throw std::string{"IllegalCastException"};
}

auto Value::get_record() -> Record& {
    if (this->tag == RECORD_PTR) {
        return *this->record;
    }
    throw std::string{"IllegalCastException"};
}

auto Value::get_closure() -> Closure& {
    if (this->tag == CLOSURE_PTR) {
        return *this->closure;
    }
    throw std::string{"IllegalCastException"};
}

auto Value::get_val_ref() -> Value& {
    if (this->tag == VALUE_PTR) {
        return *this->value;
    }
    throw std::string{"IllegalCastException"};
}

auto Value::get_fnptr() -> struct Function* {
    if (this->tag == FN_PTR) {
        return this->fnptr;
    }
    throw std::string{"IllegalCastException"};
}

auto Value::get_usize() -> size_t {
    if (this->tag == USIZE) {
        return this->usize;
    }
    throw std::string{"IllegalCastException"};
}

void Value::trace() {
    if (this->tag == HEAP_REF) {

    }
}

auto HeapObject::get_value() -> Value& {
    if (this->tag == VALUE) {
        return this->val;
    }
    throw std::string{"IllegalCastException"};
}

auto HeapObject::get_record() -> Record& {
    if (this->tag == RECORD) {
        return this->rec;
    }
    throw std::string{"IllegalCastException"};
}

auto HeapObject::get_closure() -> Closure& {
    if (this->tag == CLOSURE) {
        return this->closure;
    }
    throw std::string{"IllegalCastException"};
}

void HeapObject::trace() {
    // early return if already visited
    if (this->marked) {
        return;
    }
    this->marked = true;
    if (this->tag == VALUE) {
        this->val.trace();
    }
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