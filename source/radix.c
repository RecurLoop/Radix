#include <radix.h>

typedef struct Meta {
    // Stores the last radix node(chronologically)
    size_t lastNode;

    // Stores the last radix item(chronologically)
    size_t lastItem;

    // Information needed for memory management
    size_t structureEnd;
} Meta;

typedef struct Node {
    // Stores parent (lexicographically)
    size_t parent;

    // Stores children (lexicographically)
    size_t childSmaller;
    size_t childGreater;

    // Stores the key address
    size_t keyFore;
    size_t keyRear;

    struct {
        // Stores the bit offset (cooperates with keyFore and keyRear)
        uint8_t keyForeOffset : 3;
        uint8_t keyRearOffset : 3;
    };

    // Stores item
    size_t item;

    // Stores the last radix node before being added
    size_t lastNode;
} Node;

typedef struct Item {
    // Stores the data
    size_t size;

    // Stores the owner node
    size_t node;

    // Stores the last node item before being added (chronologically)
    size_t previous;

    // Stores the last radix item before being added(chronologically)
    size_t lastItem;
} Item;

static inline bool bitGet(uint8_t *stream, size_t bitIndex)
{
    if (stream == NULL)
        return false;

    uint8_t mask = 1 << (8 - 1 - (bitIndex % 8));
    stream += bitIndex / 8;

    return *stream & mask;
}

static inline void bitSet(uint8_t *stream, size_t bitIndex, bool value)
{
    if (stream == NULL)
        return;

    uint8_t mask = 1 << (8 - 1 - (bitIndex % 8));
    stream += bitIndex / 8;

    if (value)
        *stream |= mask;
    else
        *stream &= ~mask;
}

static inline void bitCopy(uint8_t *input, size_t inputOffset, uint8_t *output, size_t outputOffset, size_t count)
{
    for (size_t i = 0; i < count; i++) {
        bitSet(output, outputOffset + i, bitGet(input, inputOffset + i));
    }
}

static inline size_t bitCompare(uint8_t *a, size_t aFore, size_t aRear, uint8_t *b, size_t bFore, size_t bRear)
{
    size_t aSize = aRear - aFore;
    size_t bSize = bRear - bFore;

    size_t minSize = aSize < bSize ? aSize : bSize;

    for (size_t i = 0; i < minSize; i++) {
        if (bitGet(a, aFore + i) != bitGet(b, bFore + i)) {
            return i;
        }
    }

    return minSize;
}

Radix radixCreate(uint8_t *memory, size_t memorySize)
{
    return (Radix){
        .memory = memory,
        .memorySize = memorySize,
    };
}

RadixIterator radixIterator(Radix *radix)
{
    return (RadixIterator){
        .radix = radix,
        .node = 0,
        .data = NULL,
        .dataSize = 0,
    };
}

RadixValue radixValueIterator(Radix *radix)
{
    return (RadixValue){
        .radix = radix,
        .item = 0,
        .data = NULL,
        .dataSize = 0,
    };
}

RadixValue radixInsert(RadixIterator* iterator, uint8_t *key, size_t keyBits, uint8_t *data, size_t dataSize)
{
    Radix *radix = iterator->radix;

    Meta *meta = (Meta *)radix->memory;

    RadixValue result = {0};

    result.radix = radix;

    Node *node = iterator->node != 0 ? (Node *) (radix->memory + iterator->node) : NULL;

    // If node is null, this means we should start with the head-node
    if (!node) {
        // If the structure has not been managed before, it may need to be initialized
        if (meta->lastNode == 0) {
            // Calculate needed memory
            size_t neededMemory = sizeof(Meta) + sizeof(Node);

            // Check free memory
            if (neededMemory > radix->memorySize - meta->structureEnd)
                return result;

            // Compose memory
            node = (Node *) (radix->memory + sizeof(Meta));

            // Write head node
            *node = (Node) {0};

            // Write meta information
            *meta = (Meta) {
                .lastNode = (uint8_t *)node - radix->memory,
                .lastItem = 0,
                .structureEnd = neededMemory,
            };
        }

        node = (Node *) (radix->memory + sizeof(Meta));
    }

    // Insert node - iterate thought structure and create new edge-nodes
    for (size_t keyPos = 0; keyPos < keyBits;) {
        // Get direction of iteration
        bool direction = bitGet(key, keyPos);

        // Iterate..
        size_t childAddress = direction ? node->childGreater : node->childSmaller;

        // If there is no child ..create it and end iteration
        if (childAddress == 0) {
            // Calculate needed memory
            size_t neededMemory = sizeof(Node) + ((keyBits - keyPos + 8 - 1) / 8);

            // Check free memory
            if (neededMemory > radix->memorySize - meta->structureEnd) {
                return result;
            }

            // Compose memory
            Node *newNode = (Node*) (radix->memory + meta->structureEnd);
            uint8_t *newKey = (uint8_t *) newNode + sizeof(Node);

            // Write node
            *newNode = (Node) {
                .parent = (uint8_t *)node - radix->memory,
                .childSmaller = 0,
                .childGreater = 0,
                .keyFore = newKey - radix->memory,
                .keyRear = newKey - radix->memory + ((keyBits - keyPos) / 8),
                .keyForeOffset = 0,
                .keyRearOffset = ((keyBits - keyPos) % 8),
                .lastNode = meta->lastNode,
                .item = 0,
            };

            // Write key
            bitCopy(key, keyPos, newKey, 0, keyBits - keyPos);

            // Set new node as node child
            size_t *nodeChild = direction ? &(node->childGreater) : &(node->childSmaller);

            *nodeChild = (uint8_t *)newNode - radix->memory;

            // Update meta information
            meta->lastNode = (uint8_t *)newNode - radix->memory;
            meta->structureEnd += neededMemory;

            // Assign new node as current node
            node = newNode;

            break;
        }

        // Check if the key is correct
        Node *testNode = (Node *) (radix->memory + childAddress);

        uint8_t *testKey = (uint8_t *) (radix->memory + testNode->keyFore);

        size_t testKeyFore = testNode->keyForeOffset;
        size_t testKeyRear = 8 * (testNode->keyRear - testNode->keyFore) + testNode->keyRearOffset;

        size_t testKeyBits = testKeyRear - testKeyFore;

        // Compare key with testKey
        size_t matchedBits = bitCompare(key, keyPos, keyBits, testKey, testKeyFore, testKeyRear);

        // If key is not fully maching ..split this node and continue iteration
        if (matchedBits < testKeyBits) {
            // Calculate needed memory
            size_t neededMemory = sizeof(Node);

            // Check free memory
            if (neededMemory > radix->memorySize - meta->structureEnd) {
                return result;
            }

            // Compose memory
            Node *newNode = (Node *) (radix->memory + meta->structureEnd);

            // Get split direction
            bool splitDirection = bitGet(
                (uint8_t *) (radix->memory + testNode->keyFore),
                testNode->keyForeOffset + matchedBits
            );

            // Write split node
            *newNode = (Node) {
                .parent = testNode->parent,
                .childSmaller = splitDirection ? 0 : (uint8_t *)testNode - radix->memory,
                .childGreater = splitDirection ? (uint8_t *)testNode - radix->memory : 0,
                .keyFore = testNode->keyFore,
                .keyRear = testNode->keyFore + ((testNode->keyForeOffset + matchedBits) / 8),
                .keyForeOffset = testNode->keyForeOffset,
                .keyRearOffset = (testNode->keyForeOffset + matchedBits) % 8,
                .lastNode = meta->lastNode,
                .item = 0,
            };

            // Update splited node
            testNode->parent = (uint8_t *)newNode - radix->memory;
            testNode->keyFore = newNode->keyRear;
            testNode->keyForeOffset = newNode->keyRearOffset;

            // Set new node as node child
            size_t *nodeChild = direction ? &(node->childGreater) : &(node->childSmaller);

            *nodeChild = (uint8_t *)newNode - radix->memory;

            // Update meta information
            meta->lastNode = (uint8_t *)newNode - radix->memory;
            meta->structureEnd += neededMemory;

            // Assign new node as fully maching node
            testNode = newNode;
        }

        // key is fully correct, set testNode as node
        node = testNode;
        keyPos += matchedBits;
    }

    // Insert item
    // Node is matched, so add a new item to the structure
    {
        // Calculate needed memory
        size_t neededMemory = sizeof(Item) + dataSize;

        // Check free memory
        if (neededMemory > radix->memorySize - meta->structureEnd) {
            return result;
        }

        // Compose memory
        Item *newItem = (Item *) (radix->memory + meta->structureEnd);
        uint8_t *newData = (uint8_t *)newItem + sizeof(Item);

        // Write item
        *newItem = (Item) {
            .size = dataSize,
            .node = (uint8_t *)node - radix->memory,
            .previous = node->item,
            .lastItem = meta->lastItem,
        };

        // Write data
        for (size_t i = 0; i < dataSize; i++) {
            newData[i] = data[i];
        }

        // Update node
        node->item = (uint8_t *)newItem - radix->memory;

        // Update meta information
        meta->lastItem = (uint8_t *)newItem - radix->memory;
        meta->structureEnd += neededMemory;

        // Update result
        result.item = (uint8_t *)newItem - radix->memory;
        result.data = newData;
        result.dataSize = dataSize;
    }

    return result;
}

