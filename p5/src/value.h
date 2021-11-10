#pragma once

#include <cstdint>

using Value = std::uint64_t;

struct String {
    std::size_t len;
    char data[];
};

struct Closure {
    std::uint64_t fnptr;
    std::size_t n_args;
    Value free_vars[];
};

struct Record {
    
};

Value from_bool(bool b);
Value from_int(uint32_t i);
Value from_str_ptr(String* str);
Value from_rec_ptr(Record* rec);
Value from_closure_ptr(Closure* clsr);