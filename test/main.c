#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <radix.h>

typedef struct TestCase {
    unsigned char *key;
    unsigned char *data;
} TestCase;

int main()
{
    // Prepare radix
    size_t radixMemorySize = 1024 * 20; // 20 KiB
    unsigned char *radixMemory = malloc(radixMemorySize);

    Radix radix = radixCreate(radixMemory, radixMemorySize);

    if (radixClear(&radix)) {
        printf("ERROR (Clear): Out of memory!\n");
        return -1;
    }

    TestCase cases[] = {
        (TestCase) {"Key-a",   " Value-a"},
        (TestCase) {"Key-aa",  " Value-aa"},
        (TestCase) {"Key-aaa", " Value-aaa"},
        (TestCase) {"Key-aab", " Value-aab"},
        (TestCase) {"Key-aac", " Value-aac"},
        (TestCase) {"Key-ab",  " Value-ab"},
        (TestCase) {"Key-aba", " Value-aba"},
        (TestCase) {"Key-abb", " Value-abb"},
        (TestCase) {"Key-abc", " Value-abc"},
        (TestCase) {"Key-ac",  " Value-ac"},
        (TestCase) {"Key-aca", " Value-aca"},
        (TestCase) {"Key-acb", " Value-acb"},
        (TestCase) {"Key-acc", " Value-acc"},
        (TestCase) {"Key-b",   " Value-b"},
        (TestCase) {"Key-ba",  " Value-ba"},
        (TestCase) {"Key-baa", " Value-baa"},
        (TestCase) {"Key-bab", " Value-bab"},
        (TestCase) {"Key-bac", " Value-bac"},
        (TestCase) {"Key-bb",  " Value-bb"},
        (TestCase) {"Key-bba", " Value-bba"},
        (TestCase) {"Key-bbb", " Value-bbb"},
        (TestCase) {"Key-bbc", " Value-bbc"},
        (TestCase) {"Key-bc",  " Value-bc"},
        (TestCase) {"Key-bca", " Value-bca"},
        (TestCase) {"Key-bcb", " Value-bcb"},
        (TestCase) {"Key-bcc", " Value-bcc"},
        (TestCase) {"Key-c",   " Value-c"},
        (TestCase) {"Key-ca",  " Value-ca"},
        (TestCase) {"Key-caa", " Value-caa"},
        (TestCase) {"Key-cab", " Value-cab"},
        (TestCase) {"Key-cac", " Value-cac"},
        (TestCase) {"Key-cb",  " Value-cb"},
        (TestCase) {"Key-cba", " Value-cba"},
        (TestCase) {"Key-cbb", " Value-cbb"},
        (TestCase) {"Key-cbc", " Value-cbc"},
        (TestCase) {"Key-cc",  " Value-cc"},
        (TestCase) {"Key-cca", " Value-cca"},
        (TestCase) {"Key-ccb", " Value-ccb"},
        (TestCase) {"Key-ccc", " Value-ccc"},
        (TestCase) {"Key-null", NULL},
    };

    RadixIterator iterator = radixIterator(&radix);
    RadixValue valueIterator = radixValueIterator(&radix);

    for (size_t i = 0; i < sizeof(cases)/sizeof(cases[0]); i++) {
        unsigned char *key = cases[i].key;
        size_t keySize = strlen(cases[i].key) * 8;
        unsigned char *data = cases[i].data;
        size_t dataSize = data == NULL ? 0 : strlen(cases[i].data) + 1;

        RadixValue insertValue = radixInsert(&iterator, key, keySize, data, dataSize);

        if (radixValueIsEmpty(&insertValue)) {
            printf("ERROR (Insert): Out of memory! (key: %s, value: %s)\n", cases[i].key, cases[i].data);
            return -1;
        }
    }

    // Show radixMatchFirst functionality
    printf("First match:\n");
    for (size_t i = 0; i < sizeof(cases)/sizeof(cases[0]); i++) {
        RadixMatch match = radixMatchFirst(&iterator, cases[i].key, strlen(cases[i].key) * 8);

        if (radixMatchIsEmpty(&match)) {
            printf(
                "There is no match with key (maybe try with nullable variant of match): \"%s\"!\tmatched bits: %lld \n",
                cases[i].key,
                match.matchedBits
            );
        } else {
            printf("key: %s\tvalue: %s \tmatched bits: %lld \n", cases[i].key, match.data, match.matchedBits);
        }
    }
    printf("\n");

    // Show radixMatchLongest functionality
    printf("Longest match:\n");
    for (size_t i = 0; i < sizeof(cases)/sizeof(cases[0]); i++) {
        RadixMatch match = radixMatchLongest(&iterator, cases[i].key, strlen(cases[i].key) * 8);

        if (radixMatchIsEmpty(&match)) {
            printf(
                "There is no match with key (maybe try with nullable variant of match): \"%s\"!\tmatched bits: %lld \n",
                cases[i].key,
                match.matchedBits
            );
        } else {
            printf("key: %s \tvalue: %s \tmatched bits: %lld | ", cases[i].key, match.data, match.matchedBits);

            RadixIterator it = radixMatchToIterator(&match);

            it = radixPredecessor(&it);

            size_t keyBits = radixKeyBits(&it);
            size_t keySize = (keyBits + 8 - 1) / 8; // round up
            unsigned char *key = malloc(keySize + 1); //add 1 for the terminating character
            key[keySize] = 0; // set terminating character at end of key

            RadixError error = radixKeyCopy(&it, key, keyBits);

            printf(
                "predecessor%s: %s\tdata: %s\n",
                error == RADIX_OUT_OF_MEMORY ? "(its only suffix, becouse there is not enought memory)" : "",
                key,
                it.data
            );

            free(key);
        }
    }
    printf("\n");

    // Show radixNext, radixKeySize, radixKeyBits, radixKeyCopy functionality
    printf("Iterator next:\n");
    for (RadixIterator it = radixNext(&iterator); !radixIteratorIsEmpty(&it); it = radixNext(&it)) {
        size_t keyBits = radixKeyBits(&it);
        size_t keySize = (keyBits + 8 - 1) / 8; // round up
        unsigned char *key = malloc(keySize + 1); //add 1 for the terminating character
        key[keySize] = 0; // set terminating character at end of key

        RadixError error = radixKeyCopy(&it, key, keyBits);

        printf(
            "key%s: %s\tdata: %s\n",
            error == RADIX_OUT_OF_MEMORY ? "(its only suffix, becouse there is not enought memory)" : "",
            key,
            it.data
        );

        free(key);
    }
    printf("\n");

    // Show radixPrev, radixKeySize, radixKeyBits, radixKeyCopy functionality
    printf("Iterator prev:\n");
    for (RadixIterator it = radixPrev(&iterator); !radixIteratorIsEmpty(&it); it = radixPrev(&it)) {
        size_t keyBits = radixKeyBits(&it);
        size_t keySize = (keyBits + 8 - 1) / 8; // round up
        unsigned char *key = malloc(keySize + 1); //add 1 for the terminating character
        key[keySize] = 0; // set terminating character at end of key

        RadixError error = radixKeyCopy(&it, key, keyBits);

        printf(
            "key%s: %s\tdata: %s\n",
            error == RADIX_OUT_OF_MEMORY ? "(its only suffix, becouse there is not enought memory)" : "",
            key,
            it.data
        );

        free(key);
    }
    printf("\n");

    // Show radixNextInverse, radixKeySize, radixKeyBits, radixKeyCopy functionality
    printf("Iterator next inverse (shorter keys are greater longer ones):\n");
    for (RadixIterator it = radixNextInverse(&iterator); !radixIteratorIsEmpty(&it); it = radixNextInverse(&it)) {
        size_t keyBits = radixKeyBits(&it);
        size_t keySize = (keyBits + 8 - 1) / 8; // round up
        unsigned char *key = malloc(keySize + 1); //add 1 for the terminating character
        key[keySize] = 0; // set terminating character at end of key

        RadixError error = radixKeyCopy(&it, key, keyBits);

        printf(
            "key%s: %s\tdata: %s\n",
            error == RADIX_OUT_OF_MEMORY ? "(its only suffix, becouse there is not enought memory)" : "",
            key,
            it.data
        );

        free(key);
    }
    printf("\n");

    // Show radixPrevInverse, radixKeySize, radixKeyBits, radixKeyCopy functionality
    printf("Iterator prev inverse (shorter keys are greater longer ones):\n");
    for (RadixIterator it = radixPrevInverse(&iterator); !radixIteratorIsEmpty(&it); it = radixPrevInverse(&it)) {
        size_t keyBits = radixKeyBits(&it);
        size_t keySize = (keyBits + 8 - 1) / 8; // round up
        unsigned char *key = malloc(keySize + 1); //add 1 for the terminating character
        key[keySize] = 0; // set terminating character at end of key

        RadixError error = radixKeyCopy(&it, key, keyBits);

        printf(
            "key%s: %s\tdata: %s\n",
            error == RADIX_OUT_OF_MEMORY ? "(its only suffix, becouse there is not enought memory)" : "",
            key,
            it.data
        );

        free(key);
    }
    printf("\n");

    // Show radixEarlier, radixKeySize, radixKeyBits, radixKeyCopy functionality
    printf("Iterator Earlier (chronological-reverse order):\n");
    for (RadixIterator it = radixEarlier(&iterator); !radixIteratorIsEmpty(&it); it = radixEarlier(&it)) {
        size_t keyBits = radixKeyBits(&it);
        size_t keySize = (keyBits + 8 - 1) / 8; // round up
        unsigned char *key = malloc(keySize + 1); //add 1 for the terminating character
        key[keySize] = 0; // set terminating character at end of key

        RadixError error = radixKeyCopy(&it, key, keyBits);

        printf(
            "key%s: %s\tdata: %s\n",
            error == RADIX_OUT_OF_MEMORY ? "(its only suffix, becouse there is not enought memory)" : "",
            key,
            it.data
        );

        free(key);
    }
    printf("\n");

    // Show radixEarlier, radixKeySize, radixKeyBits, radixKeyCopy functionality
    printf("Value Iterator Earlier (chronological-reverse order):\n");
    for (RadixValue val = radixValueEarlier(&valueIterator); !radixValueIsEmpty(&val); val = radixValueEarlier(&val)) {
        RadixIterator it = radixValueToIterator(&val);

        size_t keyBits = radixKeyBits(&it);
        size_t keySize = (keyBits + 8 - 1) / 8; // round up
        unsigned char *key = malloc(keySize + 1); //add 1 for the terminating character
        key[keySize] = 0; // set terminating character at end of key


        RadixError error = radixKeyCopy(&it, key, keyBits);

        printf(
            "key%s: %s\tdata: %s\n",
            error == RADIX_OUT_OF_MEMORY ? "(its only suffix, becouse there is not enought memory)" : "",
            key,
            val.data
        );

        free(key);
    }
    printf("\n");

    // Show radixMemoryUsage functionality
    printf("Radix Memory Usage: %lld", radixMemoryUsage(&radix));

    free(radixMemory);

    return 0;
}
