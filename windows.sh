cd build
cmake --build . --target distclean
cmake .. --toolchain ../toolchains/mingw-w64-x86_64.cmake
