# Virtual Machine

## Design
### Value Type
A type 'Value' represents the different kinds of values that can found on the stack of an executing MITscript program. This includes the program values themselves (such as integers, booleans, etc.), but is also extended by pointers to functions (which are required for alloc_closure instructions), pointers to objects on the program heap (which can either be a value, a closure or a record), and unsinged machine size integers (size_t) which are necessary for managing the stack. Values are implemented as a tagged union to avoid pointer indirection and provide constructors/destructors and access methods.

## HeapObject Type
This type is a tagged union of either a Value, a Closure or a Record object, which should conceptually be allocated on the heap. It provides basic accessor methods. The design is further explained in the documentation for the garbage collector.

### Stack
The virtual machine has a single callstack for all functions, similar to the callstack on a real machine. This avoids having to allocate a new stackframe whenever a function is called. Instead of allcating stackframes, at the time of the function call, the current execution context (consisting of a pointer to a Function structure, the instruction pointer, the number of locals, and the current base/frame-pointer) is pushed on the stack, and the corresponding values for the called function are loaded. Then, once a function returns from a call, it can read off the return context from the stack.

### Interpreter Loop
The bulk of the implementation is located in the step function of the interpreter, which executes a single instruction, or throws an error if there are no instructions left to execute.

### Native Functions
In addition to a vector of references and a pointer to its corresponding function object, each closure object also carries a tag representing its kind. If the kind is DEFAULT, the closure is user-defined and is executed as usual. If, however, the kind is PRINT, INPUT or INTCAST, this is detected when the closure is called, and, instead of the usuall call procedure, native code specific to each function is executed. These special closures are created once when the virtual machine is initialized. Then, whenever a closure is allocated, the vm checks if the function pointer at the top of the stack corresponds to one of the builtins. If that is the case, instead of allocating a new closure, a pointer to the previously allocated builtin closure is returned instead.

# Collaboration
The virtual machine was implemented by Georgijs. The tests were designed by Jakob.