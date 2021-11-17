#include <iterator>
#include "ir.h"
#include "optimizer.h"
#include "value.h"
#include <set>

using namespace IR;

const std::set<IR::Instruction> unused_ins = {ADD,
    
    ADD_INT,
    SUB,
    MUL,
    DIV,
    EQ,
    GT,
    GEQ,
    AND,
    OR,
    NOT,
    MOV,
    ASSERT_BOOL,
    ASSERT_INT,
    ASSERT_STRING,
    ASSERT_RECORD,
    ASSERT_CLOSURE,
    ASSERT_NONZERO};


optimizer::optimizer(IR::Program* prog) : prog_(prog){};

IR::Program* optimizer::optimize() {
    return prog_;
}

bool optimizer::is_unused_var(const IR::Operation op){
    return unused_ins.contains();
}

void optimizer::rm_unused_var(){
    for (auto& fun : prog_->functions) {
        std::unordered_set<int> used_var;
        for (size_t idx = fun.blocks.size() - 1; idx >= 0; idx--) {
            IR::BasicBlock new_block;
            std::vector<IR::Instruction>::reverse_iterator rit = fun.blocks[idx].instructions.rbegin();
            for (; rit != fun.blocks[idx].instructions.rend(); rit++) {
                
            }
        }
    }
}

enum class Operation {
    ADD,
    ADD_INT,
    SUB,
    MUL,
    DIV,
    EQ,
    GT,
    GEQ,
    AND,
    OR,
    NOT,

    LOAD_ARG,  // LOAD_ARG (VIRT_REG id) <- (LOGICAL index)

    LOAD_FREE_REF,  // LOAD_FREE_REF (VIRT_REG id) <- (LOGICAL index)

    REF_LOAD,   // REF_LOAD (VIRT_REG id) <- (VIRT_REG id)
    REF_STORE,  // REF_STORE (VIRT_REG id) <- (VIRT_REG id)
    REC_LOAD_NAME,
    REC_LOAD_INDX,
    REC_STORE_NAME,
    REC_STORE_INDX,

    ALLOC_REF,
    ALLOC_REC,
    ALLOC_CLOSURE,		// (VIRT_REG id), FUNCTION IDX, #SET_CAPTUREs
    
    SET_CAPTURE,        // SET_CAPTURE NONE <- (LOGICAL index) (VIRT_REG id) (VIRT_REG id)

    SET_ARG,            // SET_ARG NONE <- (LOGICAL index) (VIRT_REG id)
    CALL,               // CALL (VIRT_REG id) <- (LOGICAL num_args) (VIRT_REG id)
    RETURN,

    MOV,
    LOAD_GLOBAL,
    STORE_GLOBAL,
    ASSERT_BOOL,
    ASSERT_INT,
    ASSERT_STRING,
    ASSERT_RECORD,
    ASSERT_CLOSURE,
    ASSERT_NONZERO,

    PRINT,
    INPUT,
    INTCAST,
    
    SWAP,
};