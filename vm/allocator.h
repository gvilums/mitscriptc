#pragma once

#include <cstdlib>
#include <iostream>
#include <limits>
#include <new>
#include <vector>

namespace Allocation {

static size_t total_alloc = 0;

template <class T>
struct TrackingAlloc {
    using value_type = T;

    //   static size_t total_alloc;

    TrackingAlloc() = default;
    template <class U>
    constexpr TrackingAlloc(const TrackingAlloc<U>&) noexcept {}

    [[nodiscard]] auto allocate(std::size_t n) -> T* {
        if (n > std::numeric_limits<std::size_t>::max() / sizeof(T)) {
            throw std::bad_array_new_length();
        }

        total_alloc += sizeof(T) * n;

        if (auto p = static_cast<T*>(std::malloc(n * sizeof(T)))) {
            return p;
        }

        throw std::bad_alloc();
    }

    void deallocate(T* p, std::size_t n) noexcept {
        total_alloc -= sizeof(T) * n;
        std::free(p);
    }
};


}  // namespace alloc

template <class T, class U>
auto operator==(const Allocation::TrackingAlloc<T>&, const Allocation::TrackingAlloc<U>&) -> bool {
    return true;
}
template <class T, class U>
auto operator!=(const Allocation::TrackingAlloc<T>&, const Allocation::TrackingAlloc<U>&) -> bool {
    return false;
}