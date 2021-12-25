# `mitscriptc` -  A JIT compiler for the `mitscript` language

This repository hosts an implementation of a custom virtual machine for `mitscript`, which performs extensive just-in-time compilation to achieve remarkable performance.

## Compiler
The first step of the execution process is to translate an `mitscript` program into a high level intermediate representation suitable for further processing.

## Optimizer
Various optimizations such as constant propagation and type analysis are performed to specialize instructions, improving performance.

## Register Allocator
This step maps the intermediate representation from virtual registers to machine registers.

## Code Generator
Finally, the intermediate representation with machine registers is translated into x86 assembly, and executed.

## Further Info
More detailed information can be found in `writeup.pdf`