RadixValue radixRemove(RadixIterator* iterator, uint8_t *key, size_t keyBits)
{
    return radixInsert(iterator, key, keyBits, NULL, 0);
}

RadixMatch radixMatch(RadixIterator* iterator, uint8_t *key, size_t keyBits)
{
    Radix *radix = iterator->radix;

    Meta *meta = (Meta *)radix->memory;

    RadixMatch result = {0};

    result.radix = radix;

    Node *node = iterator->node != 0 ? (Node *) (radix->memory + iterator->node) : NULL;

    // If node is null, this means we should start with the head-node
    if (!node) {
        // If the structure has not been managed before, you can't start from head
        if (meta->lastNode == 0)
            return result;

        node = (Node *) (radix->memory + sizeof(Meta));
    }

    for (size_t keyPos = 0; true;) {
        // If keyPos has reached the size ..end iteration
        if (keyPos >= keyBits) {
            Item *item = node->item != 0 ? (Item *) (radix->memory + node->item) : NULL;

            // If matched node has item (not nullable) - update match
            if (item && item->size > 0) {
                result.node = (uint8_t *)node - radix->memory;
                result.matchedBits = keyPos;
                result.data = (uint8_t*)item + sizeof(Item);
                result.dataSize = item->size;
            }

            break;
        }

        // Get direction of iteration
        bool direction = bitGet(key, keyPos);

        // Iterate..
        size_t childAddress = direction ? node->childGreater : node->childSmaller;

        // If there is no child ..break
        if (childAddress == 0)
            break;

        // Check if the key is correct
        Node *testNode = (Node *) (radix->memory + childAddress);

        uint8_t *testKey = (uint8_t *) (radix->memory + testNode->keyFore);

        size_t testKeyFore = testNode->keyForeOffset;
        size_t testKeyRear = 8 * (testNode->keyRear - testNode->keyFore) + testNode->keyRearOffset;

        size_t testKeySize = testKeyRear - testKeyFore;

        // Compare key with testKey
        size_t matchedBits = bitCompare(key, keyPos, keyBits, testKey, testKeyFore, testKeyRear);

        // if key is not fully correct ..break
        if (matchedBits < testKeySize)
            break;

        // Set child as current node and move key position
        node = testNode;
        keyPos += matchedBits;
    }

    return result;
}

RadixMatch radixMatchNullable(RadixIterator* iterator, uint8_t *key, size_t keyBits)
{
    Radix *radix = iterator->radix;

    Meta *meta = (Meta *)radix->memory;

    RadixMatch result = {0};

    result.radix = radix;

    Node *node = iterator->node != 0 ? (Node *) (radix->memory + iterator->node) : NULL;

    // If node is null, this means we should start with the head-node
    if (!node) {
        // If the structure has not been managed before, you can't start from head
        if (meta->lastNode == 0)
            return result;

        node = (Node *) (radix->memory + sizeof(Meta));
    }

    for (size_t keyPos = 0; true;) {
        // If keyPos has reached the size ..end iteration
        if (keyPos >= keyBits) {
            Item *item = node->item != 0 ? (Item *) (radix->memory + node->item) : NULL;

            // If matched node has item (nullable) - update match
            if (item) {
                result.node = (uint8_t *)node - radix->memory;
                result.matchedBits = keyPos;
                result.data = (uint8_t*)item + sizeof(Item);
                result.dataSize = item->size;
            }

            break;
        }

        // Get direction of iteration
        bool direction = bitGet(key, keyPos);

        // Iterate..
        size_t childAddress = direction ? node->childGreater : node->childSmaller;

        // If there is no child ..break
        if (childAddress == 0)
            break;

        // Check if the key is correct
        Node *testNode = (Node *) (radix->memory + childAddress);

        uint8_t *testKey = (uint8_t *) (radix->memory + testNode->keyFore);

        size_t testKeyFore = testNode->keyForeOffset;
        size_t testKeyRear = 8 * (testNode->keyRear - testNode->keyFore) + testNode->keyRearOffset;

        size_t testKeySize = testKeyRear - testKeyFore;

        // Compare key with testKey
        size_t matchedBits = bitCompare(key, keyPos, keyBits, testKey, testKeyFore, testKeyRear);

        // If key is not fully correct ..break
        if (matchedBits < testKeySize)
            break;

        // Set child as current node and move key position
        node = testNode;
        keyPos += matchedBits;
    }

    return result;
}

