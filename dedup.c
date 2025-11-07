/**
 * A C23 program to de-duplicate an array of uint64_t.
 *
 * This program uses the standard C library's qsort() function combined with
 * a single-pass "unique" scan to de-duplicate the array in-place.
 */

#include <stdio.h>    // For printf
#include <stdint.h>   // For uint64_t
#include <stdlib.h>   // For qsort
#include <stddef.h>   // For size_t
#include <assert.h>   // For assert()
#include <string.h>   // For memcmp()

/**
 * @brief A comparison function for qsort, required for sorting uint64_t.
 */
int compare_u64(const void *a, const void *b) {
    uint64_t val_a = *(const uint64_t *)a;
    uint64_t val_b = *(const uint64_t *)b;
    return (val_a > val_b) - (val_a < val_b);
}

/**
 * @brief De-duplicates a given array of uint64_t in-place.
 * * This function first sorts the array, then moves all unique elements
 * to the beginning of the array.
 *
 * @param arr The array of uint64_t to de-duplicate.
 * @param size The number of elements in the array.
 * @return The new size of the array after de-duplication.
 */
size_t dedup(uint64_t arr[], size_t size) {
    if (size <= 1) {
        return size;   // already unique
    }

    qsort(arr, size, sizeof(uint64_t), compare_u64);

    size_t j = 1;  // write index - same as i until first duplicate
    for (size_t i = 1; i < size; i++) {
        if (arr[i] != arr[i - 1]) {
            arr[j] = arr[i];
            j++; 
        }
    }

    return j;
}

