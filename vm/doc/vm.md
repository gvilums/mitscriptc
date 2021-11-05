# Virtual Machine

## Design
### Code Generation
The compiler visits the whole AST tree using the visitor pattern. While doing so, it keeps track of the current function and some values associated to it. Among them there are a boolean flag indicating us whether we are in the global scope and two sets (`globals_` and `ref_`) storing the current reference variables and global variables.
For each node it produces the code which corresponds to the operation stored in that node. We will shortly describe the code of the most verbose functions:
- `AST::Assignment`: we perform a case distinction on what the LHS is. If it is a string constant then we do the variable resolution. If we are in the global scope we add the variable to the globals and use StoreGlobal. Otherwise, if it is contained in the `local_references_vars` we store it in the corresponding reference. Lastly, if it is in `local_vars_` we use StoreLocal. The next case is when we have a FieldDereference. In that case we use FieldStore after storing the filed string and the base expression. Finally, there is also the case of a IndexExpression. There, we use IndexStore after having evaluated the base expression and the index.
- `AST::IfStatement`: we first evaluate the boolean expression. Then, we push an If instruction and a Goto expression whose jump value will be determined after evaluating the first block. If there is also an else statement we push an additional Goto statement whose jump value will be determined after visiting the second statement.
- `AST::WhileLoop`: Similarly as before we first evaluate the boolean expression. Then, we add an If instruction and a Goto which we will be useful in the case the loop ends. At the end of the cose of the block we will add another Goto to jump back to the boolean expression.
- `AST::FunctionDeclaration`: we store all information relative to the current function in temporary variables. That includes the function pointer, a boolean flag to know whether we are in the global scope, the current global and reference variables. Then, we create a new Function and we push it to the functions of the old function. As next we perform variable resolution. As this is the most complicated part it is possible to find the details of it in the next section. Lastly, we load the function and alloc the closure in the old function. To handle non returning function we simply return none by default at the end of each function (in the case we will return earlier we will not reach that point).
- `AST::BinaryExpression`: we visit both expression and then we add the expression contained in the node.
- `AST::StringConstant`: in the case we are dealing with a variable, we have to read from the right variable. If we are in the global scope we add it to names. Then we use different operations based on whether we find the variables in `free_vars_`, `local_reference_vars_` or in `names_`. Notice, if a variable that we are reading was not used in an assignement statement so far, then it is placed in `names_` (to handle correctly the errors in the vm).

### Variable Resolution
The class responsibile for name resolution is FreeVariables.h. When we execute it in some scope we find three different types of variables which can be obtained by invoking some corresponding methods:
- `getNotBoundVariables()`: variables that are not global, not assigned and not appear as arguments in a function declaration but are free variables in some function scope which is in the subtree of this tree. When finding these variables we recursively call the visitor class FreeVariables.h. Then, we remove all variables that are declared as globals, appear as lhs in assignements or are arguments of the function.
- `getFreeVariables()`: same as `nb_variables` with the exception that we also add variables that are assigned in the current scope.
- `getCurVariables()`: variables that are assigned in this scope.
The variables resolution then works in the following way (we now refer to code in compiler.h). For each function we keep track of the globals and the reference variables in `globals_` and `ref_`. When a function is declared we execute the visitor class FreeVariables.h in the body of the function. First, we resolve the globals variables. Then, we resolve the free variables. These are all those that are not arguments, not assigned variables and appear as references in some scope above the current. Next, we ass the new references, which are those that are appear in `nb_variables`. Finally, we update `globals_` and `ref_` by erasing the variables that do not stick to their specification anymore.

### Value Type
A type 'Value' represents the different kinds of values that can found on the stack of an executing MITscript program. This includes the program values themselves (such as integers, booleans, etc.), but is also extended by pointers to functions (which are required for alloc_closure instructions), pointers to objects on the program heap (which can either be a value, a closure or a record), and unsinged machine size integers (size_t) which are necessary for managing the stack. Values are implemented as a tagged union to avoid pointer indirection and provide constructors/destructors and access methods.

## HeapObject Type
This type is a tagged union of either a Value, a Closure or a Record object, which should conceptually be allocated on the heap. It provides basic accessor methods. The design is further explained in the documentation for the garbage collector.

### Stack
The virtual machine has a single callstack for all functions, similar to the callstack on a real machine. This avoids having to allocate a new stackframe whenever a function is called. Instead of allcating stackframes, at the time of the function call, the current execution context (consisting of a pointer to a Function structure, the instruction pointer, the number of locals, and the current base/frame-pointer) is pushed on the stack, and the corresponding values for the called function are loaded. Then, once a function returns from a call, it can read off the return context from the stack.

### Interpreter Loop
The bulk of the implementation is located in the step function of the interpreter, which executes a single instruction, or throws an error if there are no instructions left to execute. This implementation is mostly straightforward.

### Native Functions
In addition to a vector of references and a pointer to its corresponding function object, each closure object also carries a tag representing its kind. If the kind is DEFAULT, the closure is user-defined and is executed as usual. If, however, the kind is PRINT, INPUT or INTCAST, this is detected when the closure is called, and, instead of the usuall call procedure, native code specific to each function is executed. These special closures are created once when the virtual machine is initialized. Then, whenever a closure is allocated, the vm checks if the function pointer at the top of the stack corresponds to one of the builtins. If that is the case, instead of allocating a new closure, a pointer to the previously allocated builtin closure is returned instead.


## Difficulties
The most difficoult part was the variable resolution. We agreed that we should always try to use put a variable in `local_vars_` before assigning it to `local_reference_vars_`. This was firstly implemented naively without caring to much about producing elegant (and most importantly readable) code. This was more difficoult than expected, so we decided the code at it was to avoid to have similar issues when trying to write a more readable version of the code. 


## Contribuition
The compiler was implemented by Jakob. The vm interpreter was implemented by Georgijs.