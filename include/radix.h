#ifndef __RADIX_H
#define __RADIX_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

typedef enum RadixError {
    RADIX_SUCCESS,
    RADIX_OUT_OF_MEMORY,
} RadixError;

/**
 * Radix
 *  This structure provides information about the resources of radix.
 *
 *  Readonly!
 *
 *  Pass radix with data about the memory it
 *  should operate on and start using it!
 *  @see radixCreate
 *
 *  If you assign randomly filled memory, remember to clear the radix state.
 *  Memory filled with zeros is a cleared radix state.
 *  @see radixClear
 */
typedef struct Radix {
    unsigned char *memory;
    size_t memorySize;
} Radix;

/**
 * Radix Value Iterator
 *  This structure provides information about radix value
 *
 *  Readonly!
 *
 *  @see radixValuePrevious
 *  @see radixIteratorToValue
 *  @see radixValueToIterator
 */
typedef struct RadixValue {
    Radix *radix;
    size_t item;

    unsigned char *data;
    size_t dataSize;
} RadixValue;

/**
 * Radix Iterator
 *  This structure provides information about radix node
 *
 *  Readonly!
 *
 *  @see radixPrev
 *  @see radixNext
 *  @see radixPrevInver
 *  @see radixNextInver
 *  @see radixIteratorToValue
 *  @see radixValueToIterator
 */
typedef struct RadixIterator {
    Radix *radix;
    size_t node;

    unsigned char *data;
    size_t dataSize;
} RadixIterator;

/**
 * Radix Match
 *  This structure provides information about match
 *
 *  only the exact match function can return match on a nullable object
 *  @see radixMatch
 *
 *  Readonly!
 *
 *  @see radixMatch
 *  @see radixMatchFirst
 *  @see radixMatchLongest
 *  @see radixMatchToIterator
 */
typedef struct RadixMatch {
    Radix *radix;
    size_t node;

    size_t matchedBits;

    unsigned char *data;
    size_t dataSize;
} RadixMatch;

/**
 * Radix Checkpoint
 *  This structure contains information needed to restore the radix tree state.
 *
 *  Readonly!
 *
 *  @see radixCheckpoint
 *  @see radixCheckpointRestore
 */
typedef struct RadixCheckpoint {
    size_t state;
} RadixCheckpoint;

/**
 * Radix Create
 *  This function creates a radix tree object.
 *
 *  @param memory pointer to allocated memory
 *  @return RadixIterator object
 */
Radix radixCreate(unsigned char *memory, size_t memorySize);

/**
 * Radix Iterator
 *  This function creates a radix tree iterator.
 *
 *  When this function is called, the iterator
 *  is set to not existing object (empty).
 *
 *  @param radix radix tree
 *  @return RadixIterator object
 */
RadixIterator radixIterator(Radix *radix);

/**
 * Radix Value Iterator
 *  This function creates a radix value iterator.
 *
 *  When this function is called, the iterator
 *  is set to not existing object (empty).
 *
 *  @param radix radix tree
 *  @return RadixValue object
 */
RadixValue radixValueIterator(Radix *radix);

/**
 * Radix Insert
 *  This function inserts the data pointer under the given key relative to the
 *  given iterator. If there is already a data with the given key, it is
 *  shadowing with the new one.
 *
 *  @param iterator radix tree iterator
 *  @param key pointer to key
 *  @param keyBits key size (in bits)
 *  @param data pointer to data
 *  @param dataSize data size (in bytes)
 *  @return success or specific error
 */
RadixValue radixInsert(RadixIterator* iterator, unsigned char *key, size_t keyBits, unsigned char *data, size_t dataSize);

/**
 * Radix Record Remove
 *  This function does not actually delete the record.
 *  Inserts new null data for this key.
 *
 *  @param iterator radix tree iterator
 *  @param key pointer to key
 *  @param keyBits key size (in bits)
 *  @return success or specific error
 */
RadixValue radixRemove(RadixIterator* iterator, unsigned char *key, size_t keyBits);

/**
 * Radix Match Record
 *  this function looks for a record with the exactly matching key.
 *  returns only an match with non-null data.
 *  Match object can be empty.
 *
 *  @param iterator radix tree iterator
 *  @param key pointer to key
 *  @param keyBits key size (in bits)
 *  @return RadixMatch object
 */
RadixMatch radixMatch(RadixIterator *iterator, unsigned char *key, size_t keyBits);