RadixMatch radixMatchFirst(RadixIterator* iterator, uint8_t *key, size_t keyBits)
{
    Radix *radix = iterator->radix;

    Meta *meta = (Meta *)radix->memory;

    RadixMatch result = {0};

    result.radix = radix;

    Node *node = iterator->node != 0 ? (Node *) (radix->memory + iterator->node) : NULL;

    // If node is null, this means we should start with the head-node
    if (!node) {
        // If the structure has not been managed before, you can't start from head
        if (meta->lastNode == 0)
            return result;

        node = (Node *) (radix->memory + sizeof(Meta));
    }

    for (size_t keyPos = 0; true;) {
        Item *item = node->item != 0 ? (Item *) (radix->memory + node->item) : NULL;

        // If matched node has item (not nullable) - update match and break
        if (item && item->size > 0) {
            result.node = (uint8_t *)node - radix->memory;
            result.matchedBits = keyPos;
            result.data = (uint8_t*)item + sizeof(Item);
            result.dataSize = item->size;

            break;
        }

        // If keyPos has reached the size of the given key - break
        if (keyBits <= keyPos)
            break;

        // Get direction of iteration
        bool direction = bitGet(key, keyPos);

        // Iterate..
        size_t childAddress = direction ? node->childGreater : node->childSmaller;

        // If there is no child ..break
        if (childAddress == 0)
            break;

        // Check if the key is correct
        Node *testNode = (Node *) (radix->memory + childAddress);

        uint8_t *testKey = (uint8_t *) (radix->memory + testNode->keyFore);

        size_t testKeyFore = testNode->keyForeOffset;
        size_t testKeyRear = 8 * (testNode->keyRear - testNode->keyFore) + testNode->keyRearOffset;

        size_t testKeySize = testKeyRear - testKeyFore;

        // Compare key with testKey
        size_t matchedBits = bitCompare(key, keyPos, keyBits, testKey, testKeyFore, testKeyRear);

        // If key is not fully correct ..break
        if (matchedBits < testKeySize)
            break;

        // Set child as current node and move key position
        node = testNode;
        keyPos += matchedBits;
    }

    return result;
}

RadixMatch radixMatchFirstNullable(RadixIterator* iterator, uint8_t *key, size_t keyBits)
{
    Radix *radix = iterator->radix;

    Meta *meta = (Meta *)radix->memory;

    RadixMatch result = {0};

    result.radix = radix;

    Node *node = iterator->node != 0 ? (Node *) (radix->memory + iterator->node) : NULL;

    // If node is null, this means we should start with the head-node
    if (!node) {
        // If the structure has not been managed before, you can't start from head
        if (meta->lastNode == 0)
            return result;

        node = (Node *) (radix->memory + sizeof(Meta));
    }

    for (size_t keyPos = 0; true;) {
        Item *item = node->item != 0 ? (Item *) (radix->memory + node->item) : NULL;

        // If matched node has item (nullable) - update match and break
        if (item) {
            result.node = (uint8_t *)node - radix->memory;
            result.matchedBits = keyPos;
            result.data = (uint8_t*)item + sizeof(Item);
            result.dataSize = item->size;

            break;
        }

        // If keyPos has reached the size of the given key - break
        if (keyBits <= keyPos)
            break;

        // Get direction of iteration
        bool direction = bitGet(key, keyPos);

        // Iterate..
        size_t childAddress = direction ? node->childGreater : node->childSmaller;

        // If there is no child ..break
        if (childAddress == 0)
            break;

        // Check if the key is correct
        Node *testNode = (Node *) (radix->memory + childAddress);

        uint8_t *testKey = (uint8_t *) (radix->memory + testNode->keyFore);

        size_t testKeyFore = testNode->keyForeOffset;
        size_t testKeyRear = 8 * (testNode->keyRear - testNode->keyFore) + testNode->keyRearOffset;

        size_t testKeySize = testKeyRear - testKeyFore;

        // Compare key with testKey
        size_t matchedBits = bitCompare(key, keyPos, keyBits, testKey, testKeyFore, testKeyRear);

        // If key is not fully correct ..break
        if (matchedBits < testKeySize)
            break;

        // Set child as current node and move key position
        node = testNode;
        keyPos += matchedBits;
    }

    return result;
}

RadixMatch radixMatchLongest(RadixIterator* iterator, uint8_t *key, size_t keyBits)
{
    Radix *radix = iterator->radix;

    Meta *meta = (Meta *)radix->memory;

    RadixMatch result = {0};

    result.radix = radix;

    Node *node = iterator->node != 0 ? (Node *) (radix->memory + iterator->node) : NULL;

    // If node is null, this means we should start with the head-node
    if (!node) {
        // If the structure has not been managed before, you can't start from head
        if (meta->lastNode == 0) return result;

        node = (Node *) (radix->memory + sizeof(Meta));
    }

    for (size_t keyPos = 0; true;) {
        Item *item = node->item != 0 ? (Item *) (radix->memory + node->item) : NULL;

        // If matched node has item (not nullable) - update match and continue
        if (item && item->size > 0) {
            result.node = (uint8_t *)node - radix->memory;
            result.matchedBits = keyPos;
            result.data = (uint8_t*)item + sizeof(Item);
            result.dataSize = item->size;
        }

        // If keyPos has reached the size of the given key - break
        if (keyBits <= keyPos)
            break;

        // Get direction of iteration
        bool direction = bitGet(key, keyPos);

        // Iterate..
        size_t childAddress = direction ? node->childGreater : node->childSmaller;

        // If there is no child ..break
        if (childAddress == 0)
            break;

        // Check if the key is correct
        Node *testNode = (Node *) (radix->memory + childAddress);

        uint8_t *testKey = (uint8_t *) (radix->memory + testNode->keyFore);

        size_t testKeyFore = testNode->keyForeOffset;
        size_t testKeyRear = 8 * (testNode->keyRear - testNode->keyFore) + testNode->keyRearOffset;

        size_t testKeySize = testKeyRear - testKeyFore;

        // Compare key with testKey
        size_t matchedBits = bitCompare(key, keyPos, keyBits, testKey, testKeyFore, testKeyRear);

        // If key is not fully correct ..break
        if (matchedBits < testKeySize)
            break;

        // Set child as current node and move key position
        node = testNode;
        keyPos += matchedBits;
    }

    return result;
}

