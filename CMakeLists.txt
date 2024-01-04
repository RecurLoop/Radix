cmake_minimum_required(VERSION 3.25.1)

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

    if (NOT DEFINED RADIX_TEST)
        set(RADIX_TEST TRUE)
    endif()

# Build
    add_library(radix "${RADIX_BUILD_TYPE}" "source/radix.c")

    target_include_directories(radix PRIVATE "include")

# Test
    if(RADIX_TEST)
        include(CTest)

        add_executable(radix_test "test/main.c")

        target_link_libraries(radix_test radix)

        target_include_directories(radix_test PRIVATE "include")

        add_test(NAME radix_test COMMAND $<TARGET_FILE:radix_test>)
    endif()