/**
 * Radix Match Record
 *  this function looks for a record with the exactly matching key.
 *  returns even if the data is null.
 *  Match object can be empty.
 *
 *  @param iterator radix tree iterator
 *  @param key pointer to key
 *  @param keyBits key size (in bits)
 *  @return RadixMatch object
 */
RadixMatch radixMatchNullable(RadixIterator *iterator, unsigned char *key, size_t keyBits);

/**
 * Radix Match First Record
 *  This function looks for the record with the first matching key.
 *  returns only an match with non-null data.
 *  Match object can be empty.
 *
 *  @param radix radix tree
 *  @param key pointer to key
 *  @param keyBits key size (in bits)
 *  @return RadixMatch object
 */
RadixMatch radixMatchFirst(RadixIterator *iterator, unsigned char *key, size_t keyBits);

/**
 * Radix Match First Record
 *  This function looks for the record with the first matching key.
 *  returns even if the data is null.
 *  Match object can be empty.
 *
 *  @param radix radix tree
 *  @param key pointer to key
 *  @param keyBits key size (in bits)
 *  @return RadixMatch object
 */
RadixMatch radixMatchFirstNullable(RadixIterator *iterator, unsigned char *key, size_t keyBits);

/**
 * Radix Match Longest Record
 *  This function looks for a record with the longest matching key.
 *  returns only an match with non-null data.
 *  Match object can be empty.
 *
 *  @param radix radix tree
 *  @param key pointer to key
 *  @param keyBits key size (in bits)
 *  @return RadixMatch object
 */
RadixMatch radixMatchLongest(RadixIterator *iterator, unsigned char *key, size_t keyBits);

/**
 * Radix Match Longest Record
 *  This function looks for a record with the longest matching key.
 *  returns even if the data is null.
 *  Match object can be empty.
 *
 *  @param radix radix tree
 *  @param key pointer to key
 *  @param keyBits key size (in bits)
 *  @return RadixMatch object
 */
RadixMatch radixMatchLongestNullable(RadixIterator *iterator, unsigned char *key, size_t keyBits);

/**
 * Radix Match To Iterator
 *  This function converts match object to iterator object.
 *
 *  @param match radix match object
 *  @return RadixIterator object
 */
RadixIterator radixMatchToIterator(RadixMatch *match);

/**
 * Radix Match Is Empty
 *  This function returns information whether the match
 *  object points to a specific record in the structure.
 *
 *  @param match radix match object
 *  @return bool
 */
bool radixMatchIsEmpty(RadixMatch *match);

/**
 * Radix Predecessor (lexicographical order)
 *  This function will return an iterator pointing to predecessor (parent).
 *  returns only an iterator with non-null data.
 *
 *  Passing an empty iterator returns an empty iterator.
 *
 *  @param iterator radix iterator
 *  @return radix iterator object
 */
RadixIterator radixPredecessor(RadixIterator *iterator);

/**
 * Radix Predecessor (lexicographical order)
 *  This function will return an iterator pointing to predecessor (parent).
 *  returns even if the value is null.
 *
 *  Passing an empty iterator returns an empty iterator.
 *
 *  @param iterator radix iterator
 *  @return radix iterator object
 */
RadixIterator radixPredecessorNullable(RadixIterator *iterator);

/**
 * Radix Prev (lexicographical order)
 *  This function will return an iterator pointing to smaller element.
 *  returns only an iterator with non-null data.
 *
 *  It assumes that shorter keys are smaller than longer ones.
 *
 *  Passing an empty iterator returns an iterator
 *  pointing to the largest element.
 *
 *  @param iterator radix iterator
 *  @return radix iterator object
 */
RadixIterator radixPrev(RadixIterator *iterator);

/**
 * Radix Prev (lexicographical order)
 *  This function will return an iterator pointing to the smaller element.
 *  returns even if the value is null.
 *
 *  It assumes that shorter keys are smaller than longer ones.
 *
 *  Passing an empty iterator returns an iterator
 *  pointing to the largest element.
 *
 *  @param iterator radix iterator
 *  @return radix iterator object
 */
RadixIterator radixPrevNullable(RadixIterator *iterator);

/**
 * Radix Next (lexicographical order)
 *  This function will return an iterator pointing to the greater element.
 *  returns only an iterator with non-null data.
 *
 *  It assumes that shorter keys are smaller than longer ones.
 *
 *  Passing an empty iterator returns an iterator
 *  pointing to the smallest element.
 *
 *  @param iterator radix iterator
 *  @return radix iterator object
 */
