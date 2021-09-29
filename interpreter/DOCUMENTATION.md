# The Parser

The parser has been implemented following strictly the instructions of the first assignment. The code of the parser is contained in the following files:

## parsecode.cpp

This files contains the actual parsing methods. For each rule in the grammar of MITScript it is possible to find a corresponding function that tries to apply that rule. Then implementation takes also care to implement the associativeness, by returning an Abstract Syntax Tree (AST) strucured according to it. It is worth to mention, that the function for parsing a Call and Assignment are invoked when the LHS was already parsed (and thus it is passed as an argument). This happens, as Unit and Statement contain two different rules having as first syntactic element a LHS. Once, the LHS was parsed, we can decide which of the two rules use.

## AST.h

This file contains all nodes which is possible to find the AST (again, it follows exaclty the structure of the problem statement). All nodes are either subclasses or `Statment` of `Expression` (which are subclasses of `AST_node`). The only exception is `Call`, which is a subclass of both. In fact, function calls can appear either as an statement or as an expression. `StringConstant` is used to hold both strings (between double quotes) and variables names (which do not contain any double quote). 

## MITScript.g

Contains the definitions for the lexer.

## PrettyPrinter.h

A subclass of `Visitor` which prints the code according to some canonical form.


# The Interpreter
 
 In order to create the interpreter, six new header fiels were added. We will go through them, one by one.

 ## 1. Exceptions.h

 Contains the four exceptions mentioned in the language semantics.

 ## 2. Globals.h

 Defines a visitor class that finds all global statements in the body of a function following the semantics of the _globals_ function (Figure 8 of the language specification). All variables are store in the vector `globals`, whose content is returned by invoking `getGlobals`. 

  ## 3. Assigns.h

 Defines a visitor class that finds all assigned variables (that do not contain any field deference or index access) in the body of a function following the semantics of the _assigns_ function (Figure 7 of the language specification). All variables are store in the vector `variables`, whose content is returned by invoking `getVariables`.

 ## 4. Frame.h

 Defines a class for frames. The class declaration is contained in Value.h in order to avoid cyclic dependecies among the libraries. It contains the following fields:

- `parent`: a pointer to the parent frame.  
- `global`: a pointer to the global frame.
- `gvariables`: a set of variables names which corresponds to $X_g$ of the language specification (together with `global` it forms the global field).
- `dict`: a mapping from variables names to addresses of values.

The class also contains methods to access and modify some of these fields:

- `addGlobalVariables`: adds a set variables (stored in a `std::vector`) to the global variables set.
- `insert`: Insert a new element in the mapping `dict`.
- `count`: check whether a variable is in the domain of the mapping.
- `get `: get the address where a certain variable maps to.

The frame class also contains the functions `lookupWrite` and `lookupRead`, which work exactly as specified in the language specification.

## 5. Value.h

Defines a value class and also all subclasses of it. For each subclass, `Value` contains a virtual method to check whether an instance belongs to that subclass. It returns `false` for all instances, except for the given subclass where the return value is overwritten to `true`. Morevoer, each value class also defines a method to convert it to a string, a method that implements equality and a method to check whether another value class belongs to the same subclass. The subclasses are:
- `Integer`: its value is stored in `int value`. 
- `Boolean`: its value is stored in `bool value`.
- `String`: its value is stored in `string value`.
- `Function`: the fields of this subclass are: `frame` (a pointer to the frame where it was defined), `block` (a pointer to the statement that defines the function) and `arguments` (a list of strings that describe the arguments).
- `Record`: the records are stored in a `std::map` named `dict` that maps the field's name to pointers to the values.
- `None`: this class has no field.

For some subclasses it is possible to find functions that access and modify some of their fields.

## 6. Interpreter.h

In this file we can find the interpreter class which is a subclass of visitor. The return value of the visitor methods is stored implicitly in `rval_` and `return_`. The class has the following fields:
- `return_`: a boolean value that indicates whether the return value of the visitor methods is of type return.
- `rval_`:  a pointer to the value returned implicitly by the visitor class.
- `stack`: the stack keeping all frames.
- `global_`: a pointer to the global frame.
- `none_`: the pointer to the none value initialized at the beginning.
- `print_`, `intcast_` and `input_`: pointers to the values that store the predefined functions.

Let us also give a short description of the most verbose methods of this class:
- `void visit(AST::Call& expr)`: This is the visit method for function calls. It first checks whether the call matches the number of arguments of the function. Then, we handle the predefined functions. If the pointer to the function matches one of the pointer of the predefined functions, we call custom methods of the class that simulate their behavior. Otherwise, we follow the semantics of the function call in the language specification. At the end of the call we check whether the function returned a value by controlling the `return_` flag.
- `void visit(AST::Assignment& expr)`: For assignments we have first to check how our LHS looks like. If it not a field dereference nor a index expression we can direclty call `lookupWrite` of our current frame and find a proper location where to write the evaluated right hand side in. Otherwise, we have first to evaluate our base expression and then perform the assignment according the rules in the language specification.
- `visit(AST::BinaryExpression& expr)`: Accordingly to the the operation stored in the `op` field of the statement, we behave differently. The implemention for `+`, `-`, `\`, `&`, `|`, `<`, `<=`, `>` and `>=` is easier, since they can operater over values of just one type. After, checking for consistency, we evaulate the result and store it in to a new value. For `==` we have to check first if the two values are of the same type. If this is the case, we check for equality using the methods defined in 5. Lastly, for `+` we have to do a case split depending on whether we have to deal with at least one string or just integers.  
- `void visit(AST::StringConstant& expr)`: Since both string constants and variables names are stored here, we have first to check whether the first character of the string is a double quote. If this is the case, we create a new string constant. Otherwise, we call `lookupRead` for that variable.


All other visitor function are much more straightforward, since they follow directly the language specification. 

## interpreter.cpp

Now, we are ready to put everything together tu run the interpreter. We first call the parser, whose code can be found in `parsercode.cpp`. Then, we call the interpreter on the resulting abstract syntax tree. When doing this, we have to be careful, to catch any exception and print their description on failure.


