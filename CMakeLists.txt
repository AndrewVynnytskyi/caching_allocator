cmake_minimum_required(VERSION 3.10)
project(caching_allocator CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(caching_allocator src/caching_allocator.cpp
            src/buffer.cpp)
target_include_directories(caching_allocator PUBLIC include)

add_executable(demo main.cpp)
target_link_libraries(demo PRIVATE caching_allocator)

enable_testing()
add_executable(test_allocator tests/test_allocator.cpp)
target_link_libraries(test_allocator PRIVATE caching_allocator)
add_test(NAME test_allocator COMMAND test_allocator)

if(MSVC)
    target_compile_options(caching_allocator PRIVATE /W4)
else ()
    target_compile_options(caching_allocator PRIVATE -Wall -Wextra)
endif ()