RadixIterator radixNext(RadixIterator *iterator);

/**
 * Radix Next (lexicographical order)
 *  This function will return an iterator pointing to the greater element.
 *  returns even if the value is null.
 *
 *  It assumes that shorter keys are smaller than longer ones.
 *
 *  Passing an empty iterator returns an iterator
 *  pointing to the smallest element.
 *
 *  @param iterator radix iterator
 *  @return radix iterator object
 */
RadixIterator radixNextNullable(RadixIterator *iterator);

/**
 * Radix Prev Inver (lexicographical order)
 *  This function will return an iterator pointing to the smaller element.
 *  returns only an iterator with non-null data.
 *
 *  It assumes that shorter keys are greater than longer ones.
 *
 *  Passing an empty iterator returns an iterator
 *  pointing to the largest element.
 *
 *  @param iterator radix iterator
 *  @return radix iterator object
 */
RadixIterator radixPrevInverse(RadixIterator *iterator);

/**
 * Radix Prev Inver (lexicographical order)
 *  This function will return an iterator pointing to the smaller element.
 *  returns even if the value is null.
 *
 *  It assumes that shorter keys are greater than longer ones.
 *
 *  Passing an empty iterator returns an iterator
 *  pointing to the largest element.
 *
 *  @param iterator radix iterator
 *  @return radix iterator object
 */
RadixIterator radixPrevInverseNullable(RadixIterator *iterator);

/**
 * Radix Next Inver (lexicographical order)
 *  This function will return an iterator pointing to the greater element.
 *  returns only an iterator with non-null data.
 *
 *  It assumes that shorter keys are greater than longer ones.
 *
 *  Passing an empty iterator returns an iterator
 *  pointing to the smallest element.
 *
 *  @param iterator radix iterator
 *  @return radix iterator object
 */
RadixIterator radixNextInverse(RadixIterator *iterator);

/**
 * Radix Next Inver (lexicographical order)
 *  This function will return an iterator pointing to the greater element.
 *  returns even if the value is null.
 *
 *  It assumes that shorter keys are greater than longer ones.
 *
 *  Passing an empty iterator returns an iterator
 *  pointing to the smallest element.
 *
 *  @param iterator radix iterator
 *  @return radix iterator object
 */
RadixIterator radixNextInverseNullable(RadixIterator *iterator);

/**
 * Radix Earlier (chronological order)
 *  This function will return an iterator pointing to the earlier element.
 *  returns only an iterator with non-null data.
 *
 *  Passing an empty iterator returns an iterator
 *  pointing to the latest element.
 *
 *  There is no radixLater function because it would require increasing
 *  memory usage.
 *
 *  @param iterator radix iterator
 *  @return radix iterator object
 */
RadixIterator radixEarlier(RadixIterator *iterator);

/**
 * Radix Earlier (chronological order)
 *  This function will return an iterator pointing to the earlier element.
 *  returns even if the value is null.
 *
 *  Passing an empty iterator returns an iterator
 *  pointing to the latest element.
 *
 *  There is no radixLaterNullable function because it would require increasing
 *  memory usage.
 *
 *  @param iterator radix iterator
 *  @return radix iterator object
 */
RadixIterator radixEarlierNullable(RadixIterator *iterator);

/**
 * Radix Iterator To Value
 *  This function converts iterator to value (current value).
 *
 *  @param iterator radix iterator object
 *  @return RadixValue object
 */
RadixValue radixIteratorToValue(RadixIterator *iterator);

/**
 * Radix Iterator To Checkpoint
 *  This function creates a checkpoint that can then be used to restore
 *  the radix to its state before the key was added to the structure.
 *  @see radixCheckpointRestore
 *
 *  @param iterator radix iterator object
 *  @return RadixCheckpoint object
 */
RadixCheckpoint radixIteratorToCheckpoint(RadixIterator *iterator);

/**
 * Radix Iterator Is Empty
 *  This function returns information whether the iterator
 *  object points to a specific record in the structure.
 *
 *  @param iterator radix iterator object
 *  @return bool
 */
bool radixIteratorIsEmpty(RadixIterator *iterator);

/**
 * Radix Value Previous
 *  This function returns a iterator pointing to previous value for this key.
 *  returns only an iterator with non-null data.
 *
 *  There is no radixValueLater function because it would require increasing
 *  memory usage.
 *
 *  @param value radix value iterator object
 *  @return RadixValue object
 */
