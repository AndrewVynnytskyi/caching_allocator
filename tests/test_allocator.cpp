#include "caching_allocator.hpp"
#include <iostream>
#include <limits>
#include <new>
#include <stdexcept>
#include <utility>
#include <vector>

#include "buffer.hpp"

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

int test_buffer_returns_block_on_scope_exit() {
    CachingAllocator alloc;
    {
        Buffer buf(alloc, 1000);
        CHECK(buf.data() != nullptr);
        CHECK(alloc.stats().allocated_bytes == 1024);
    }  // тут викликається деструктор Buffer
    CHECK(alloc.stats().allocated_bytes == 0);
    CHECK(alloc.stats().cached_bytes == 1024);
    return 0;
}

int test_buffer_move_constructor_transfers_ownership() {
    CachingAllocator alloc;
    {
        Buffer a(alloc, 1000);
        void* p = a.data();
        Buffer b(std::move(a));
        CHECK(b.data() == p);
        CHECK(a.data() == nullptr);  // a більше нічим не володіє
    }  // обидва деструктори, але блок повертається лише один раз
    CHECK(alloc.stats().cached_bytes == 1024);
    return 0;
}

int test_buffer_move_assignment_releases_old_block() {
    CachingAllocator alloc;
    {
        Buffer a(alloc, 1000);   // 1024
        Buffer b(alloc, 2000);   // 2048
        void* pa = a.data();
        b = std::move(a);
        CHECK(b.data() == pa);
        CHECK(a.data() == nullptr);
        CHECK(alloc.stats().cached_bytes == 2048);  // старий блок b повернувся
    }
    CHECK(alloc.stats().cached_bytes == 1024 + 2048);
    return 0;
}

int test_buffers_in_vector() {
    CachingAllocator alloc;
    {
        std::vector<Buffer> buffers;
        for (int i = 0; i < 10; ++i) {
            buffers.emplace_back(alloc, 512);  // вектор розширюється і переміщує Buffer
        }
        CHECK(alloc.stats().allocated_bytes == 10 * 512);
    }
    CHECK(alloc.stats().allocated_bytes == 0);
    CHECK(alloc.stats().cached_bytes == 10 * 512);
    return 0;
}

int test_huge_size_throws_bad_alloc() {
    CachingAllocator alloc;
    try {
        alloc.allocate(std::numeric_limits<std::size_t>::max());
    } catch (const std::bad_alloc&) {
        return 0;
    }
    std::cerr << "FAIL: huge size did not throw bad_alloc\n";
    return 1;
}

int main() {
    int failures = test_round_up() + test_reuses_freed_block() +
                   test_reuses_across_sizes_in_same_bucket() +
                   test_live_blocks_are_distinct() + test_double_free_throws() +
                   test_stats_track_allocations() +
                   test_empty_cache_frees_only_cached_blocks() +
                   test_foreign_pointer_throws() +
                   test_buffer_returns_block_on_scope_exit() +
                   test_buffer_move_constructor_transfers_ownership() +
                   test_buffer_move_assignment_releases_old_block() +
                   test_buffers_in_vector() +
                   test_huge_size_throws_bad_alloc();
    if (failures == 0) std::cout << "All tests passed\n";
    return failures == 0 ? 0 : 1;
}

