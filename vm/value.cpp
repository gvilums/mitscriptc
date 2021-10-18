#include "value.h"

#include "types.h"

#include <string>
#include <variant>

auto value_from_constant(Constant c) -> Value {
    return std::visit([](auto x) -> Value { return x; }, c);
}

auto value_eq(Value l, Value r) -> bool {
    return std::visit(
        overloaded{
            [](None x, None y) -> bool { return true; },
            [](bool l, bool r) -> bool { return l == r; },
            [](int x, int y) -> bool { return x == y; },
            [](const std::string& l, const std::string& r) -> bool {
                return l == r;
            },
            [](RecordRef lrec, RecordRef rrec) -> bool {
                return lrec.internal == rrec.internal;
            },
            // TODO think about this
            [](auto x, auto y) -> bool { return false; }},
        l, r);
}

auto value_to_string(Value val) -> std::string {
    return std::visit(
        overloaded{
            [](None x) -> std::string { return std::string{"None"}; },
            [](bool b) -> std::string { return b ? "true" : "false"; },
            [](int i) -> std::string { return std::to_string(i); },
            [](std::string s) -> std::string { return s; },
            [](RecordRef r) -> std::string {
                std::string out{"{"};
                for (const auto& p : *r.internal) {
                    out.append(p.first);
                    out.push_back(':');
                    out.append(value_to_string(p.second));
                    out.push_back(' ');
                }
                out.push_back('}');
                return out;
            },
            [](ClosureRef c) -> std::string { return std::string{"FUNCTION"}; },
            [](auto x) -> std::string { throw std::string{"ERROR: cannot convert non-value to string"}; },
        },
        val);
}