RadixMatch radixMatchLongestNullable(RadixIterator* iterator, uint8_t *key, size_t keyBits)
{
    Radix *radix = iterator->radix;

    Meta *meta = (Meta *)radix->memory;

    RadixMatch result = {0};

    result.radix = radix;

    Node *node = iterator->node != 0 ? (Node *) (radix->memory + iterator->node) : NULL;

    // If node is null, this means we should start with the head-node
    if (!node) {
        // If the structure has not been managed before, you can't start from head
        if (meta->lastNode == 0)
            return result;

        node = (Node *) (radix->memory + sizeof(Meta));
    }

    for (size_t keyPos = 0; true;) {
        Item *item = node->item != 0 ? (Item *) (radix->memory + node->item) : NULL;

        // If matched node has item (nullable) - update match and continue
        if (item) {
            result.node = (uint8_t *)node - radix->memory;
            result.matchedBits = keyPos;
            result.data = (uint8_t*)item + sizeof(Item);
            result.dataSize = item->size;
        }

        // If keyPos has reached the size of the given key - break
        if (keyBits <= keyPos)
            break;

        // Get direction of iteration
        bool direction = bitGet(key, keyPos);

        // Iterate..
        size_t childAddress = direction ? node->childGreater : node->childSmaller;

        // If there is no child ..break
        if (childAddress == 0)
            break;

        // Check if the key is correct
        Node *testNode = (Node *) (radix->memory + childAddress);

        uint8_t *testKey = (uint8_t *) (radix->memory + testNode->keyFore);

        size_t testKeyFore = testNode->keyForeOffset;
        size_t testKeyRear = 8 * (testNode->keyRear - testNode->keyFore) + testNode->keyRearOffset;

        size_t testKeySize = testKeyRear - testKeyFore;

        // Compare key with testKey
        size_t matchedBits = bitCompare(key, keyPos, keyBits, testKey, testKeyFore, testKeyRear);

        // If key is not fully correct ..break
        if (matchedBits < testKeySize)
            break;

        // Set child as current node and move key position
        node = testNode;
        keyPos += matchedBits;
    }

    return result;
}

RadixIterator radixMatchToIterator(RadixMatch *match)
{
    return (RadixIterator) {
        .radix = match->radix,
        .node = match->node,
        .data = match->data,
        .dataSize = match->dataSize,
    };
}

bool radixMatchIsEmpty(RadixMatch *match)
{
    return match->node == 0;
}

RadixIterator radixPredecessor(RadixIterator *iterator)
{
    Radix *radix = iterator->radix;

    RadixIterator result = {0};

    result.radix = radix;

    Node *node = iterator->node != 0 ? (Node *) (radix->memory + iterator->node) : NULL;

    // If node is null, return null
    if (!node)
        return result;

    while (true) {
        node = node->parent != 0 ? (Node *)(radix->memory + node->parent) : NULL;

        if (!node)
            return result;

        Item *item = node->item != 0 ? (Item *)(radix->memory + node->item) : NULL;

        // If matched node has item (not nullable) - this is the object you are looking for
        if (item && item->size > 0) {
            result.node = (uint8_t *)node - radix->memory;
            result.data = (uint8_t*)item + sizeof(Item);
            result.dataSize = item->size;

            return result;
        }
    }
}

RadixIterator radixPredecessorNullable(RadixIterator *iterator)
{
    Radix *radix = iterator->radix;

    RadixIterator result = {0};

    result.radix = radix;

    Node *node = iterator->node != 0 ? (Node *) (radix->memory + iterator->node) : NULL;

    // If node is null, return null
    if (!node)
        return result;

    // Move to predecessor
    while (true) {
        node = node->parent != 0 ? (Node *)(radix->memory + node->parent) : NULL;

        if (!node)
            return result;

        Item *item = node->item != 0 ? (Item *)(radix->memory + node->item) : NULL;

        // If matched node has item (nullable) - this is the object you are looking for
        if (item) {
            result.node = (uint8_t *)node - radix->memory;
            result.data = (uint8_t*)item + sizeof(Item);
            result.dataSize = item->size;

            return result;
        }
    }
}

RadixIterator radixPrev(RadixIterator *iterator)
{
    Radix *radix = iterator->radix;

    Meta *meta = (Meta *)radix->memory;

    RadixIterator result = {0};

    result.radix = radix;

    Node *node = iterator->node != 0 ? (Node *) (radix->memory + iterator->node) : NULL;

    // If node is null, this means we should start with the greatest leaf of all nodes
    if (!node) {
        // If the structure has not been managed before, you can't start from head
        if (meta->lastNode == 0)
            return result;

        // Retrieve head-node
        node = (Node *) (radix->memory + sizeof(Meta));

        // Move to the greatest leaf-node of given node
        while (node->childSmaller != 0 || node->childGreater != 0) {
            node = (Node *) (radix->memory + (node->childGreater != 0 ? node->childGreater : node->childSmaller));
        }

        Item *item = node->item != 0 ? (Item *) (radix->memory + node->item) : NULL;

        // If matched node has item (not nullable) - this is the object you are looking for
        if (item && item->size > 0) {
            result.node = (uint8_t *)node - radix->memory;
            result.data = (uint8_t*)item + sizeof(Item);
            result.dataSize = item->size;

            return result;
        }
    }

    // Move to smaller (shorter < longer)
    // Priority:
    // 1: parent-child-child (leaf child)
    // 2: parent-child
    // 3: parent
    //   no parent - return empty iterator
    while (true) {
        Node *parentNode = node->parent != 0 ? (Node *) (radix->memory + node->parent) : NULL;

        // "no parent"
        if (!parentNode)
            return result;

        if (parentNode->childSmaller != 0 && parentNode->childSmaller != (uint8_t*)node - radix->memory) {
            node = (Node *) (radix->memory + parentNode->childSmaller);

            // Move to "parent-child-child" or "parent-child"
            while (node->childSmaller != 0 || node->childGreater != 0) {
                node = (Node *) (radix->memory + (node->childGreater != 0 ? node->childGreater : node->childSmaller));
            }
        } else {
            // Move to "parent"
            node = parentNode;
        }

        Item *item = node->item != 0 ? (Item *) (radix->memory + node->item) : NULL;

        // If matched node has item not (nullable) - this is the object you are looking for
        if (item && item->size > 0) {
            result.node = (uint8_t *)node - radix->memory;
            result.data = (uint8_t*)item + sizeof(Item);
            result.dataSize = item->size;

            return result;
        }
    }
}

RadixIterator radixPrevNullable(RadixIterator *iterator)
{
    Radix *radix = iterator->radix;

    Meta *meta = (Meta *)radix->memory;

    RadixIterator result = {0};

    result.radix = radix;

    Node *node = iterator->node != 0 ? (Node *) (radix->memory + iterator->node) : NULL;

    // If node is null, this means we should start with the greatest leaf of all nodes
    if (!node) {
        // If the structure has not been managed before, you can't start from head
        if (meta->lastNode == 0)
            return result;

        // Retrieve head-node
        node = (Node *) (radix->memory + sizeof(Meta));

        // Move to the greatest leaf-node of given node
        while (node->childSmaller != 0 || node->childGreater != 0) {
            node = (Node *) (radix->memory + (node->childGreater != 0 ? node->childGreater : node->childSmaller));
        }

        Item *item = node->item != 0 ? (Item *) (radix->memory + node->item) : NULL;

        // If matched node has item (nullable) - this is the object you are looking for
        if (item) {
            result.node = (uint8_t *)node - radix->memory;
            result.data = (uint8_t*)item + sizeof(Item);
            result.dataSize = item->size;

            return result;
        }
    }

    // Move to smaller (shorter < longer)
    // Priority:
    // 1: parent-child-child (leaf child)
    // 2: parent-child
    // 3: parent
    //   no parent - return empty iterator
    while (true) {
        Node *parentNode = node->parent != 0 ? (Node *) (radix->memory + node->parent) : NULL;

        // "no parent"
        if (!parentNode)
            return result;

        if (parentNode->childSmaller != 0 && parentNode->childSmaller != (uint8_t*)node - radix->memory) {
            node = (Node *) (radix->memory + parentNode->childSmaller);

            // Move to "parent-child-child" or "parent-child"
            while (node->childSmaller != 0 || node->childGreater != 0) {
                node = (Node *) (radix->memory + (node->childGreater != 0 ? node->childGreater : node->childSmaller));
            }
        } else {
            // Move to "parent"
            node = parentNode;
        }

        Item *item = node->item != 0 ? (Item *) (radix->memory + node->item) : NULL;

        // If matched node has item (nullable) - this is the object you are looking for
        if (item) {
            result.node = (uint8_t *)node - radix->memory;
            result.data = (uint8_t*)item + sizeof(Item);
            result.dataSize = item->size;

            return result;
        }
    }
}

