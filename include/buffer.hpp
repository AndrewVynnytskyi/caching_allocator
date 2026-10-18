#pragma once
#include "caching_allocator.hpp"

class Buffer {
public:
    Buffer(CachingAllocator& alloc, std::size_t size);
    ~Buffer();

    Buffer(const Buffer&) = delete;
    Buffer& operator=(const Buffer&) = delete;

    Buffer(Buffer&& other) noexcept;
    Buffer& operator=(Buffer&& other) noexcept;

    void* data() const {return  ptr_;}
    std::size_t size() const {return  size_;}

private:
    void release() noexcept;
    CachingAllocator* alloc_ = nullptr;
    void* ptr_ = nullptr;
    std::size_t size_ = 0;
};