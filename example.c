/* mt64_test.c - tests for 64-bit Mersenne Twister implementation.
 *
 */

#include <stdio.h>
#include <stdlib.h>    // EXIT_FAILURE
#include <inttypes.h> // PRIu64
#include "mt64.h"
#include "dedup.h"
#include "bbhash.h"


int main(int argc, char* argv[argc])
{
    uint64_t nelem = 1000000;
    //uint64_t nthreads = 1;

    if(argc != 3 ) {
        printf("Usage :\n");
        printf("%s <nelem> <nthreads> \n", argv[0]);
        return EXIT_FAILURE;
    }

    if(argc == 3 ) {
        nelem = strtoul(argv[1], NULL, 0);
        //nthreads = atoi(argv[2]);
    }


    Mt64 *rng = mt64_create_default();
    uint64_t rab = 100;

    uint64_t *data = (uint64_t * ) calloc(nelem + rab, sizeof(uint64_t));
    for (uint64_t i = 0; i < nelem + rab; i++) {
        data[i] = mt64_gen_int64(rng);
    }
    size_t new_size = dedup(data, nelem + rab);

    printf("Found %" PRIu64 " duplicated elements\n", nelem + rab - new_size );
    if (new_size < nelem) {
        printf("Failed to create %" PRIu64 "elements", nelem);
        return EXIT_FAILURE;
    }

    BBHashLevel *mpf = bbhash_mphf_create(data, nelem);

    if (true) {
        printf("checking mpf\n"); fflush(stdout);
        bool *seen = (bool *)calloc(nelem, sizeof(bool));
        if (seen == NULL) {
            // Handle allocation failure
            printf("Failed to allocate 'seen' array\n");
            bbhash_level_free(mpf); 
            mt64_destroy(rng);
            free(data);
            return EXIT_FAILURE;
        }

        for (size_t i = 0; i < nelem; i++) {
            size_t idx = bbhash_mphf_query(mpf, data[i]);
            //printf("%llu -> %zu\n", data[i], bbhash_mphf_query(mpf,data[i]));
            if (idx > nelem || seen[idx]) {
                printf("Invalid mpf - index: %zu\n", idx);
            } else {
                seen[idx] = true;
            }
        }
        for (size_t i = 0; i < nelem; i++) {
            if (!seen[i]) {
                printf("idx %zu not used\n", i);
            }
        }
        printf("done\n"); fflush(stdout);
        free(seen);
    }

    mt64_destroy(rng);
    return 0;
}

