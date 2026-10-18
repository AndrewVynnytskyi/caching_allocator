#pragma once
#include <cstddef>
#include <map>
#include <unordered_map>
#include <vector>

struct Stats {
    std::size_t allocated_bytes = 0;
    std::size_t cached_bytes = 0;
    std::size_t hits = 0;
    std::size_t misses = 0;
};

class CachingAllocator {
public:
    CachingAllocator() = default;
    ~CachingAllocator();

    CachingAllocator(const CachingAllocator&) = delete;
    CachingAllocator& operator = (const CachingAllocator&) = delete;

    void* allocate(std::size_t size);
    void deallocate(void* ptr);
    void empty_cache();
    Stats stats() const;

    static std::size_t round_up(std::size_t size);

private:
    static constexpr std::size_t kAlignment = 512;

    Stats stats_;
    std::map<std::size_t, std::vector<void*>> free_blocks_;
    std::unordered_map<void*, std::size_t> allocated_;
};
