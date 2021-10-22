#pragma once

#include <map>
#include <string>
#include <utility>
#include <exception>

#include "types.h"

class Value;

enum class FnType { DEFAULT,
                    PRINT,
                    INPUT,
                    INTCAST };

struct RecordRef {
    std::map<std::string, Value>* internal;
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

// possible values on the program stack
// using Value = std::variant<None, int, bool, std::string, RecordRef, ClosureRef, ValueRef, struct Function*, size_t>;

class Value {
    enum{NONE, NUM, BOOL, STRING, RECORD, CLOSURE, REFERENCE, FN_PTR, USIZE} tag;
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
    Value() : tag{NONE}, none{None{}} {}
    Value(None none_val) : tag{NONE}, none{None{}} {}
    Value(int n) : tag{NUM}, num{n} {}
    Value(bool b) : tag{BOOL}, boolean{b} {}
    Value(std::string s) : tag{STRING}, str{std::move(s)} {}
    Value(RecordRef rec) : tag{RECORD}, record{rec} {}
    Value(ClosureRef c) : tag{CLOSURE}, closure{c} {}
    Value(ValueRef ref) : tag{REFERENCE}, reference{ref} {}
    Value(struct Function* ptr) : tag{FN_PTR}, fnptr{ptr} {}
    Value(size_t u) : tag{USIZE}, usize{u} {}
    
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