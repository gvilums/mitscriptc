#include "value.h"

#include "types.h"

#include <string>
#include <variant>

auto value_from_constant(Constant c) -> Value {
    return std::visit([](auto x) -> Value { return x; }, c);
}

// Value::Value(const Value& other) : tag{other.tag} {

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
        if (lhs.tag == Value::RECORD) {
            return lhs.record.internal == rhs.record.internal;
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
    if (this->tag == RECORD) {
        std::string out{"{"};
        for (const auto& p : *this->record.internal) {
            out.append(p.first);
            out.push_back(':');
            out.append(p.second.to_string());
            out.push_back(' ');
        }
        out.push_back('}');
        return out;
    }
    if (this->tag == CLOSURE) {
        return "FUNCTION";
    }
    throw std::string{"ERROR: trying to convert non-program-value to string"};
}