#include "caching_allocator.hpp"
#include <new>
#include <cstdlib>
#include <iostream>
#include <stdexcept>


std::size_t CachingAllocator::round_up(std::size_t size) {
    size_t newSize = (size + kAlignment - 1)/ kAlignment * kAlignment;
    if (size == 0) {
        return kAlignment;
    }
    return newSize;
}

void *CachingAllocator::allocate(std::size_t size) {
    size_t rounded = round_up(size);
    auto iterator = free_blocks_.find(rounded);
    void* ptr;
    if (iterator != free_blocks_.end() && !iterator->second.empty()) {
        ptr = iterator->second.back();
        iterator->second.pop_back();
        stats_.hits++;
        stats_.cached_bytes -= rounded;
        stats_.allocated_bytes += rounded;
    }
    else {
        ptr = std::malloc(rounded);
        if ( ptr == nullptr) {
            throw std::bad_alloc();
        }
        stats_.misses++;
        stats_.allocated_bytes += rounded;
    }
    allocated_[ptr] = rounded;
    return ptr;
}

void CachingAllocator::deallocate(void *ptr) {
    if (ptr == nullptr) {
        return;
    }

    auto iterator = allocated_.find(ptr);
    if (iterator != allocated_.end()) {
        size_t size = iterator->second;
        free_blocks_[size].push_back(ptr);
        allocated_.erase(iterator);
        stats_.cached_bytes += size;
        stats_.allocated_bytes -= size;
    }
    else {
        throw std::invalid_argument("pointer was not allocated by this allocator or was already freed");
    }
}

CachingAllocator::~CachingAllocator() {
    empty_cache();
    if (!allocated_.empty()) {
        std::cerr << "CachingAllocator: " << allocated_.size()
                  << " block(s) were never deallocated\n";
    }
    for (auto& [block, size] : allocated_) {
            std::free(block);
    }
}

void CachingAllocator::empty_cache() {
    for (auto& [size, blocks] : free_blocks_) {
        for (void* block : blocks) {
            std::free(block);
        }
    }
    free_blocks_.clear();
    stats_.cached_bytes = 0;
}

Stats CachingAllocator::stats() const {
    return stats_;
}


