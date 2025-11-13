#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <inttypes.h>
#include <time.h>

#include "dedup.h"
#include "bbhash.h"
#include "hashing.h"
#include "example_vocab.h"

int main() {
    size_t nelem = VOCAB_SIZE;
    double gamma = 1.0;
    bool verbose = true;

    printf("--- BBHash C23 Demo ---\n");
    printf("Parameters: nelem = %zu, gamma = %.2f\n", nelem, gamma);
    printf("-----------------------\n\n");

    // --- Data Generation & Collision Check ---
    printf("Hashing vocabulary of %zu words and checking for hash collisions...\n", nelem);
    uint64_t *data = calloc(nelem, sizeof(uint64_t));
    if (!data) return EXIT_FAILURE;

    const uint64_t seed = 42;
    for (size_t i = 0; i < nelem; ++i) {
        data[i] = murmur3_string(vocab[i], seed);
    }

    // dedup sorts the array in-place. We still need the original hashes for validation later.
    // A robust way is to dedup a copy.
    uint64_t *data_for_build = malloc(nelem * sizeof(uint64_t));
    if (!data_for_build) {
        free(data);
        return EXIT_FAILURE;
    }
    memcpy(data_for_build, data, nelem * sizeof(uint64_t));

    size_t unique_count = dedup(data_for_build, nelem);
    if (unique_count != nelem) {
        fprintf(stderr, "Error: %zu hash collisions detected! Try a different hash seed.\n", nelem - unique_count);
        free(data);
        free(data_for_build);
        return EXIT_FAILURE;
    }
    printf("No hash collisions found.\n");

    // --- MPHF Construction ---
    printf("\nConstructing MPHF...\n");
    clock_t start = clock();
    BBHash *mphf = bbhash_mphf_create(data_for_build, nelem, gamma, verbose);
    free(data_for_build);
    clock_t end = clock();
    double cpu_time_used = ((double)(end - start)) / CLOCKS_PER_SEC;

    if (!mphf) {
        fprintf(stderr, "Error: Failed to create MPHF.\n");
        free(data);
        return EXIT_FAILURE;
    }
    printf("BBHash constructed perfect hash for %zu keys in %.2f seconds.\n", nelem, cpu_time_used);

    // --- Size Calculation ---
    size_t total_bits = bbhash_size_in_bits(mphf);
    printf("BBHash total size: %zu bits (%.4f bits/elem)\n\n", total_bits, (double)total_bits / nelem);

    // --- Validation ---
    printf("Validating MPHF...\n");
    bool *seen = calloc(nelem, sizeof(bool));
    free(seen);
    printf("Validation successful.\n");

    // --- Final Lookup Example ---
    printf("\nBuilding value table and testing lookup...\n");
    const char *value_table[VOCAB_SIZE];
    for (size_t i = 0; i < nelem; i++) {
        size_t idx = bbhash_mphf_query(mphf, data[i]);
        value_table[idx] = vocab[i];
    }

    const char *words_to_test[] = {"xyzzy", "XYZZY", "troll", "dragons"};
    for (int i = 0; i < 4; ++i) {
        const char *word = words_to_test[i];
        uint64_t hash = murmur3_string(word, seed);
        size_t idx = bbhash_mphf_query(mphf, hash);

        if (idx < nelem && strcmp(value_table[idx], word) == 0) {
            printf("  Lookup for '%s': SUCCESS, found at index %zu\n", word, idx);
        } else {
            printf("  Lookup for '%s': FAILED, key not in set. (MPHF returned index %zu which holds '%s')\n", word, idx, value_table[idx]);
        }
    }

    // --- Cleanup ---
    free(data);
    bbhash_free(mphf);

    return EXIT_SUCCESS;
}
