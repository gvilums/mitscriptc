#pragma once

#include "value.h"

namespace alloc {
    
    struct HeapObject {
        HeapObject* next{nullptr};
        enum {
            REF,
            STR,
            REC,
            CLOSURE,
        } type;
        bool marked{false};
        std::uint64_t data[];
    };

    Value alloc_ref();
    Value alloc_string();
    Value alloc_record();
    Value alloc_closure();
};