RadixIterator radixNext(RadixIterator *iterator)
{
    Radix *radix = iterator->radix;

    Meta *meta = (Meta *)radix->memory;

    RadixIterator result = {0};

    result.radix = radix;

    Node *node = iterator->node != 0 ? (Node *) (radix->memory + iterator->node) : NULL;

    // If node is null, this means we should start with the smallest first of all nodes
    if (!node) {
        // If the structure has not been managed before, you can't start from head
        if (meta->lastNode == 0)
            return result;

        // Retrieve head-node - this is smallest first of all nodes
        node = (Node *) (radix->memory + sizeof(Meta));

        Item *item = node->item != 0 ? (Item *) (radix->memory + node->item) : NULL;

        // If matched node has item (not nullable) - this is the object you are looking for
        if (item && item->size > 0) {
            result.node = (uint8_t *)node - radix->memory;
            result.data = (uint8_t*)item + sizeof(Item);
            result.dataSize = item->size;

            return result;
        }
    }

    // Move to greater (shorter < longer)
    // Priority:
    // 1. child
    // 2. parent-child
    // 3. parent-parent-child
    //   no parent - return empty iterator
    while (true) {
        // Move to "child"
        if (node->childSmaller != 0 || node->childGreater != 0) {
            node = (Node *) (radix->memory + (node->childSmaller != 0 ? node->childSmaller : node->childGreater));

            Item *item = node->item != 0 ? (Item *) (radix->memory + node->item) : NULL;

            // If matched node has item (not nullable) - this is the object you are looking for
            if (item && item->size > 0) {
                result.node = (uint8_t *)node - radix->memory;
                result.data = (uint8_t*)item + sizeof(Item);
                result.dataSize = item->size;

                return result;
            }

            continue;
        }

        // Move to "parent-child" or "parent-parent-child"
        while (true) {
            Node *parentNode = node->parent != 0 ? (Node *) (radix->memory + node->parent) : NULL;

            // "no parent"
            if (!parentNode)
                return result;

            if (parentNode->childGreater != 0 && parentNode->childGreater != (uint8_t*)node - radix->memory) {
                node = (Node *) (radix->memory + parentNode->childGreater);

                break;
            }

            node = parentNode;
        }

        Item *item = node->item != 0 ? (Item *) (radix->memory + node->item) : NULL;

        // If matched node has item (not nullable) - this is the object you are looking for
        if (item && item->size > 0) {
            result.node = (uint8_t *)node - radix->memory;
            result.data = (uint8_t*)item + sizeof(Item);
            result.dataSize = item->size;

            return result;
        }
    }
}

RadixIterator radixNextNullable(RadixIterator *iterator)
{
    Radix *radix = iterator->radix;

    Meta *meta = (Meta *)radix->memory;

    RadixIterator result = {0};

    result.radix = radix;

    Node *node = iterator->node != 0 ? (Node *) (radix->memory + iterator->node) : NULL;

    // If node is null, this means we should start with the smallest first of all nodes
    if (!node) {
        // If the structure has not been managed before, you can't start from head
        if (meta->lastNode == 0)
            return result;

        // Retrieve head-node - this is smallest first of all nodes
        node = (Node *) (radix->memory + sizeof(Meta));

        Item *item = node->item != 0 ? (Item *) (radix->memory + node->item) : NULL;

        // If matched node has item (nullable) - this is the object you are looking for
        if (item) {
            result.node = (uint8_t *)node - radix->memory;
            result.data = (uint8_t*)item + sizeof(Item);
            result.dataSize = item->size;

            return result;
        }
    }

    // Move to greater (shorter < longer)
    // Priority:
    // 1. child
    // 2. parent-child
    // 3. parent-parent-child
    //   no parent - return empty iterator
    while (true) {
        // Move to "child"
        if (node->childSmaller != 0 || node->childGreater != 0) {
            node = (Node *) (radix->memory + (node->childSmaller != 0 ? node->childSmaller : node->childGreater));

            Item *item = node->item != 0 ? (Item *) (radix->memory + node->item) : NULL;

            // If matched node has item (nullable) - this is the object you are looking for
            if (item) {
                result.node = (uint8_t *)node - radix->memory;
                result.data = (uint8_t*)item + sizeof(Item);
                result.dataSize = item->size;

                return result;
            }

            continue;
        }

        // Move to "parent-child" or "parent-parent-child"
        while (true) {
            Node *parentNode = node->parent != 0 ? (Node *) (radix->memory + node->parent) : NULL;

            // "no parent"
            if (!parentNode)
                return result;

            if (parentNode->childGreater != 0 && parentNode->childGreater != (uint8_t*)node - radix->memory) {
                node = (Node *) (radix->memory + parentNode->childGreater);

                break;
            }

            node = parentNode;
        }

        Item *item = node->item != 0 ? (Item *) (radix->memory + node->item) : NULL;

        // If matched node has item (nullable) - this is the object you are looking for
        if (item) {
            result.node = (uint8_t *)node - radix->memory;
            result.data = (uint8_t*)item + sizeof(Item);
            result.dataSize = item->size;

            return result;
        }
    }
}

