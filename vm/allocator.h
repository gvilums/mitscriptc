#pragma once

#include <cstdlib>
#include <iostream>
#include <limits>
#include <new>
#include <vector>
#include <string>

namespace Allocation {

const size_t ALLOC_OVERHEAD = 32;

static size_t total_alloc = 0;

template <class T>
struct TrackingAlloc {
    using value_type = T;

    //   static size_t total_alloc;

    TrackingAlloc() = default;
    template <class U>
    constexpr TrackingAlloc(const TrackingAlloc<U>& _) noexcept {}

    [[nodiscard]] auto allocate(std::size_t n) -> T* {
        if (n > std::numeric_limits<std::size_t>::max() / sizeof(T)) {
            throw std::bad_array_new_length();
        }

        total_alloc += (sizeof(T) * n);//+ ALLOC_OVERHEAD);

        if (auto p = static_cast<T*>(std::malloc(n * sizeof(T)))) {
            return p;
        }

        throw std::bad_alloc();
    }

    void deallocate(T* p, std::size_t n) noexcept {
        total_alloc -= (sizeof(T) * n);// + ALLOC_OVERHEAD);
        std::free(p);
    }
    
    template<class U>
    bool operator==(const TrackingAlloc<U>& _) {
        return true;
    }

    template<class U>
    bool operator!=(const TrackingAlloc<U>& _) {
        return false;
    }
};

using TrackedString = std::basic_string<
    char,
    std::char_traits<char>,
    TrackingAlloc<char>
>;


}  // namespace alloc