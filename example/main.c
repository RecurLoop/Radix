#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

#include <radix.h>

typedef struct TestCase {
    char *key;
    char *data;
} TestCase;

int main()
{
    // Prepare radix structure
    size_t radixMemorySize = 1024 * 20; // 20 KiB
    uint8_t *radixMemory = malloc(radixMemorySize);

    Radix radix = radixCreate(
        #if RADIX_REVERT
        radixMemory + radixMemorySize,
        #else
        radixMemory,
        #endif
        radixMemorySize
    );

    // Clear memory - show radixClear functionality
    if (radixClear(&radix))
        printf("ERROR (Clear): Out of memory!\n");

    // Prepare radix iterator (will be empty) - show radixIterator functionality
    RadixIterator iterator = radixIterator(&radix);

    // Store helper variables
    char *keyForOverride = "Key for override";

    // Insert 1st set of keys - show radixInsert functionality
    TestCase cases[] = {
        (TestCase) {"Key-a",   " Value-a"},
        (TestCase) {"Key-aa",  " Value-aa"},
        (TestCase) {"Key-ab",  " Value-ab"},
        (TestCase) {"Key-ac",  " Value-ac"},
        (TestCase) {"Key-b",   " Value-b"},
        (TestCase) {"Key-ba",  " Value-ba"},
        (TestCase) {"Key-bb",  " Value-bb"},
        (TestCase) {"Key-bc",  " Value-bc"},
        (TestCase) {"Key-c",   " Value-c"},
        (TestCase) {"Key-ca",  " Value-ca"},
        (TestCase) {"Key-cb",  " Value-cb"},
        (TestCase) {"Key-cc",  " Value-cc"},

        (TestCase) {keyForOverride, "will be override"},
        (TestCase) {"Key with NULL value", NULL},
    };

    for (size_t i = 0; i < sizeof(cases)/sizeof(cases[0]); i++) {
        uint8_t *key = (uint8_t *)cases[i].key;
        size_t keySize = strlen(cases[i].key) * 8;
        uint8_t *data = (uint8_t *)cases[i].data;
        size_t dataSize = data == NULL ? 0 : strlen(cases[i].data) + 1;

        RadixValue insertValue = radixInsert(&iterator, key, keySize, data, dataSize);

        if (radixValueIsEmpty(&insertValue))
            printf("ERROR (Insert): Out of memory! (key: %s, value: %s)\n", cases[i].key, cases[i].data);
    }

    // Check value of key "Key for override"
    RadixMatch overrideKeyMatch = radixMatch(&iterator, (uint8_t *)keyForOverride, strlen(keyForOverride) * CHAR_BIT);

    if (radixMatchIsEmpty(&overrideKeyMatch)) {
        printf("ERROR (Match): there is no key \"%s\"\n\n", keyForOverride);
    } else {
        printf("Key:\"%s\" after insert 1st set of keys has value  : \"%s\"\n\n", keyForOverride, overrideKeyMatch.data);
    }

    // Make checkpoint after 1st set of keys - show radixCheckpoint functionality
    RadixCheckpoint checkpoint = radixCheckpoint(&radix);

    // Insert 2nd set of keys
    {
        TestCase cases[] = {
            (TestCase) {"Key a",   "a2"},
            (TestCase) {"Key daa", "daa"},

            (TestCase) {keyForOverride, "has been overwritten"},
        };

        for (size_t i = 0; i < sizeof(cases)/sizeof(cases[0]); i++) {
            uint8_t *key = (uint8_t *)cases[i].key;
            size_t keySize = strlen(cases[i].key) * 8;
            uint8_t *data = (uint8_t *)cases[i].data;
            size_t dataSize = data == NULL ? 0 : strlen(cases[i].data) + 1;

            RadixValue insertValue = radixInsert(&iterator, key, keySize, data, dataSize);

            if (radixValueIsEmpty(&insertValue))
                printf("ERROR (Insert): Out of memory! (key: %s, value: %s)\n", cases[i].key, cases[i].data);
        }
    }

    // Check value of key "Key for override" - show radixMatch functionality
    overrideKeyMatch = radixMatch(&iterator, (uint8_t *)keyForOverride, strlen(keyForOverride) * CHAR_BIT);

    if (radixMatchIsEmpty(&overrideKeyMatch)) {
        printf("ERROR (Match): there is no key \"%s\"\n\n", keyForOverride);
    } else {
        printf("Key:\"%s\" after insert 2st set of keys has value  : \"%s\"\n\n", keyForOverride, overrideKeyMatch.data);
    }

    // Check previous value for key "Key for override"
    // - show radixMatchToIterator, radixIteratorToValue, radixValuePrevious functionality
    RadixIterator matchIterator = radixMatchToIterator(&overrideKeyMatch);

    RadixValue valueIterator = radixIteratorToValue(&matchIterator);

    valueIterator = radixValuePrevious(&valueIterator);

    if (radixValueIsEmpty(&valueIterator)) {
        printf("ERROR (Match): there is no previouse value for key \"%s\"\n\n", keyForOverride);
    } else {
        printf("Key:\"%s\" after insert 2st set of keys has previous value  : \"%s\"\n\n", keyForOverride, valueIterator.data);
    }

    // restore checkpoint after 1st set of keys - show radixCheckpointRestore functionality
    radixCheckpointRestore(&radix, &checkpoint);

    // Check value of key "Key for override"
    overrideKeyMatch = radixMatch(&iterator, (uint8_t *)keyForOverride, strlen(keyForOverride) * CHAR_BIT);
    if (radixMatchIsEmpty(&overrideKeyMatch)) {
        printf("ERROR (Match): there is no key \"%s\"\n\n", keyForOverride);
    } else {
        printf("Key:\"%s\" after restore to 1st set of keys has value: \"%s\"\n\n", keyForOverride, overrideKeyMatch.data);
    }

    // Remove "Key for override" - show radixRemove functionality
    RadixValue removeValue = radixRemove(&iterator, (uint8_t *)keyForOverride, strlen(keyForOverride) * CHAR_BIT);

    if (radixValueIsEmpty(&removeValue))
        printf("ERROR (Remove): Out of memory! (key: %s)\n\n", keyForOverride);
    else
        printf("Key: \"%s\" is removed from radix.\n\n", keyForOverride);

    // Show radixMatchFirst functionality
    printf("First match:\n");
    for (size_t i = 0; i < sizeof(cases)/sizeof(cases[0]); i++) {
        RadixMatch match = radixMatchFirst(&iterator, (uint8_t *)cases[i].key, strlen(cases[i].key) * 8);

        if (radixMatchIsEmpty(&match)) {
            printf(
                "There is no match with key (maybe try with nullable variant of match): \"%s\"!\tmatched bits: %lld \n",
                cases[i].key,
                match.matchedBits
            );
        } else {
            printf("key: %s\tvalue: %s \tmatched bits: %lld \n", cases[i].key, (char*)match.data, match.matchedBits);
        }
    }
    printf("\n");

    // Show radixMatchLongest functionality
    printf("Longest match:\n");
    for (size_t i = 0; i < sizeof(cases)/sizeof(cases[0]); i++) {
        RadixMatch match = radixMatchLongest(&iterator, (uint8_t *)cases[i].key, strlen(cases[i].key) * 8);

        if (radixMatchIsEmpty(&match)) {
            printf(
                "There is no match with key (maybe try with nullable variant of match): \"%s\"!\tmatched bits: %lld \n",
                cases[i].key,
                match.matchedBits
            );
        } else {
            printf("key: %s \tvalue: %s \tmatched bits: %lld \n", cases[i].key, (char*)match.data, match.matchedBits);
        }
    }
    printf("\n");

    // Show radixNext, radixKeySize, radixKeyBits, radixKeyCopy functionality
    printf("Iterator next:\n");
    for (RadixIterator it = radixNext(&iterator); !radixIteratorIsEmpty(&it); it = radixNext(&it)) {
        size_t keyBits = radixKeyBits(&it);
        size_t keySize = (keyBits + 8 - 1) / 8; // round up
        uint8_t *key = malloc(keySize + 1); //add 1 for the terminating character
        key[keySize] = 0; // set terminating character at end of key

        RadixError error = radixKeyCopy(&it, key, keyBits);

        printf(
            "key%s: %s\tdata: %s\n",
            error == RADIX_OUT_OF_MEMORY ? "(its only suffix, becouse there is not enought memory)" : "",
            key,
            (char*)it.data
        );

        free(key);
    }
    printf("\n");

    // Show radixPrev, radixKeySize, radixKeyBits, radixKeyCopy functionality
    printf("Iterator prev:\n");
    for (RadixIterator it = radixPrev(&iterator); !radixIteratorIsEmpty(&it); it = radixPrev(&it)) {
        size_t keyBits = radixKeyBits(&it);
        size_t keySize = (keyBits + 8 - 1) / 8; // round up
        uint8_t *key = malloc(keySize + 1); //add 1 for the terminating character
        key[keySize] = 0; // set terminating character at end of key

        RadixError error = radixKeyCopy(&it, key, keyBits);

        printf(
            "key%s: %s\tdata: %s\n",
            error == RADIX_OUT_OF_MEMORY ? "(its only suffix, becouse there is not enought memory)" : "",
            key,
            (char*)it.data
        );

        free(key);
    }
    printf("\n");

    // Show radixNextInverse, radixKeySize, radixKeyBits, radixKeyCopy functionality
    printf("Iterator next inverse (shorter keys are greater longer ones):\n");
    for (RadixIterator it = radixNextInverse(&iterator); !radixIteratorIsEmpty(&it); it = radixNextInverse(&it)) {
        size_t keyBits = radixKeyBits(&it);
        size_t keySize = (keyBits + 8 - 1) / 8; // round up
        uint8_t *key = malloc(keySize + 1); //add 1 for the terminating character
        key[keySize] = 0; // set terminating character at end of key

        RadixError error = radixKeyCopy(&it, key, keyBits);

        printf(
            "key%s: %s\tdata: %s\n",
            error == RADIX_OUT_OF_MEMORY ? "(its only suffix, becouse there is not enought memory)" : "",
            key,
            (char*)it.data
        );

        free(key);
    }
    printf("\n");

    // Show radixPrevInverse, radixKeySize, radixKeyBits, radixKeyCopy functionality
    printf("Iterator prev inverse (shorter keys are greater longer ones):\n");
    for (RadixIterator it = radixPrevInverse(&iterator); !radixIteratorIsEmpty(&it); it = radixPrevInverse(&it)) {
        size_t keyBits = radixKeyBits(&it);
        size_t keySize = (keyBits + 8 - 1) / 8; // round up
        uint8_t *key = malloc(keySize + 1); //add 1 for the terminating character
        key[keySize] = 0; // set terminating character at end of key

        RadixError error = radixKeyCopy(&it, key, keyBits);

        printf(
            "key%s: %s\tdata: %s\n",
            error == RADIX_OUT_OF_MEMORY ? "(its only suffix, becouse there is not enought memory)" : "",
            key,
            (char*)it.data
        );

        free(key);
    }
    printf("\n");


    // Show radixEarlier, radixKeySize, radixKeyBits, radixKeyCopy functionality
    printf("Iterator Earlier (chronological-reverse order):\n");
    for (RadixIterator it = radixEarlier(&iterator); !radixIteratorIsEmpty(&it); it = radixEarlier(&it)) {
        size_t keyBits = radixKeyBits(&it);
        size_t keySize = (keyBits + 8 - 1) / 8; // round up
        uint8_t *key = malloc(keySize + 1); //add 1 for the terminating character
        key[keySize] = 0; // set terminating character at end of key

        RadixError error = radixKeyCopy(&it, key, keyBits);

        printf(
            "key%s: %s\tdata: %s\n",
            error == RADIX_OUT_OF_MEMORY ? "(its only suffix, becouse there is not enought memory)" : "",
            key,
            (char*)it.data
        );

        free(key);
    }
    printf("\n");

    // Show radixMemoryUsage functionality
    printf("Radix Memory Usage: %lld", radixMemoryUsage(&radix));

    free(radixMemory);

    return 0;
}
