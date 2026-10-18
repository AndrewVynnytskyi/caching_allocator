# caching-allocator

A small C++17 memory allocator that caches freed blocks and reuses them instead of returning them to the system. It is a learning project modeled on the idea behind PyTorch's [CUDA caching allocator](https://pytorch.org/docs/stable/notes/cuda.html#memory-management), which keeps freed GPU memory around so later allocations don't need expensive `cudaMalloc` calls. This version works with ordinary host memory (`std::malloc`) and implements only the core idea.

## Features

- `CachingAllocator` with `allocate`, `deallocate` and `empty_cache`
- Request sizes rounded up to a multiple of 512 bytes, with freed blocks reused by size
- Statistics: allocated bytes, cached bytes, cache hits and misses
- Throws `std::invalid_argument` on double free or on a pointer the allocator doesn't own
- `Buffer`: a move-only RAII wrapper that returns its block to the allocator when it goes out of scope

## Build and test

Requires CMake 3.10+ and a C++17 compiler.

```bash
cmake -B build && cmake --build build && ctest --test-dir build
```

All tests are in a single executable, `test_allocator`, so ctest reports one test. To see the output, run `./build/test_allocator` directly or use `ctest --test-dir build -V`.

## Usage

```cpp
#include "caching_allocator.hpp"
#include "buffer.hpp"
#include <iostream>

int main() {
    CachingAllocator alloc;

    void* p = alloc.allocate(1000);   // rounded up to 1024, new malloc (miss)
    alloc.deallocate(p);              // block goes into the cache
    void* q = alloc.allocate(800);    // also 1024, reuses the cached block (hit)
    alloc.deallocate(q);

    {
        Buffer buf(alloc, 4096);      // returned to the cache at end of scope
        // use buf.data(), buf.size()
    }

    Stats s = alloc.stats();
    std::cout << "hits=" << s.hits << " misses=" << s.misses
              << " cached=" << s.cached_bytes << "\n";

    alloc.empty_cache();              // free all cached blocks
}
```

## How it works

- **Rounding.** Every request is rounded up to the next multiple of 512 bytes. A size of 0 becomes 512. A size so large that rounding would overflow `size_t` throws `std::bad_alloc`. Sizes in the same 512-byte step share one bucket, so a freed 100-byte block can serve a later 300-byte request.
- **Free blocks by size.** Freed blocks are kept in a `std::map` from rounded size to a list of pointers. `allocate` looks up the exact rounded size. If a block is there, it is reused (a hit); otherwise a new block is taken from `std::malloc` (a miss).
- **Ownership tracking.** Live blocks are kept in an `unordered_map` from pointer to size. `deallocate` uses it to find the block's size and to reject pointers that aren't live.
- **Stats.** `stats()` returns `allocated_bytes` (in live blocks), `cached_bytes` (in the cache), `hits` and `misses`. Byte counts use rounded sizes.
- **Releasing memory.** Memory is not returned to the system on `deallocate`. Only `empty_cache()` and the destructor do that. The destructor also frees blocks still in use and prints a warning to `stderr` with their count.
- **Buffer.** `Buffer` allocates in its constructor and deallocates in its destructor. It cannot be copied. Moving transfers the block, and move assignment first returns the target's old block.

## Limitations

- **Not thread-safe.** There is no locking, so one allocator must not be used from several threads at once.
- **No block splitting or coalescing.** A request is served only by a cached block of exactly the same rounded size. A cached 4096-byte block is not split to serve a 512-byte request, and adjacent free blocks are not merged. PyTorch's allocator does both, and it also separates small and large allocation pools.
- **No memory limit or automatic trimming.** The cache grows until `empty_cache()` is called.
- **The 512 is a size step, not an alignment.** Blocks have the alignment `std::malloc` gives (`alignof(std::max_align_t)`), not 512 bytes.
- **Host memory only.** There is no GPU or device memory support.
- **Buffer must not outlive its allocator.** `Buffer` keeps a raw pointer to the `CachingAllocator`.
