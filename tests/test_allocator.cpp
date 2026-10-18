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

int test_stats_track_allocations() {
    CachingAllocator alloc;
    void* a = alloc.allocate(1000);  // округлиться до 1024
    Stats s = alloc.stats();
    CHECK(s.allocated_bytes == 1024);
    CHECK(s.cached_bytes == 0);
    CHECK(s.misses == 1);
    CHECK(s.hits == 0);

    alloc.deallocate(a);
    s = alloc.stats();
    CHECK(s.allocated_bytes == 0);
    CHECK(s.cached_bytes == 1024);

    void* b = alloc.allocate(1000);
    s = alloc.stats();
    CHECK(s.hits == 1);
    CHECK(s.cached_bytes == 0);
    alloc.deallocate(b);
    return 0;
}

int test_empty_cache_frees_only_cached_blocks() {
    CachingAllocator alloc;
    void* live = alloc.allocate(512);
    void* freed = alloc.allocate(2048);
    alloc.deallocate(freed);

    alloc.empty_cache();
    Stats s = alloc.stats();
    CHECK(s.cached_bytes == 0);
    CHECK(s.allocated_bytes == 512);  // живий блок не зачеплено

    alloc.allocate(2048);             // кеш порожній -> має бути miss
    CHECK(alloc.stats().misses == 3);
    alloc.deallocate(live);
    return 0;
}

int test_foreign_pointer_throws() {
    CachingAllocator alloc;
    int x = 0;
    try {
        alloc.deallocate(&x);
    } catch (const std::invalid_argument&) {
        return 0;
    }
    std::cerr << "FAIL: foreign pointer did not throw\n";
    return 1;
}


int main() {
    int failures = test_round_up() + test_reuses_freed_block() +
                   test_reuses_across_sizes_in_same_bucket() +
                   test_live_blocks_are_distinct() + test_double_free_throws() +
                   test_stats_track_allocations() +
                   test_empty_cache_frees_only_cached_blocks() +
                   test_foreign_pointer_throws();
    if (failures == 0) std::cout << "All tests passed\n";
    return failures == 0 ? 0 : 1;
}

