#include "buffer.hpp"

void Buffer::release() noexcept {
    if ( ptr_ != nullptr) {
        alloc_->deallocate(ptr_);
        ptr_ = nullptr;
        size_  = 0;
    }
}

Buffer::Buffer(CachingAllocator& alloc, std::size_t size)
    : alloc_(&alloc),
    ptr_(alloc.allocate(size)),
    size_(size){}



Buffer::~Buffer() {
    release();
}

Buffer::Buffer(Buffer &&other) noexcept {
    alloc_ = other.alloc_;
    ptr_ = other.ptr_;
    size_ = other.size_;
    other.ptr_ = nullptr;
    other.size_ = 0;
}

Buffer &Buffer::operator=(Buffer &&other) noexcept {
    if (this == &other) {
        return *this;
    }
    this->release();
    alloc_ = other.alloc_;
    ptr_ = other.ptr_;
    size_ = other.size_;
    other.ptr_ = nullptr;
    other.size_ = 0;
    return *this;
}
