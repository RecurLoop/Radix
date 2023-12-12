cmake_minimum_required(VERSION 3.27.4)

if (NOT CMAKE_PROJECT_NAME OR "${CMAKE_PROJECT_NAME}" STREQUAL "radix")
    project(radix C)
endif()

# Initialization
    if (NOT DEFINED CMAKE_ARCHIVE_OUTPUT_DIRECTORY)
        set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/cache/libraries")
    endif()

    if (NOT DEFINED CMAKE_RUNTIME_OUTPUT_DIRECTORY)
        set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/build")
    endif()

    if (NOT DEFINED RADIX_BUILD_TYPE)
        set(RADIX_BUILD_TYPE "STATIC")
    endif()

    if (NOT DEFINED RADIX_BUILD_EXAMPLE)
        set(RADIX_BUILD_EXAMPLE ON)
    endif()

    if (NOT DEFINED RADIX_REVERT)
        set(RADIX_REVERT 0)
    endif()

# Build
    add_library(radix "${RADIX_BUILD_TYPE}" "${CMAKE_CURRENT_SOURCE_DIR}/source/radix.c")

    target_include_directories(radix PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/include")

    target_compile_definitions(radix PRIVATE RADIX_REVERT=${RADIX_REVERT})

    if (RADIX_BUILD_EXAMPLE)
        add_executable(radix-example "${CMAKE_CURRENT_SOURCE_DIR}/example/main.c")

        target_link_libraries(radix-example radix)

        target_include_directories(radix-example PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/include")

        target_compile_definitions(radix-example PRIVATE RADIX_REVERT=${RADIX_REVERT})
    endif()