RadixIterator radixPrevInverse(RadixIterator *iterator)
{
    Radix *radix = iterator->radix;

    Meta *meta = (Meta *)radix->memory;

    RadixIterator result = {0};

    result.radix = radix;

    Node *node = iterator->node != 0 ? (Node *) (radix->memory + iterator->node) : NULL;

    // if node is null - start iteration by finding the greatest first of all nodes
    if (!node) {
        // If the structure has not been managed before, you can't start from head
        if (meta->lastNode == 0)
            return result;

        // Retrieve head-node - this is greatest first of all nodes
        node = (Node *) (radix->memory + sizeof(Meta));

        Item *item = node->item != 0 ? (Item *) (radix->memory + node->item) : NULL;

        // If matched node has item not nullable - this is the object you are looking for
        if (item && item->size > 0) {
            result.node = (uint8_t *)node - radix->memory;
            result.data = (uint8_t*)item + sizeof(Item);
            result.dataSize = item->size;

            return result;
        }
    }

    // Move to smaller (longer < shorter)
    // Priority:
    // 1. child
    // 2. parent-child
    // 3. parent-parent-child
    //   no parent - return empty iterator
    while (true) {
        // move to "child"
        if (node->childGreater != 0 || node->childSmaller != 0) {
            node = (Node *) (radix->memory + (node->childGreater != 0 ? node->childGreater : node->childSmaller));

            Item *item = node->item != 0 ? (Item *) (radix->memory + node->item) : NULL;

            // If matched node has item not nullable - this is the object you are looking for
            if (item && item->size > 0) {
                result.node = (uint8_t *)node - radix->memory;
                result.data = (uint8_t*)item + sizeof(Item);
                result.dataSize = item->size;

                return result;
            }

            continue;
        }

        // Move to "parent-child" or "parent-parent-child"
        while (true) {
            Node *parentNode = node->parent != 0 ? (Node *) (radix->memory + node->parent) : NULL;

            // "no parent"
            if (!parentNode)
                return result;

            if (parentNode->childSmaller != 0 && parentNode->childSmaller != (uint8_t*)node - radix->memory) {
                node = (Node *) (radix->memory + parentNode->childSmaller);

                break;
            }

            node = parentNode;
        }

        Item *item = node->item != 0 ? (Item *) (radix->memory + node->item) : NULL;

        // If matched node has item not nullable - this is the object you are looking for
        if (item && item->size > 0) {
            result.node = (uint8_t *)node - radix->memory;
            result.data = (uint8_t*)item + sizeof(Item);
            result.dataSize = item->size;

            return result;
        }
    }
}

RadixIterator radixPrevInverseNullable(RadixIterator *iterator)
{
    Radix *radix = iterator->radix;

    Meta *meta = (Meta *)radix->memory;

    RadixIterator result = {0};

    result.radix = radix;

    Node *node = iterator->node != 0 ? (Node *) (radix->memory + iterator->node) : NULL;

    // if node is null - start iteration by finding the greatest first of all nodes
    if (!node) {
        // If the structure has not been managed before, you can't start from head
        if (meta->lastNode == 0)
            return result;

        // Retrieve head-node - this is greatest first of all nodes
        node = (Node *) (radix->memory + sizeof(Meta));

        Item *item = node->item != 0 ? (Item *) (radix->memory + node->item) : NULL;

        // If matched node has item (nullable) - this is the object you are looking for
        if (item) {
            result.node = (uint8_t *)node - radix->memory;
            result.data = (uint8_t*)item + sizeof(Item);
            result.dataSize = item->size;

            return result;
        }
    }

    // Move to smaller (longer < shorter)
    // Priority:
    // 1. child
    // 2. parent-child
    // 3. parent-parent-child
    //   no parent - return empty iterator
    while (true) {
        // move to "child"
        if (node->childGreater != 0 || node->childSmaller != 0) {
            node = (Node *) (radix->memory + (node->childGreater != 0 ? node->childGreater : node->childSmaller));

            Item *item = node->item != 0 ? (Item *) (radix->memory + node->item) : NULL;

            // If matched node has item nullable - this is the object you are looking for
            if (item) {
                result.node = (uint8_t *)node - radix->memory;
                result.data = (uint8_t*)item + sizeof(Item);
                result.dataSize = item->size;

                return result;
            }

            continue;
        }

        // Move to "parent-child" or "parent-parent-child"
        while (true) {
            Node *parentNode = node->parent != 0 ? (Node *) (radix->memory + node->parent) : NULL;

            // "no parent"
            if (!parentNode)
                return result;

            if (parentNode->childSmaller != 0 && parentNode->childSmaller != (uint8_t*)node - radix->memory) {
                node = (Node *) (radix->memory + parentNode->childSmaller);

                break;
            }

            node = parentNode;
        }

        Item *item = node->item != 0 ? (Item *) (radix->memory + node->item) : NULL;

        // If matched node has item (nullable) - this is the object you are looking for
        if (item) {
            result.node = (uint8_t *)node - radix->memory;
            result.data = (uint8_t*)item + sizeof(Item);
            result.dataSize = item->size;

            return result;
        }
    }
}

RadixIterator radixNextInverse(RadixIterator *iterator)
{
    Radix *radix = iterator->radix;

    Meta *meta = (Meta *)radix->memory;

    RadixIterator result = {0};

    result.radix = radix;

    Node *node = iterator->node != 0 ? (Node *) (radix->memory + iterator->node) : NULL;

    // If node is null, this means we should start with the smallest leaf of all nodes
    if (!node) {
        // If the structure has not been managed before, you can't start from head
        if (meta->lastNode == 0)
            return result;

        // Retrieve head-node
        node = (Node *) (radix->memory + sizeof(Meta));

        // Move to the smallest leaf-node of given node
        while (node->childGreater != 0 || node->childSmaller != 0) {
            node = (Node *) (radix->memory + (node->childSmaller != 0 ? node->childSmaller : node->childGreater));
        }

        Item *item = node->item != 0 ? (Item *) (radix->memory + node->item) : NULL;

        // If matched node has item not nullable - this is the object you are looking for
        if (item && item->size > 0) {
            result.node = (uint8_t *)node - radix->memory;
            result.data = (uint8_t*)item + sizeof(Item);
            result.dataSize = item->size;

            return result;
        }
    }

    // Move to greater (longer < shorter)
    // Priority:
    // 1: parent-child-child (leaf child)
    // 2: parent-child
    // 3: parent
    //   no parent - return empty iterator
    while (true) {
        Node *parentNode = node->parent != 0 ? (Node *) (radix->memory + node->parent) : NULL;

        // "no parent"
        if (!parentNode)
            return result;

        // Move
        if (parentNode->childGreater != 0 && parentNode->childGreater != (uint8_t*)node - radix->memory) {
            node = (Node *) (radix->memory + parentNode->childGreater);

            // Move to "parent-child-child" or "parent-child"
            while (node->childGreater != 0 || node->childSmaller != 0) {
                node = (Node *) (radix->memory + (node->childSmaller != 0 ? node->childSmaller : node->childGreater));
            }
        } else {
            // Move to "parent"
            node = parentNode;
        }

        Item *item = node->item != 0 ? (Item *) (radix->memory + node->item) : NULL;

        // If matched node has item (not nullable) - this is the object you are looking for
        if (item && item->size > 0) {
            result.node = (uint8_t *)node - radix->memory;
            result.data = (uint8_t*)item + sizeof(Item);
            result.dataSize = item->size;

            return result;
        }
    }
}

