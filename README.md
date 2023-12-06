# Radix
This project implements stack-based radix datastore written in C.

Implements all operations known from radix:
| TYPE | OPERATIONS |
| --- | --- |
| modification | insert, remove |
| match | match, matchFirst, matchLongest |
| iterating | prev, next, prevInverse*, nextInverse*, Earlier |

*"Inverse" in this context means the strange assumption that shorter keys are larger than longer ones

It's binary safe, so you can include any string of bits as a key (down to the bit, not the byte).

The structure was implemented like a stack, so that data was added to the stack and data could then be pulled from the stack using operations 'checkpoint', 'checkpointRestore'

The stack-based structure allows for two different implementations - adding at the beginning (normal) or adding at the end (revert) of the memory area. You can decide what implementation to use using the cmake flag ```set(RADIX_REVERT 0)``` (or 1). <br>
*You must remember to pass to the structure point to the end of memory, and not to the beginning as in a normal implementation.

There is no complete documentation at the moment, but the code contained in header and example should be sufficient

## Requirements

 - [Cmake](https://cmake.org/download/) (min. version 3.27.4)
 - [GCC](https://www.mingw-w64.org/downloads/) (eg. version 11.3.0)
   [version for Windows](https://winlibs.com/)
   (can be any C compiler)


## Build
The following instructions will generate a shared library and an executable file with the included example.

- **Configure**
    ```
    cmake -B./cache/cmake -G Ninja
    ```

- **Compile**
    ```
    cmake --build ./cache/cmake --config Release --target all -j
    ```

## Add source to cmake project
- Download source to your project
- add to your cmake file (please note that you need to adjust some keys)
    ```
    set(RADIX_BUILD_EXAMPLE OFF)
    set(RADIX_BUILD_TYPE "STATIC")
    add_subdirectory("path-to-radix-directory")
    target_include_directories(your-target-name PRIVATE "path-to-radix-directory/include")
    target_link_libraries(your-target-name radix)
    ```

