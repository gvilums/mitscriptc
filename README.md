# `mitscriptc` - A JIT compiler for `mitscript`

`mitscriptc` is a just-in-time compiler and runtime for executing `mitscript` programs.

It was developed as a course project for MIT course `6.818` in the fall of 2021 by [Georgijs Vilums](https://github.com/gvilums) and [Jakob Nogler](https://github.com/jakob-nogler).

## `mitscript`

The `mitscript` language is in many ways similar to Python and JavaScript. It supports the following features, among others:
- Dynamic typing, with integers, strings, booleans and objects as fundamental types.
- Functions and closures that can capture their environment.
- Automatic memory management through garbage collection.

See the following example program:
```js
Point = fun(x, y){
    this = {
        x: x;
        y: y;
        print: fun() {
            print("Point(" + this.x + ", " + this.y + ")");
        };
        add: fun(other) {
            return Point(this.x + other.x, this.y + other.y);
        }; 
    };
    return this;
};

p1 = Point(5,5);

// prints Point(5, 5)
p1.print();

p2 = Point(3, 2);
p3 = p1.add(p2);

// prints Point(8, 7)
p3.print();

```

## Building and Running
Build `mitscriptc` (CMake and a C++ compiler are required)
```bash
mkdir build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j`nproc`
```

Run the game of life benchmark program:
```bash
./build/mitscriptc ./test/bench/life.mit < ./test/bench/life.mit.input
```
Or more generally:
```bash
mitscriptc <program.mit>
```

## Internals
The following is a brief overview of the different moving parts in the compiler and virtual machine:
- The first step of the execution process is to translate an `mitscript` program into a high level intermediate representation in static single assignment form, such that it becomes suitable for further processing.
- Various optimizations such as constant propagation and type analysis are performed to specialize instructions, improving performance.
- Next, register allocation is performed to map the intermediate representation from virtual registers to machine registers.
- The intermediate representation with machine registers is translated into x86-64 assembly.
- Arguments are initialized and control is transfered to the generated code.
- The runtime system performs garbage collection and handles any I/O.

More detailed information can be found in the [writeup](/writeup.pdf).
