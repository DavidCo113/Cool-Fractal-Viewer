cd build
cmake --build . --target distclean
cmake .. --toolchain ../toolchains/gcc-x86_64.cmake # -DCMAKE_FIND_DEBUG_MODE=ON