RadixValue radixValuePrevious(RadixValue *iterator);

/**
 * Radix Value Previous
 *  This function returns a iterator pointing to previous value for this key.
 *  returns even if the value is null.
 *
 *  There is no radixValueLater function because it would require increasing
 *  memory usage.
 *
 *  @param value radix value iterator object
 *  @return RadixValue object
 */
RadixValue radixValuePreviousNullable(RadixValue *iterator);

/**
 * Radix Value Earlier (chronological order)
 *  This function will return an iterator pointing to the earlier element.
 *  returns only an iterator with non-null data.
 *
 *  Passing an empty value iterator returns an value iterator
 *  pointing to the latest element.
 *
 *  There is no radixValueLater function because it would require increasing
 *  memory usage.
 *
 *  @param iterator radix value iterator
 *  @return radix value iterator object
 */
RadixValue radixValueEarlier(RadixValue *iterator);

/**
 * Radix Value Earlier (chronological order)
 *  This function will return an value iterator pointing to the earlier element.
 *  returns even if the value is null.
 *
 *  Passing an empty value iterator returns an value iterator
 *  pointing to the latest element.
 *
 *  There is no radixValueLaterNullable function because it would require
 *  increasing memory usage.
 *
 *  @param iterator radix value iterator
 *  @return radix value iterator object
 */
RadixValue radixValueEarlierNullable(RadixValue *iterator);

/**
 * Radix Iterator To Value
 *  This function converts value to iterator.
 *
 *  @param iterator radix value iterator object
 *  @return RadixIterator object
 */
RadixIterator radixValueToIterator(RadixValue *iterator);

/**
 * Radix Value To Checkpoint
 *  This function creates a checkpoint that can then be used to restore
 *  the radix to its state before the value was added to the structure.
 *  @see radixCheckpointRestore
 *
 *  @param iterator radix value iterator object
 *  @return RadixCheckpoint object
 */
RadixCheckpoint radixValueToCheckpoint(RadixValue *iterator);

/**
 * Radix Value Is Empty
 *  This function returns information whether the iterator
 *  object points to a specific record in the structure.
 *
 *  @param iterator radix value iterator object
 *  @return bool
 */
bool radixValueIsEmpty(RadixValue *iterator);

/**
 * Radix Key Bits
 *  This function returns the number of key bits.
 *
 *  @param iterator radix iterator
 *  @return bit count of key from the given iterator
 */
size_t radixKeyBits(RadixIterator *iterator);

/**
 * Radix Key Copy
 *  This function copies the key to the given address.
 *
 *  It is best to provide the result of
 *  the included function as the keyBits.
 *  @see radixKeyBits
 *
 *  Error returned means that an incomplete key (suffix)
 *  was copied to outputKey.
 *
 *  @param iterator radix iterator
 *  @param outputKey pointer to memory where the key will be copied
 *  @param keyBits key size (in bits)
 *  @return success or specific error
 */
RadixError radixKeyCopy(RadixIterator *iterator, unsigned char *outputKey, size_t keyBits);

/**
 * Radix Checkpoint
 *  This function creates a checkpoint to restore in future.
 *  @see radixCheckpointRestore
 *
 *  @param radix radix tree
 *  @return RadixCheckpoint object
 */
RadixCheckpoint radixCheckpoint(Radix *radix);

/**
 * Radix Checkpoint Restore
 *  This function undoes changes to the structure that occurred between
 *  the creation of the checkpoint and the call of the function.
 *
 *  Remember that iterators and checkpoints created
 *  after the checkpoint will not work properly.
 *
 *  @param radix radix tree
 *  @param checkpoint radix checkpoint
 *  @return void - the function may fail, but it will never report it
 */
void radixCheckpointRestore(Radix *radix, RadixCheckpoint *checkpoint);

/**
 * Radix Clear
 *  This function clears the contents of the structure.
 *
 *  Calling this function may be necessary if random memory is assigned
 *  to the radix object.
 *
 *  Setting all memory bits to zero results in clearing the structure.
 *
 *  Remember that iterators and checkpoints after that will not work properly.
 *
 *  @param radix radix tree
 *  @return success or specific error
 */
RadixError radixClear(Radix *radix);

/**
 * Radix Memory Usage
 *  This function returns the size of structure memory in use.
 *  e.g. for serialization to a file.
 *
 *  @param radix radix tree
 *  @return size of memory in use (in bytes)
 */
size_t radixMemoryUsage(Radix *radix);

#endif
