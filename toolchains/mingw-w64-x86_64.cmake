# Sample toolchain file for building for Windows from an Ubuntu Linux system.
#
# Typical usage:
#    *) install cross compiler: `sudo apt-get install mingw-w64`
#    *) cd build
#    *) cmake -DCMAKE_TOOLCHAIN_FILE=~/mingw-w64-x86_64.cmake ..

set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR AMD64)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -static-libgcc -static-libstdc++")
set(TOOLCHAIN_PREFIX x86_64-w64-mingw32)

# cross compilers to use for C, C++ and Fortran
if (EXISTS ${TOOLCHAIN_PREFIX}-gcc-posix)
    set(CMAKE_CXX_COMPILER ${TOOLCHAIN_PREFIX}-gcc-posix)
else()
    set(CMAKE_CXX_COMPILER ${TOOLCHAIN_PREFIX}-gcc)
endif()

if (EXISTS ${TOOLCHAIN_PREFIX}-g++-posix)
    set(CMAKE_CXX_COMPILER ${TOOLCHAIN_PREFIX}-g++-posix)
else()
    set(CMAKE_CXX_COMPILER ${TOOLCHAIN_PREFIX}-g++)
endif()
set(CMAKE_RC_COMPILER ${TOOLCHAIN_PREFIX}-windres)

# target environment on the build host system
set(CMAKE_FIND_ROOT_PATH /usr/${TOOLCHAIN_PREFIX})

# modify default behavior of FIND_XXX() commands
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)
