# benchmarking tools for 6.s081

This directory contains tools for benchmarking your code.

Based on the [criterion-c](https://gitlab.com/kazimuth/criterion-c) library:
> A dead-simple, statistically reliable benchmarking library for C and C++.
> It automatically warms-up your code, filters outliers, and performs statistical tests to see if the performance of your code has changed between runs.

# usage

Use in c++:

`bench.cpp`:
```cpp
#include "criterion.h"

int main() {
    std::string x("hello, ");
    criterion::benchmark("example-benchmark", [&]() { // c++11 closure syntax
        std::string y("world!");

        // use "volatile" to prevent dead code elimination from removing benchmark code
        volatile std::string result = x + y;
    });
    criterion::benchmark("other-benchmark", [&]() {
        volatile int x = 3 + 5; 
    });
    criterion::summary();
}
```

You'll need to add commands like the following to your Makefiles: (organize however you wish)
```sh

# building `bench` executable from `bench.cpp`:

# include the `bench/include` directory                   (-I ../bench/include)
# add the `bench/lib` directory to the linker search path (-L ../bench/lib)
# link the criterion, pthread, math, and dl libraries     (-lcriterion -lpthread -lm -ldl)
# make sure you build in release mode!                    (-O3)
# make sure you use at least -std=c++11 as well           (-std=c++11)

c++ bench.cpp -o bench -I ../bench/include -L ../bench/lib \
     -lcriterion -lpthread -lm -ldl -O3 -std=c++11 # other CFLAGS...

# or, in two steps:
c++ bench.cpp -o bench.o -O3 -std=c++11 -I ../bench/include -L ../bench/lib \
     -lcriterion -lpthread -lm -ldl -O3 -std=c++11 
c++ bench.o -o bench -L ../bench/lib \
     -lcriterion -lpthread -lm -ldl # other CFLAGS...

# feel free to include and link in whatever parts of your code you want to benchmark.

# run a single benchmark:
./bench example-benchmark
# run all benchmarks:
./bench
# see other command-line arguments:
./bench --help
```

Sample output:

```
Benchmarking example-benchmark
Benchmarking example-benchmark: Warming up for 3.0000 s
Benchmarking example-benchmark: Collecting 100 samples in estimated 5.0001 s (146M iterations)
Benchmarking example-benchmark: Analyzing
example-benchmark       time:   [33.998 ns 34.607 ns 35.390 ns]
                        change: [-7.5597% -4.0353% -0.3488%] (p = 0.03 < 0.05)
                        Change within noise threshold.
```

Note that criterion-c will track the performance of your code over time;
it will save results in the directory `target/criterion` relative to wherever you run your executable. Add this directory to your .gitignore.

Note also that the library adds about 2 nanoseconds of overhead to benchmarked code. Be aware of this if you're benchmarking very small operations!

# docs

```cpp
namespace criterion {

/// Benchmark a function.
/// - name: a human-readable name for the benchmark
/// - bench: a function / closure / functor to call. Will be called many times. Can have state.
template<typename Bench>
inline void benchmark(const char *name, Bench &bench);

/// Print a summary after all benchmarks have been run.
inline void summary();
};
```

For more documentation, see the [criterion-rs manual](https://bheisler.github.io/criterion.rs/book/criterion_rs.html). (Note that the original library is in Rust; you're using a c++ wrapping of that API.)

Specifically, the [analysis process](https://bheisler.github.io/criterion.rs/book/analysis.html) section of the manual explains the statistical analysis performed by criterion-c.