RadixIterator radixNextInverseNullable(RadixIterator *iterator)
{
    Radix *radix = iterator->radix;

    Meta *meta = (Meta *)radix->memory;

    RadixIterator result = {0};

    result.radix = radix;

    Node *node = iterator->node != 0 ? (Node *) (radix->memory + iterator->node) : NULL;

    // If node is null, this means we should start with the smallest leaf of all nodes
    if (!node) {
        // If the structure has not been managed before, you can't start from head
        if (meta->lastNode == 0)
            return result;

        // Retrieve head-node
        node = (Node *) (radix->memory + sizeof(Meta));

        // Move to the smallest leaf-node of given node
        while (node->childGreater != 0 || node->childSmaller != 0) {
            node = (Node *) (radix->memory + (node->childSmaller != 0 ? node->childSmaller : node->childGreater));
        }

        Item *item = node->item != 0 ? (Item *) (radix->memory + node->item) : NULL;

        // If matched node has item (nullable) - this is the object you are looking for
        if (item) {
            result.node = (uint8_t *)node - radix->memory;
            result.data = (uint8_t*)item + sizeof(Item);
            result.dataSize = item->size;

            return result;
        }
    }

    // Move to greater (longer < shorter)
    // Priority:
    // 1: parent-child-child (leaf child)
    // 2: parent-child
    // 3: parent
    //   no parent - return empty iterator
    while (true) {
        Node *parentNode = node->parent != 0 ? (Node *) (radix->memory + node->parent) : NULL;

        // "no parent"
        if (!parentNode)
            return result;

        // Move
        if (parentNode->childGreater != 0 && parentNode->childGreater != (uint8_t*)node - radix->memory) {
            node = (Node *) (radix->memory + parentNode->childGreater);

            // Move to "parent-child-child" or "parent-child"
            while (node->childGreater != 0 || node->childSmaller != 0) {
                node = (Node *) (radix->memory + (node->childSmaller != 0 ? node->childSmaller : node->childGreater));
            }
        } else {
            // Move to "parent"
            node = parentNode;
        }

        Item *item = node->item != 0 ? (Item *) (radix->memory + node->item) : NULL;

        // If matched node has item (nullable) - this is the object you are looking for
        if (item) {
            result.node = (uint8_t *)node - radix->memory;
            result.data = (uint8_t*)item + sizeof(Item);
            result.dataSize = item->size;

            return result;
        }
    }
}

RadixIterator radixEarlier(RadixIterator *iterator)
{
    Radix *radix = iterator->radix;

    Meta *meta = (Meta *)radix->memory;

    RadixIterator result = {0};

    result.radix = radix;

    Node *node = iterator->node != 0 ? (Node *) (radix->memory + iterator->node) : NULL;

    // if node is null - start iteration by finding the latest node
    if (!node) {
        node = meta->lastNode != 0 ? (Node *) (radix->memory + meta->lastNode) : NULL;

        if (!node)
            return result;

        Item *item = node->item != 0 ? (Item *) (radix->memory + node->item) : NULL;

        // If node has item (not nullable) - this is the object you are looking for
        if (item && item->size > 0) {
            result.node = (uint8_t *)node - radix->memory;
            result.data = (uint8_t*)item + sizeof(Item);
            result.dataSize = item->size;

            return result;
        }
    }

    // Move to earlier node than given
    while (true) {
        node = node->lastNode != 0 ? (Node *) (radix->memory + node->lastNode) : NULL;

        if (!node)
            return result;

        Item *item = node->item != 0 ? (Item *) (radix->memory + node->item) : NULL;

        // If node has item (not nullable) - this is the object you are looking for
        if (item && item->size > 0) {
            result.node = (uint8_t *)node - radix->memory;
            result.data = (uint8_t*)item + sizeof(Item);
            result.dataSize = item->size;

            return result;
        }
    }
}

RadixIterator radixEarlierNullable(RadixIterator *iterator)
{
    Radix *radix = iterator->radix;

    Meta *meta = (Meta *)radix->memory;

    RadixIterator result = {0};

    result.radix = radix;

    Node *node = iterator->node != 0 ? (Node *) (radix->memory + iterator->node) : NULL;

    // if node is null - start iteration by finding the latest node
    if (!node) {
        node = meta->lastNode != 0 ? (Node *) (radix->memory + meta->lastNode) : NULL;

        if (!node)
            return result;

        Item *item = node->item != 0 ? (Item *) (radix->memory + node->item) : NULL;

        // If node has item (nullable) - this is the object you are looking for
        if (item) {
            result.node = (uint8_t *)node - radix->memory;
            result.data = (uint8_t*)item + sizeof(Item);
            result.dataSize = item->size;

            return result;
        }
    }

    // Move to earlier node than given
    while (true) {
        node = node->lastNode != 0 ? (Node *) (radix->memory + node->lastNode) : NULL;

        // If node has no earlier node - return empty iterator
        if (!node)
            return result;

        Item *item = node->item != 0 ? (Item *) (radix->memory + node->item) : NULL;

        // If node has item (nullable) - this is the object you are looking for
        if (item) {
            result.node = (uint8_t *)node - radix->memory;
            result.data = (uint8_t*)item + sizeof(Item);
            result.dataSize = item->size;

            return result;
        }
    }
}

RadixValue radixIteratorToValue(RadixIterator *iterator)
{
    Radix *radix = iterator->radix;

    RadixValue result = {0};

    result.radix = radix;

    Node *node = iterator->node != 0 ? (Node *) (radix->memory + iterator->node) : NULL;

    if (!node)
        return result;

    Item *item = node->item != 0 ? (Item *) (radix->memory + node->item) : NULL;

    if (!item)
        return result;

    result.item = (uint8_t*)item - radix->memory;
    result.data = (uint8_t*)item + sizeof(Item);
    result.dataSize = item->size;

    return result;
}

RadixCheckpoint radixIteratorToCheckpoint(RadixIterator *iterator)
{
    return (RadixCheckpoint) { .state = iterator->node };
}

bool radixIteratorIsEmpty(RadixIterator *iterator)
{
    return iterator->node == 0;
}

RadixValue radixValuePrevious(RadixValue *iterator)
{
    Radix *radix = iterator->radix;

    RadixValue result = {0};

    result.radix = radix;

    Item *item = iterator->item != 0 ? (Item *) (radix->memory + iterator->item) : NULL;

    while (item) {
        item = item->previous != 0 ? (Item *) (radix->memory + item->previous) : NULL;

        if (item && item->size > 0) {
            result.item = (uint8_t *)item - radix->memory;
            result.data = (uint8_t *)item + sizeof(Item);
            result.dataSize = item->size;

            return result;
        }
    }

    return result;
}

RadixValue radixValuePreviousNullable(RadixValue *iterator)
{
    Radix *radix = iterator->radix;

    RadixValue result = {0};

    result.radix = radix;

    Item *item = iterator->item != 0 ? (Item *) (radix->memory + iterator->item) : NULL;

    while (item) {
        item = item->previous != 0 ? (Item *) (radix->memory + item->previous) : NULL;

        if (item) {
            result.item = (uint8_t *)item - radix->memory;
            result.data = (uint8_t *)item + sizeof(Item);
            result.dataSize = item->size;

            return result;
        }
    }

    return result;
}

