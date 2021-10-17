#include "value.h"

#include "types.h"

#include <string>
#include <variant>

ProgVal value_from_constant(Constant c) {
    return std::visit([](auto x) -> ProgVal { return x; }, c);
}

bool value_eq(ProgVal l, ProgVal r) {
    return std::visit(
        overloaded{
            [](None, None) -> bool { return true; },
            [](bool l, bool r) -> bool { return l == r; },
            [](int x, int y) -> bool { return x == y; },
            [](const std::string& l, const std::string& r) -> bool {
                return l == r;
            },
            [](RecordCell lrec, RecordCell rrec) -> bool {
                return lrec.internal == rrec.internal;
            },
            [](auto x, auto y) -> bool { return false; }},
        l, r);
}

std::string value_to_string(ProgVal val) {
    return std::visit(
        overloaded{
            [](None x) -> std::string { return std::string{"None"}; },
            [](bool b) -> std::string { return b ? "true" : "false"; },
            [](int i) -> std::string { return std::to_string(i); },
            [](std::string s) -> std::string { return s; },
            [](RecordCell r) -> std::string {
                std::string out{"{"};
                for (auto p : *r.internal) {
                    out.append(p.first);
                    out.push_back(':');
                    out.append(value_to_string(p.second));
                    out.push_back(' ');
                }
                out.push_back('}');
                return out;
            },
            [](ClosureRef c) -> std::string { return std::string{"FUNCTION"}; },
        },
        val);
}
