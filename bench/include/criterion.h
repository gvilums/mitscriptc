#pragma once
#ifndef CRITERION_H
#define CRITERION_H

#ifdef __cplusplus
extern "C"
{
#endif

    /// Benchmark a function.
    /// The function will be called repeatedly, with `payload` as its argument.
    void criterion_benchmark(const char *name, void *payload, void (*cb)(void *));

    /// Print a summary after running all benchmarks.
    void criterion_summary();

#define CRITERION_USE()

#ifdef __cplusplus
} // extern "C"

namespace criterion
{

namespace impl
{

template <typename Bench>
static void callback(void *bench)
{
    static_cast<Bench *>(bench)->operator()();
}
} // namespace impl

template <typename Bench>
inline void benchmark(const char *name, Bench &bench)
{
    criterion_benchmark(name, (void *)&bench, impl::callback<Bench>);
}

template <typename Bench>
inline void benchmark(const char *name, Bench &&bench)
{
    criterion_benchmark(name, (void *)&bench, impl::callback<Bench>);
}

inline void summary()
{
    criterion_summary();
}

} // namespace criterion

#endif // __cplusplus

#endif // CRITERION_H