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
