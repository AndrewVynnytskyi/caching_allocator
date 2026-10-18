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
    }
    else {
        ptr = std::malloc(rounded);
        if ( ptr == nullptr) {
            throw std::bad_alloc();
        }
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
        free_blocks_[iterator->second].push_back(ptr);
        allocated_.erase(iterator);
    }
    else {
        throw std::invalid_argument("pointer was not allocated by this allocator or was already freed");
    }
}

CachingAllocator::~CachingAllocator() {
    for (auto& [size, blocks] : free_blocks_) {
        for (void* block : blocks) {
            free(block);
        }
    }
    if (!allocated_.empty()) {
        std::cerr << "CachingAllocator: " << allocated_.size()
                  << " block(s) were never deallocated\n";
    }
    for (auto& [block, size] : allocated_) {
            free(block);
    }

}