RadixValue radixValueEarlier(RadixValue *iterator)
{
    Radix *radix = iterator->radix;

    Meta *meta = (Meta *)radix->memory;

    RadixValue result = {0};

    result.radix = radix;

    Item *item = iterator->item != 0 ? (Item *) (radix->memory + iterator->item) : NULL;

    // if item is null - start iteration by finding the latest value
    if (!item) {
        item = meta->lastItem != 0 ? (Item *) (radix->memory + meta->lastItem) : NULL;

        if (item && item->size > 0) {
            result.item = (uint8_t *)item - radix->memory;
            result.data = (uint8_t*)item + sizeof(Item);
            result.dataSize = item->size;

            return result;
        }
    }

    // Move to earlier item than given
    while (item) {
        item = item->lastItem != 0 ? (Item *) (radix->memory + item->lastItem) : NULL;

        if (item && item->size > 0) {
            result.item = (uint8_t *)item - radix->memory;
            result.data = (uint8_t*)item + sizeof(Item);
            result.dataSize = item->size;

            return result;
        }
    }

    return result;
}

RadixValue radixValueEarlierNullable(RadixValue *iterator)
{
    Radix *radix = iterator->radix;

    Meta *meta = (Meta *)radix->memory;

    RadixValue result = {0};

    result.radix = radix;

    Item *item = iterator->item != 0 ? (Item *) (radix->memory + iterator->item) : NULL;

    // if item is null - start iteration by finding the latest value
    if (!item) {
        item = meta->lastItem != 0 ? (Item *) (radix->memory + meta->lastItem) : NULL;

        if (item) {
            result.item = (uint8_t *)item - radix->memory;
            result.data = (uint8_t*)item + sizeof(Item);
            result.dataSize = item->size;

            return result;
        }
    }

    // Move to earlier item than given
    while (item) {
        item = item->lastItem != 0 ? (Item *) (radix->memory + item->lastItem) : NULL;

        if (item) {
            result.item = (uint8_t *)item - radix->memory;
            result.data = (uint8_t*)item + sizeof(Item);
            result.dataSize = item->size;

            return result;
        }
    }

    return result;
}

RadixIterator radixValueToIterator(RadixValue *iterator)
{
    Radix *radix = iterator->radix;

    RadixIterator result = {0};

    result.radix = radix;

    Item *item = iterator->item != 0 ? (Item *) (radix->memory + iterator->item) : NULL;

    if (!item)
        return result;

    result.node = item->node;
    result.data = iterator->data;
    result.dataSize = iterator->dataSize;

    return result;
}

RadixCheckpoint radixValueToCheckpoint(RadixValue *iterator)
{
    return (RadixCheckpoint) { .state = iterator->item };
}

bool radixValueIsEmpty(RadixValue *iterator)
{
    return iterator->item == 0;
}

size_t radixKeyBits(RadixIterator *iterator)
{
    Radix *radix = iterator->radix;

    Node *node = iterator->node != 0 ? (Node *) (radix->memory + iterator->node) : NULL;

    size_t keyBits = 0;

    while(node) {
        keyBits += 8 * (node->keyRear - node->keyFore) + node->keyRearOffset - node->keyForeOffset;

        node = node->parent != 0 ? (Node *) (radix->memory + node->parent) : NULL;
    }

    return keyBits;
}

RadixError radixKeyCopy(RadixIterator *iterator, uint8_t *outputKey, size_t keyBits)
{
    Radix *radix = iterator->radix;

    Node *node = iterator->node != 0 ? (Node *) (radix->memory + iterator->node) : NULL;

    while (node) {
        size_t nodeKeyBits = 8 * (node->keyRear - node->keyFore) + node->keyRearOffset - node->keyForeOffset;

        // If its out of memory.. copy only suffix of nodeKey and return error
        if (keyBits < nodeKeyBits) {
            size_t nodeKeySuffixOffset = nodeKeyBits + node->keyForeOffset - keyBits;

            bitCopy((uint8_t *) (radix->memory + node->keyFore), nodeKeySuffixOffset, outputKey, keyBits, keyBits);

            return RADIX_OUT_OF_MEMORY;
        }

        keyBits -= nodeKeyBits;

        bitCopy((uint8_t *) (radix->memory + node->keyFore), node->keyForeOffset, outputKey, keyBits, nodeKeyBits);

        node = node->parent != 0 ? (Node *) (radix->memory + node->parent) : NULL;
    }

    return RADIX_SUCCESS;
}

RadixCheckpoint radixCheckpoint(Radix *radix)
{
    Meta *meta = (Meta *)radix->memory;

    return (RadixCheckpoint) { .state = meta->structureEnd };
}

void radixCheckpointRestore(Radix *radix, RadixCheckpoint *checkpoint)
{
    Meta *meta = (Meta *)radix->memory;

    // Restore items
    while (meta->lastItem >= checkpoint->state) {
        Item *item = meta->lastItem != 0 ? (Item *) (radix->memory + meta->lastItem) : NULL;

        if (!item)
            break;

        // Restore item meta
        meta->lastItem = item->lastItem;

        // Restore item owner
        Node *node = (Node *) (radix->memory + item->node); // all items ​​have an owner, so we don't need to check it

        node->item = item->previous;
    }

    // Restore nodes
    while (meta->lastNode >= checkpoint->state) {
        Node *node = meta->lastNode != 0 ? (Node *) (radix->memory + meta->lastNode) : NULL;

        if (!node)
            break;

        // Restore node meta
        meta->lastNode = node->lastNode;

        // Restore node child (node is splitting node)
        bool direction = bitGet(radix->memory + node->keyFore, node->keyForeOffset);

        size_t splittedNodeAddress = node->childSmaller != 0 ? node->childSmaller : node->childGreater;

        Node *splittedNode = splittedNodeAddress != 0 ? (Node *) (radix->memory + splittedNodeAddress) : NULL;

        if (splittedNode) {
            splittedNode->parent = node->parent;
            splittedNode->keyFore = node->keyFore;
            splittedNode->keyForeOffset = node->keyForeOffset;
        }

        // Restore node parent (taking into account whether a node is a splitting node)
        Node *parentNode = node->parent != 0 ? (Node *) (radix->memory + node->parent) : NULL;

        if (parentNode) {
            size_t *parentNodeChildReference = direction ? &(parentNode->childGreater) : &(parentNode->childSmaller);

            *parentNodeChildReference = splittedNode ? (uint8_t *)splittedNode - radix->memory : 0;
        }
    }

    // Restore meta structure
    meta->structureEnd = checkpoint->state;
}

RadixError radixClear(Radix* radix)
{
    size_t neededMemory = sizeof(Meta);

    if (neededMemory > radix->memorySize) return RADIX_OUT_OF_MEMORY;

    Meta *meta = (Meta *)radix->memory;

    meta->lastNode = 0;
    meta->lastItem = 0;
    meta->structureEnd = neededMemory;

    return RADIX_SUCCESS;
}

size_t radixMemoryUsage(Radix *radix)
{
    Meta *meta = (Meta *)radix->memory;

    return meta->structureEnd;
}
