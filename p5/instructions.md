# IR instruction set specification

## Operands
- Arg(i) : i-th argument to the function
- Reg(i) : i-th virtual register (local to function)
- Imm(value) : immediate value (encoded as uin64_t)

## General Instruction Format
`instr param_1 param_2 ... param_n out`

## Instructions
### Arithmetic/Logic
add p1 p2 out
add_int p1 p2 out
sub p1 p2 out
mul p1 p2 out
div p1 p2 out
eq p1 p2 [out]
gt p1 p2 [out]
geq p1 p2 [out]
and p1 p2 [out]
or p1 p2 [out]
not p1 [out]


### References
ref_load p1 out
ref_store p1 p2 out         // p1 is reference
rec_load_name p1 p2 out     // p1 is rec, p2 is Imm(value)
rec_load_indx p1 p2 out     // p1 is rec
rec_store_name p1 p2 p3     // p1 is rec, p2 is Imm(value), p3 is value to store
rec_store_indx p1 p2 p3     // p1 is rec, p3 is value to store

### Allocation
alloc_ref out
alloc_record out
alloc_closure p1 p2 ... p{n+1} out  // p1 is function index, p2 ... p{n+1} are captures

### Control Flow
call p1 p2 ... p{n+1} [out] // p1 is callable, p2 ... p{n+1} are arguments
return p1

### Misc
load_global p1 out          // p1 is index
store_global p1 p2			// p1 is index, p2 value
assert_type p1 TYPE			// asserts that p1 is of type TYPE
assert_nonzero p1
phi [BLOCK p1] [BLOCK p2] ... [BLOCK pn] out