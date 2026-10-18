#include "caching_allocator.hpp"
#include <iostream>
#include <stdexcept>

#define CHECK(cond)                                                      \
    do {                                                                 \
        if (!(cond)) {                                                   \
            std::cerr << "FAIL: " #cond " (line " << __LINE__ << ")\n";  \
            return 1;                                                    \
        }                                                                \
    } while (0)

int test_round_up() {
    CHECK(CachingAllocator::round_up(0) == 512);
    CHECK(CachingAllocator::round_up(1) == 512);
    CHECK(CachingAllocator::round_up(512) == 512);
    CHECK(CachingAllocator::round_up(513) == 1024);
    return 0;
}

int test_reuses_freed_block() {
    CachingAllocator alloc;
    void* a = alloc.allocate(1000);
    alloc.deallocate(a);
    void* b = alloc.allocate(1000);
    CHECK(a == b);  // той самий блок з кешу, без нового malloc
    alloc.deallocate(b);
    return 0;
}

int test_reuses_across_sizes_in_same_bucket() {
    CachingAllocator alloc;
    void* a = alloc.allocate(100);
    alloc.deallocate(a);
    void* b = alloc.allocate(300);  // обидва округлюються до 512
    CHECK(a == b);
    alloc.deallocate(b);
    return 0;
}

int test_live_blocks_are_distinct() {
    CachingAllocator alloc;
    void* a = alloc.allocate(1000);
    void* b = alloc.allocate(1000);
    CHECK(a != b);  // обидва зайняті, тож мають бути різні
    alloc.deallocate(a);
    alloc.deallocate(b);
    return 0;
}

int test_double_free_throws() {
    CachingAllocator alloc;
    void* a = alloc.allocate(64);
    alloc.deallocate(a);
    try {
        alloc.deallocate(a);
    } catch (const std::invalid_argument&) {
        return 0;
    }
    std::cerr << "FAIL: double free did not throw\n";
    return 1;
}

int main() {
    int failures = test_round_up() + test_reuses_freed_block() +
                   test_reuses_across_sizes_in_same_bucket() +
                   test_live_blocks_are_distinct() + test_double_free_throws();
    if (failures == 0) std::cout << "All tests passed\n";
    return failures == 0 ? 0 : 1;
}