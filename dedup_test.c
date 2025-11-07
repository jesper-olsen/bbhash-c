/**
 * A C23 program to de-duplicate an array of uint64_t.
 *
 * This program uses the standard C library's qsort() function combined with
 * a single-pass "unique" scan to de-duplicate the array in-place.
 */

#include <stdio.h>    // For printf
#include <stdint.h>   // For uint64_t
#include <stddef.h>   // For size_t
#include <assert.h>   // For assert()
#include <string.h>   // For memcmp()
#include "dedup.h"


/**
 * @brief test function to demonstrate de-duplication.
 */
int test_dedup(void) {
    uint64_t data[] = {5, 1, 10, 2, 5, 5, 10, 8, 2, 100, 1, 8};
    size_t size = sizeof(data) / sizeof(data[0]);
    size_t new_size = dedup(data, size);

    uint64_t dedup_data[] = {1, 2, 5, 8, 10, 100};

    assert(new_size == 6);
    assert(memcmp(data, dedup_data, new_size * sizeof(uint64_t)) == 0);

    return 0;
}

int main() {
    test_dedup();
    return 0;
}
