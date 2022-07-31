cmake_minimum_required(VERSION 3.20)
project(fractal VERSION 1.0)

option(USE_LONGDOUBLE "Use long doubles for keeping track of zoom" ON)

configure_file(config.h.in config.h)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")
set(CMAKE_CXX_FLAGS_DEBUG "-g")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")

message("Using flags \"${CMAKE_CXX_FLAGS}\"")

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
        set(file_list "/usr/${TOOLCHAIN_PREFIX}/bin/libwinpthread-1.dll;/usr/${TOOLCHAIN_PREFIX}/bin/SDL2.dll")

        foreach(file IN LISTS file_list)
            message(STATUS ${file})
            cmake_path(GET file FILENAME filename)
            add_custom_command(
                OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/${filename}"
                COMMAND ${CMAKE_COMMAND} -E copy "${file}" "${CMAKE_CURRENT_BINARY_DIR}/${file}"
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

find_package(SDL2 REQUIRED)

include_directories(${SDL2_INCLUDE_DIRS})
target_link_libraries(${PROJECT_NAME} "${SDL2_LIBRARIES}")

include(InstallRequiredSystemLibraries)
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE.md")
set(CPACK_PACKAGE_VERSION_MAJOR "${fractal_VERSION_MAJOR}")
set(CPACK_PACKAGE_VERSION_MINOR "${fractal_VERSION_MINOR}")
set(CPACK_SOURCE_GENERATOR "TGZ")
include(CPack)
