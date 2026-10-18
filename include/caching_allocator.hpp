#pragma once
#include <cstddef>
#include <map>
#include <unordered_map>
#include <vector>

class CachingAllocator {
public:
    CachingAllocator() = default;
    ~CachingAllocator();

    CachingAllocator(const CachingAllocator&) = delete;
    CachingAllocator& operator = (const CachingAllocator&) = delete;

    void* allocate(std::size_t size);
    void deallocate(void* ptr);

    static std::size_t round_up(std::size_t size);

private:
    static constexpr std::size_t kAlignment = 512;

    std::map<std::size_t, std::vector<void*>> free_blocks_;
    std::unordered_map<void*, std::size_t> allocated_;
};
