cmake_minimum_required(VERSION 3.17)
project(concrete-class)

find_package(LLVM REQUIRED)
list(APPEND CMAKE_MODULE_PATH ${LLVM_CMAKE_DIR})
find_package(Clang REQUIRED)
list(APPEND CMAKE_MODULE_PATH ${CLANG_CMAKE_DIR})

add_subdirectory(src)
add_subdirectory(tests)

