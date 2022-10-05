cmake_minimum_required(VERSION 3.20)
project(fractal VERSION 1.3)

set(PRECISION_T "long_double" CACHE STRING "MUST BE ONE WORD! What type to use for most major calculations. Try mpfr_float for arbitrary precision, long_double for medium precision.")

configure_file(config.h.in config.h)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -lgmp -lmpfr")
set(CMAKE_CXX_FLAGS_DEBUG "-g")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")

message("Using flags \"${CMAKE_CXX_FLAGS}\"")

if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

if (NOT CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_CURRENT_BINARY_DIR)
    set(file_list "LICENSE.md;README.md")
    
    foreach(file IN LISTS file_list)
        message(STATUS ${file})
        add_custom_command(
            OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/${file}"
            COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_SOURCE_DIR}/${file}" "${CMAKE_CURRENT_BINARY_DIR}/${file}"
        )
    endforeach()
endif()

if (CMAKE_SYSTEM_NAME STREQUAL Windows)
    if (NOT CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_CURRENT_BINARY_DIR)
        set(file_list "libwinpthread-1.dll;SDL2.dll")

        foreach(file IN LISTS file_list)
            message(STATUS ${file})
            if (EXISTS "/usr/${TOOLCHAIN_PREFIX}/bin/${file}")
                add_custom_command(
                    OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/${file}"
                    COMMAND ${CMAKE_COMMAND} -E copy "/usr/${TOOLCHAIN_PREFIX}/bin/${file}" "${CMAKE_CURRENT_BINARY_DIR}/${file}"
                )
            elseif (EXISTS "/usr/${TOOLCHAIN_PREFIX}/lib/${file}")
                add_custom_command(
                    OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/${file}"
                    COMMAND ${CMAKE_COMMAND} -E copy "/usr/${TOOLCHAIN_PREFIX}/lib/${file}" "${CMAKE_CURRENT_BINARY_DIR}/${file}"
                )
            endif()
        endforeach()
    endif()
    add_custom_target(libs ALL DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/libwinpthread-1.dll;${CMAKE_CURRENT_BINARY_DIR}/SDL2.dll")
    install(FILES "libwinpthread-1.dll;SDL2.dll" DESTINATION bin)
endif()

add_custom_target(distclean COMMAND make clean COMMAND ${CMAKE_COMMAND} -E rm -r *.cmake CMakeCache.txt config.h Makefile CMakeFiles _CPack_Packages fractal-*.*-* install_manifest.txt)
add_custom_target(depends ALL DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/README.md;${CMAKE_CURRENT_BINARY_DIR}/LICENSE.md")

add_executable(fractal fractal.cpp)

install(TARGETS fractal DESTINATION bin)

target_include_directories(fractal PUBLIC "${PROJECT_BINARY_DIR}")

find_package(SDL2 REQUIRED)

include_directories(${SDL2_INCLUDE_DIRS})
target_link_libraries(${PROJECT_NAME} ${SDL2_LIBRARIES})

include(InstallRequiredSystemLibraries)
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE.md")
set(CPACK_PACKAGE_VERSION_MAJOR "${fractal_VERSION_MAJOR}")
set(CPACK_PACKAGE_VERSION_MINOR "${fractal_VERSION_MINOR}")
set(CPACK_SOURCE_GENERATOR "TGZ")
include(CPack)
