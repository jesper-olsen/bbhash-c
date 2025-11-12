#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "bitarray.h"
#include "hashing.h"
#include "bbhash.h"

constexpr size_t MIN_BITARRAY_SIZE = 64;

typedef struct BBHashLevel BBHashLevel;
struct BBHashLevel {
    Bitarray *collision_free_set;
    size_t seed;
    size_t level_offset;
    BBHashLevel *next;
};

BBHashLevel *bbhash_level_new(size_t seed) {
    BBHashLevel *level = malloc(sizeof(BBHashLevel));
    if (!level) return NULL;
    level->seed = seed;
    level->level_offset = 0;
    level->collision_free_set = NULL;
    level->next = NULL;
    return level;
}

void bbhash_level_free(BBHashLevel *level) {
    if (level == NULL) {
        return;
    }
    if (level->collision_free_set != NULL) {
        bitarray_free(level->collision_free_set);
    }
    if (level->next) {
        bbhash_level_free(level->next);
    }
    free(level);
}

typedef struct BBHash {
    size_t num_keys;             // number of elements the MPHF was built for.
    struct BBHashLevel *levels;  // linked list
} BBHash;


size_t calc_level_size(size_t unplaced, double gamma) {
    size_t n = (size_t) unplaced * gamma;
    return n < MIN_BITARRAY_SIZE ? MIN_BITARRAY_SIZE : n;
}

BBHash *bbhash_mphf_create(const uint64_t data[], size_t unplaced, double gamma, bool verbose) {
    BBHash *mphf = malloc(sizeof(BBHash));
    if (!mphf) return NULL;
    mphf->levels = NULL;
    mphf->num_keys = unplaced;

    size_t *bucket_indexes = malloc(sizeof(size_t) * unplaced);
    if (bucket_indexes == NULL) {
        free(mphf);
        return NULL;
    }

    uint64_t *key_buffer = malloc(sizeof(uint64_t) * unplaced);
    if (key_buffer == NULL) {
        free(bucket_indexes);
        free(mphf);
        return NULL;
    }

    uint64_t *next_data = key_buffer;

    BBHashLevel *level0 = NULL;
    BBHashLevel *current_level = NULL;
    size_t placed = 0;  // number of keys perfectly mapped
    constexpr uint64_t INITIAL_SEED = 41;
    uint64_t current_seed = INITIAL_SEED;
    size_t level_size = calc_level_size(unplaced, gamma);
    Bitarray* used_slots = bitarray_new(level_size);

    while (unplaced > 0) {
        // --- Setup for the current level ---
        current_seed++;
        BBHashLevel *new_level = bbhash_level_new(current_seed);
        new_level->level_offset = placed;
        if (level0 == NULL) {
            level0 = new_level;
        } else {
            current_level->next = new_level;
        }
        current_level = new_level;

        size_t level_size = calc_level_size(unplaced, gamma);
        bitarray_shrink(used_slots, level_size);
        bitarray_clear_all(used_slots);
        Bitarray* colliding_slots = bitarray_new(level_size); // collisions

        for (size_t i = 0; i < unplaced; i++) {
            uint64_t hash = hash_with_seed(data[i], current_level->seed);
            size_t idx = hash % level_size;
            bucket_indexes[i] = idx;
            if (bitarray_get(used_slots, idx) == 1) {
                bitarray_set(colliding_slots, idx);
            } else {
                bitarray_set(used_slots, idx);
            }
        }

        bitarray_andnot(colliding_slots, used_slots, colliding_slots);
        current_level->collision_free_set = colliding_slots;

        size_t rank = 0;
        size_t next_level_unplaced = 0;
        for (size_t i = 0; i < unplaced ; i++) {
            size_t idx = bucket_indexes[i];
            if (bitarray_get(current_level->collision_free_set, idx) == 1) {
                rank++;
            } else {
                next_data[next_level_unplaced++] = data[i];
            }
        }
        data = next_data;
        unplaced = next_level_unplaced;
        placed += rank;

        if (verbose)
            printf("Level %llu; placed %zu; offset %zu\n", 
                    current_level->seed - INITIAL_SEED - 1, 
                    rank, 
                    current_level->level_offset);
    }

    bitarray_free(used_slots);
    free(key_buffer);
    free(bucket_indexes);

    mphf->levels = level0;
    return mphf;
}

size_t bbhash_size_in_bits(const BBHash *mphf) {
    if (mphf == NULL) {
        return 0;
    }

    size_t total_bits = 0;
    struct BBHashLevel *current_level = mphf->levels;

    while (current_level != NULL) {
        if (current_level->collision_free_set) {
            total_bits += current_level->collision_free_set->nbits;
        }
        current_level = current_level->next;
    }

    return total_bits;
}

/**
 * @brief Queries the BBHash MPHF for the unique integer hash of a key.
 *
 * @param level0 The pointer to the first level of the BBHash structure.
 * @param key    The key to query.
 * @return The unique hash value (from 0 to nelem-1) if the key was in the
 * original set. Otherwise, the behavior is undefined (it will
 * likely return an incorrect value or a "random" index).
 */
size_t bbhash_mphf_query(BBHash *mphf, uint64_t key) {
    BBHashLevel *current_level = mphf->levels;

    while (current_level != NULL) {
        size_t level_size = current_level->collision_free_set->nbits;
        uint64_t hash = hash_with_seed(key, current_level->seed);
        size_t idx = hash % level_size;
        if (bitarray_get(current_level->collision_free_set, idx) == 1) {
            size_t rank = bitarray_rank(current_level->collision_free_set, idx);
            return current_level->level_offset + rank;
        }
        current_level = current_level->next;
    }

    // Should not happen if the key was in the original set.
    // This indicates the key was not part of the set used to build the MPHF.
    // Returning (size_t)-1 (which is SIZE_MAX) is a common C idiom.
    return (size_t) -1;
}

void bbhash_free(BBHash *mphf) {
    if (mphf == NULL) {
        return;
    }
    bbhash_level_free(mphf->levels);
    free(mphf);
}
