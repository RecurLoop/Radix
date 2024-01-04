# Radix

This project implements stack-based radix datastore written in C.

Implements all operations known from radix:
| TYPE | OPERATIONS |
| --- | --- |
| modification | insert, remove |
| search | match, ... |
| iterating | prev, next, ... |

It's binary safe, so you can include any string of bits as a key (down to the bit, not the byte).

## Requirements

 - [Cmake](https://cmake.org/download/) (min. version 3.27.4)
 - [GCC](https://www.mingw-w64.org/downloads/) (eg. version 11.3.0)
   [version for Windows](https://winlibs.com/)
   (can be any C compiler)


## Build
The following instructions will generate a static library and an executable file with the included example.

- **Configure**
    ```
    cmake -B./cache/cmake -G Ninja
    ```

- **Compile**
    ```
    cmake --build ./cache/cmake --config Release --target all -j
    ```

## Attach to cmake project
**Download source**
- Download source to your project
- add to your cmake file (please note that you need to adjust some keys)
    ```cmake
    # Configure Radix
    set(RADIX_BUILD_EXAMPLE OFF)

    # Configure subdirectory
    add_subdirectory("path_to_radix_directory")

    # Add include directory and link library with your target
    target_include_directories(your_target_name PRIVATE "path_to_radix_directory/include")
    target_link_libraries(your_target_name radix)
    ```

**Fetch at cmake configure**
- add to your cmake file (please note that you need to adjust some keys)
    ```cmake
    # Enable Fetch Content
    include(FetchContent)

    # Define Radix repository and where to store source code
    FetchContent_Declare(radix
        GIT_REPOSITORY https://github.com/RecurLoop/Radix.git
        GIT_TAG        main
        SOURCE_DIR     "${CMAKE_CURRENT_SOURCE_DIR}/vendor/radix"
    )

    # Configure Radix
    set(RADIX_BUILD_EXAMPLE OFF)

    # Make Available (download repository and configure subdirectory)
    FetchContent_MakeAvailable(radix)

    # Add include directory and link library with your target
    target_include_directories(your_target_name PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/vendor/radix/include")
    target_link_libraries(your_target_name radix)
    ```

## Usage

#### Initialize
Initializes the object and structure memory
```c
#include <radix.h>

size_t radixMemorySize = 1024 * 20; // 20 KiB
uint8_t *radixMemory = malloc(radixMemorySize); // or calloc(radixMemorySize, 1)

Radix radix = radixCreate(radixMemory, radixMemorySize);

// if we zeroed the radix memory it would be empty... but in principle radixClear is definitely faster.
if (radixClear(&radix)) {
    // This means that there is no memory for the radix structure
}

// ...

// Remember to free the allocated memory
free(radixMemory);
```

#### Insert
adds a new value to the structure
```c
// Prepare radix iterator, will be empty - empty iterator starts from the head of the tree
RadixIterator iterator = radixIterator(&radix);

// key size in bits, therefore we multiply by 8
uint8_t *key = (uint8_t *)"KEY";
size_t keyBits = strlen(key) * 8;

// the data size is in bytes, we do not multiply by 8
// but we add 1 to include the terminating character
uint8_t *data = (uint8_t *)"VALUE";
size_t dataSize = strlen(data) + 1;

RadixValue valueIterator = radixInsert(&iterator, key, keyBits, data, dataSize);

if (radixValueIsEmpty(&valueIterator)) {
    // This means that there is no memory for the radix structure
}

// ...
```

#### Remove
actually insert new value = NULL
```c
// Prepare radix iterator, will be empty - empty iterator starts from the head of the tree
RadixIterator iterator = radixIterator(&radix);

// key size in bits, therefore we multiply by 8
uint8_t *key = (uint8_t *)"KEY";
size_t keyBits = strlen(key) * 8;

RadixValue valueIterator = radixRemove(&iterator, key, keyBits);

if (radixValueIsEmpty(&valueIterator)) {
    // This means that there is no memory for the radix structure
}

// ...
```

#### Match Match-First Match-Longest
Finds an element in the structure that matches the given key<br/>
They all treat a NULL value as if it didn't exist, which is why there are nullable equivalents of these functions that also return iterators pointing to a value equal to NULL<br/>
```c
// Prepare radix iterator, will be empty - empty iterator starts from the head of the tree
RadixIterator iterator = radixIterator(&radix);

// key size in bits, therefore we multiply by 8
uint8_t *key = (uint8_t *)"KEY";
size_t keyBits = strlen(key) * 8;

RadixMatch match;

// radixMatch tries to find an exact match
match = radixMatch(&iterator, key, keyBits);

// radixMatchFirst returns the first possible matching word
match = radixMatchFirst(&iterator, key, keyBits);

// radixMatchLongest returns the longest possible matching word
match = radixMatchFirst(&iterator, key, keyBits);

if (radixMatchIsEmpty(&match)) {
    // This means that no words have been matched
}

// This way you can check how many bits the matched word contains
// note that with radixMatch this number will be equal to the keyBits passed to the function - 24 In this case
size_t matchedBits = match.matchedBits;

// ...
```

#### Predecessor Prev Next Prev-Inverse Next-Inverse
Iterate through a structure in lexicographic order <br/>
They all treat a NULL value as if it didn't exist, which is why there are nullable equivalents of these functions that also return iterators pointing to a value equal to NULL
```c
// Prepare radix iterator, will be empty
// empty iterator empty iterator starts from the first element (depending on the function being called)
RadixIterator iterator = radixIterator(&radix);

// key size in bits, therefore we multiply by 8
uint8_t *key = (uint8_t *)"KEY";
size_t keyBits = strlen(key) * 8;

RadixMatch match;

// radixMatch tries to find an exact match
match = radixMatch(&iterator, key, keyBits);

// convert match to iterator
iterator = radixMatchToIterator(&match);

// find predecessor of given key
RadixIterator predecessor = radixPredecessor(&iterator);

if (radixIteratorIsEmpty(&predecessor)) {
    // This means that the given word has no predecessor
}

// find smaller word in lexicographical order
iterator = radixPrev(&iterator);

// find greater word in lexicographical order
iterator = radixNext(&iterator);

// find smaller word in lexicographical order
// this function is different in that it has a strange assumption that shorter keys are larger than longer ones
iterator = radixPrevInverse(&iterator);

// find greater word in lexicographical order
// this function is different in that it has a strange assumption that shorter keys are larger than longer ones
iterator = radixNextInverse(&iterator);

//...
if (radixIteratorIsEmpty(&iterator)) {
    // This means there is no bigger/smaller word in the structure
}
// ...
```

#### Earlier
Iterate through a structure in chronological order <br/>
There is no later function because it would require additional memory
They all treat a NULL value as if it didn't exist, which is why there are nullable equivalents of these functions that also return iterators pointing to a value equal to NULL<br/>
```c
// Prepare radix iterator, will be empty
// empty iterator empty iterator starts from the last element
RadixIterator iterator = radixIterator(&radix);

iterator = radixEarlier(&iterator);
```


... <br/>
#### Features also include:
- iterating through values ​​(within the same key and within the entire structure)
- key length checking and key copying function
- functions that convert one object into another and check whether they point to a specific element in the structure
- saving and restoring checkpoints (based on stack-based structure construction)
- checking the structure's memory usage
<br/>

#### For more information, I recommend that you familiarize yourself with the function definitions and their calls in the
- include\radix.h file
- example\main.c file
