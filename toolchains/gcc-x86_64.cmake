# Sample toolchain file for building for Linux from a Linux system.
#
# Typical usage:
#    *) cd build
#    *) cmake -DCMAKE_TOOLCHAIN_FILE=../toolchains/gcc-x86_64.cmake ..

set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR AMD64)

# compilers to use for C and C++
set(CMAKE_C_COMPILER gcc)
set(CMAKE_CXX_COMPILER g++)
