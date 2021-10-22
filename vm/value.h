#pragma once

#include <exception>
#include <map>
#include <string>
#include <utility>

#include "types.h"

namespace VM {

class Value;

enum class FnType { DEFAULT,
                    PRINT,
                    INPUT,
                    INTCAST };

struct Record {
    std::map<std::string, Value> fields;
};

struct RecordRef {
    Record* internal;
};

struct ValueRef {
    Value* ref;
};

struct Closure {
    FnType type;
    struct Function* fn;
    std::vector<ValueRef> refs;
};

struct ClosureRef {
    Closure* closure;
};

class Value {
    enum { NONE,
           NUM,
           BOOL,
           STRING,
           RECORD,
           CLOSURE,
           REFERENCE,
           FN_PTR,
           USIZE } tag{NONE};
    union {
        None none;
        int num;
        bool boolean;
        std::string str;
        RecordRef record;
        ClosureRef closure;
        ValueRef reference;
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
    Value(RecordRef rec)
        : tag{RECORD}, record{rec} {}
    Value(ClosureRef c)
        : tag{CLOSURE}, closure{c} {}
    Value(ValueRef ref)
        : tag{REFERENCE}, reference{ref} {}
    Value(struct Function* ptr)
        : tag{FN_PTR}, fnptr{ptr} {}
    Value(size_t u)
        : tag{USIZE}, usize{u} {}
    Value(std::string s)
        : tag{STRING} {
        ::new (&this->str) auto(std::move(s));
    }

    Value(const Value& other);
    Value(Value&& other) noexcept;

    ~Value() {
        if (this->tag == STRING) {
            this->str.~basic_string();
        }
    }

    auto operator=(const Value& other) -> Value&;
    auto operator=(Value&& other) noexcept -> Value&;

    friend auto operator+(const Value& lhs, const Value& rhs) -> Value;
    friend auto operator==(const Value& lhs, const Value& rhs) -> bool;
    friend auto operator>=(Value const& lhs, Value const& rhs) -> bool;
    friend auto operator>(Value const& lhs, Value const& rhs) -> bool;

    [[nodiscard]] auto to_string() const -> std::string;

    inline auto get_bool() -> bool {
        if (this->tag == BOOL) {
            return this->boolean;
        }
        throw std::string{"ERROR: invalid cast to bool"};
    }

    inline auto get_int() -> int {
        if (this->tag == NUM) {
            return this->num;
        }
        throw std::string{"ERROR: invalid cast to int"};
    }

    inline auto get_string() -> std::string {
        if (this->tag == STRING) {
            return this->str;
        }
        throw std::string{"ERROR: invalid cast to string"};
    }

    inline auto get_record() -> RecordRef {
        if (this->tag == RECORD) {
            return this->record;
        }
        throw std::string{"ERROR: invalid cast to record"};
    }

    inline auto get_closure() -> ClosureRef {
        if (this->tag == CLOSURE) {
            return this->closure;
        }
        throw std::string{"ERROR: invalid cast to closure"};
    }

    inline auto get_ref() -> ValueRef {
        if (this->tag == REFERENCE) {
            return this->reference;
        }
        throw std::string{"ERROR: invalid cast to reference"};
    }

    inline auto get_fnptr() -> struct Function* {
        if (this->tag == FN_PTR) {
            return this->fnptr;
        }
        throw std::string{"ERROR: invalid cast to function pointer"};
    }

    inline auto get_usize() -> size_t {
        if (this->tag == USIZE) {
            return this->usize;
        }
        throw std::string{"ERROR: invalid cast to usize"};
    }
};

auto value_from_constant(Constant c) -> Value;

};  // namespace VM