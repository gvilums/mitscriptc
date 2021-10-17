#pragma once

#include "instructions.h"

#include <cstdint>
#include <memory>
#include <string>
#include <variant>
#include <vector>

using None = std::monostate;
using Constant = std::variant<None, int, bool, std::string>;

template <class... Ts>
struct overloaded : Ts... {
    using Ts::operator()...;
};
template <class... Ts>
overloaded(Ts...) -> overloaded<Ts...>;

struct Function {
    // List of functions defined within this function (but not functions defined
    // inside of nested functions)
    std::vector<Function*> functions_;

    // List of constants used by the instructions within this function (but not
    // nested functions)
    std::vector<Constant> constants_;

    // The number of parameters to the function
    uint32_t parameter_count_;

    // List of local variables
    // The first parameter_count_ variables are the function's parameters
    // in their order as given in the paraemter list
    std::vector<std::string> local_vars_;

    // List of local variables accessed by reference (LocalReference)
    std::vector<std::string> local_reference_vars_;

    // List of the names of non-global and non-local variables accessed by the
    // function
    std::vector<std::string> free_vars_;

    // List of global variable and field names used inside the function
    std::vector<std::string> names_;

    InstructionList instructions;
};
