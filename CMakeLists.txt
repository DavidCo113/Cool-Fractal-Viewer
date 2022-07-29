cmake_minimum_required(VERSION 3.16)
project(fractal VERSION 1.0)

option(USE_LONGDOUBLE "Use long doubles for keeping track of zoom" ON)

configure_file(config.h.in config.h)

#set(CMAKE_MODULE_PATH "/usr/x86_64-w64-mingw32/lib;/usr/x86_64-w64-mingw32;/usr/x86_64-w64-mingw32/bin;/usr/x86_64-w64-mingw32/include")
#set(FIND_SDL2_PATHS "/usr/x86_64-w64-mingw32/bin")
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -static-libgcc -static-libstdc++")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")
set(CMAKE_CXX_FLAGS_DEBUG "-g")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")

message("Using flags \"${CMAKE_CXX_FLAGS}\"")

#set(CMAKE_EXE_LINKER_FLAGS "-static")

#add_compile_options("-static")
#link_libraries("-static")

#add_library(myLib SHARED mandelbrot.cpp)
#set(CMAKE_EXE_LINKER_FLAGS " -static")
#target_link_libraries(myLib -static-libgcc -static-libstdc++)

#add_library(gcc SHARED fractal.cpp)
#set(CMAKE_EXE_LINKER_FLAGS " -static")
#target_link_libraries(gcc -static-libgcc -static-libstdc++)
#target_link_options(gcc static-libgcc static-libstdc++)
#target_link_libraries(gcc fractal)

#Target OS
#set(CMAKE_SYSTEM_NAME Windows)
#set(CMAKE_SYSTEM_PROCESSOR AMD64)
#set(prefix "/usr/x86_64-w64-mingw32")
#set(exec_prefix "${prefix}")

#set(CMAKE_FIND_ROOT_PATH /usr/x86_64-w64-mingw32 /usr/x86_64-w64-mingw32/lib /usr/x86_64-w64-mingw32/bin /usr/x86_64-w64-mingw32/include /usr/i686-w64-mingw32)

#Compilers
#set(CMAKE_C_COMPILER "/usr/bin/86_64-w64-mingw32-gcc")
#set(CMAKE_CXX_COMPILER "/usr/bin/x86_64-w64-mingw32-g++")

#set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
#set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
#set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
#set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

# search headers and libraries in the target environment
#set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
#set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

if (NOT CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_CURRENT_BINARY_DIR)
    message(STATUS icon.bmp)
    add_custom_command(
        OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/icon.bmp"
        COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_SOURCE_DIR}/icon.bmp" "${CMAKE_CURRENT_BINARY_DIR}/icon.bmp"
    )
endif()

if (CMAKE_SYSTEM_NAME STREQUAL Windows)
    if (NOT CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_CURRENT_BINARY_DIR)
        set(file_list "libwinpthread-1.dll;SDL2.dll")

        foreach(file IN LISTS file_list)
            message(STATUS ${file})
            add_custom_command(
                OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/${file}"
                COMMAND ${CMAKE_COMMAND} -E copy "/usr/${TOOLCHAIN_PREFIX}/bin/${file}" "${CMAKE_CURRENT_BINARY_DIR}/${file}"
            )
        endforeach()
    endif()
    add_custom_target(libs ALL DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/libwinpthread-1.dll;${CMAKE_CURRENT_BINARY_DIR}/SDL2.dll")
    install(FILES "libwinpthread-1.dll;SDL2.dll" DESTINATION bin)
endif()

add_custom_target(distclean COMMAND make clean COMMAND ${CMAKE_COMMAND} -E rm -r *.cmake CMakeCache.txt config.h Makefile CMakeFiles _CPack_Packages fractal-*.*-* install_manifest.txt)
add_custom_target(icon ALL DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/icon.bmp")

add_executable(fractal fractal.cpp)

install(TARGETS fractal DESTINATION bin)
install(FILES "${PROJECT_BINARY_DIR}/icon.bmp" DESTINATION share)

target_include_directories(fractal PUBLIC "${PROJECT_BINARY_DIR}")

#find_package(PkgConfig REQUIRED)
#pkg_search_module(SDL2 REQUIRED sdl2)
#include_directories(${SDL2_INCLUDE_DIRS})
#target_link_libraries(${PROJECT_NAME} ${SDL2_LIBRARIES})

find_package(SDL2 REQUIRED)

include_directories(${SDL2_INCLUDE_DIRS})
target_link_libraries(${PROJECT_NAME} "${SDL2_LIBRARIES}")
#include_directories("/usr/x86_64-w64-mingw32/bin")
#target_link_libraries(${PROJECT_NAME} "/usr/x86_64-w64-mingw32/lib/libSDL2main.a;/usr/x86_64-w64-mingw32/bin/SDL2.dll")

#message("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa   ${SDL2_LIBRARIES}")

include(InstallRequiredSystemLibraries)
#set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/License.txt")
set(CPACK_PACKAGE_VERSION_MAJOR "${fractal_VERSION_MAJOR}")
set(CPACK_PACKAGE_VERSION_MINOR "${fractal_VERSION_MINOR}")
set(CPACK_SOURCE_GENERATOR "TGZ")
include(CPack)
