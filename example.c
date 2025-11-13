#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <inttypes.h>
#include <time.h> // For clock()

#include "mt64.h"
#include "dedup.h"
#include "bbhash.h"

// Forward declaration for the usage function
void print_usage(const char* prog_name);

int main(int argc, char* argv[]) {
    // --- Default Parameters ---
    size_t nelem = 10000000;
    double gamma = 2.0; // 1.0 => smaller mphf, 2.0 larger, but faster construction.
    bool validate = false;
    bool nelem_set = false;
    bool verbose = true;

    // --- Argument Parsing ---
    if (argc < 2) {
        print_usage(argv[0]);
        return EXIT_FAILURE;
    }

    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            print_usage(argv[0]);
            return EXIT_SUCCESS;
        } else if (strcmp(argv[i], "-g") == 0 || strcmp(argv[i], "--gamma") == 0) {
            if (++i >= argc) {
                fprintf(stderr, "Error: Missing value for gamma.\n");
                return EXIT_FAILURE;
            }
            gamma = strtod(argv[i], NULL);
        } else if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--validate") == 0) {
            validate = true;
        } else {
            // Assume the first non-flag argument is the number of elements
            if (!nelem_set) {
                nelem = strtoul(argv[i], NULL, 0);
                nelem_set = true;
            } else {
                fprintf(stderr, "Error: Unknown argument '%s'.\n", argv[i]);
                print_usage(argv[0]);
                return EXIT_FAILURE;
            }
        }
    }

    if (!nelem_set) {
        fprintf(stderr, "Error: Number of elements not specified.\n");
        print_usage(argv[0]);
        return EXIT_FAILURE;
    }

    printf("--- BBHash C23 Demo ---\n");
    printf("Parameters: nelem = %zu, gamma = %.2f, validate = %s\n",
           nelem, gamma, validate ? "yes" : "no");
    printf("-----------------------\n\n");


    // --- Data Generation ---
    printf("Generating and de-duplicating initial key set...\n");
    Mt64 *rng = mt64_create_default();
    size_t buffer_size = nelem + (nelem / 100) + 100; // Generate 1% extra to account for duplicates
    uint64_t *data = calloc(buffer_size, sizeof(uint64_t));
    if (!data) {
        fprintf(stderr, "Failed to allocate memory for data.\n");
        return EXIT_FAILURE;
    }

    for (size_t i = 0; i < buffer_size; i++) {
        data[i] = mt64_gen_int64(rng);
    }
    size_t unique_count = dedup(data, buffer_size);

    printf("Found %zu duplicated elements.\n", buffer_size - unique_count);
    if (unique_count < nelem) {
        fprintf(stderr, "Error: Failed to generate %zu unique elements (only got %zu).\n", nelem, unique_count);
        free(data);
        mt64_destroy(rng);
        return EXIT_FAILURE;
    }

    // --- MPHF Construction & Timing ---
    printf("\nConstructing MPHF...\n");
    clock_t start = clock();
    BBHash *mphf = bbhash_mphf_create(data, nelem, gamma, verbose);
    clock_t end = clock();
    double cpu_time_used = ((double)(end - start)) / CLOCKS_PER_SEC;

    //bbhash_mphf_save(mphf, "mphf.bin");
    //bbhash_free(mphf);
    //mphf = bbhash_mphf_load("mphf.bin");

    if (!mphf) {
        fprintf(stderr, "Error: Failed to create MPHF.\n");
        free(data);
        mt64_destroy(rng);
        return EXIT_FAILURE;
    }
    printf("BBHash constructed perfect hash for %zu keys in %.2f seconds (CPU time).\n", nelem, cpu_time_used);

    // --- Size Calculation ---
    size_t total_bits = bbhash_size_in_bits(mphf);
    double bits_per_elem = (double)total_bits / nelem;
    printf("BBHash total size: %zu bits (%.2f MB)\n", total_bits, (double)total_bits / (8.0 * 1024 * 1024));
    printf("BBHash bits/elem : %.4f\n\n", bits_per_elem);


    // --- Validation (Optional) ---
    if (validate) {
        printf("Validating MPHF...\n");
        bool *seen = calloc(nelem, sizeof(bool));
        if (seen == NULL) {
            fprintf(stderr, "Failed to allocate 'seen' array for validation.\n");
        } else {
            bool success = true;
            for (size_t i = 0; i < nelem; i++) {
                size_t idx = bbhash_mphf_query(mphf, data[i]);
                if (idx >= nelem || seen[idx]) {
                    fprintf(stderr, "Validation failed! Key %" PRIu64 " -> index %zu (out of bounds or duplicate).\n", data[i], idx);
                    success = false;
                    break;
                }
                seen[idx] = true;
            }

            if (success) {
                // Check if all indices were hit
                for (size_t i = 0; i < nelem; i++) {
                    if (!seen[i]) {
                        fprintf(stderr, "Validation failed! Index %zu was not mapped.\n", i);
                        success = false;
                        break;
                    }
                }
            }
            if (success) {
                printf("Validation successful: all keys map to a unique index in [0, %zu).\n", nelem);
            }
            free(seen);
        }
    }

    // --- Cleanup ---
    free(data);
    bbhash_free(mphf);
    mt64_destroy(rng);

    return EXIT_SUCCESS;
}

void print_usage(const char* prog_name) {
    fprintf(stderr, "Usage: %s <num_elements> [options]\n\n", prog_name);
    fprintf(stderr, "  <num_elements>   Number of keys to build the MPHF for (required).\n\n");
    fprintf(stderr, "Options:\n");
    fprintf(stderr, "  -g, --gamma <f>  Set the gamma parameter (bits/key ratio). Default: 2.0\n");
    fprintf(stderr, "                   Lower values (e.g., 1.0) save space but are slower to build.\n");
    fprintf(stderr, "  -v, --validate   Verify that the generated MPHF is correct.\n");
    fprintf(stderr, "  -h, --help       Show this help message.\n");
}
