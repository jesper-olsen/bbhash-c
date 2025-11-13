#include <string.h>
#include <stdlib.h>
#include <stdio.h>  // FILE operations
#include "bitarray.h"
#include "hashing.h"
#include "bbhash.h"

constexpr size_t MIN_BITARRAY_SIZE = 64;

typedef struct BBHashLevel BBHashLevel;
struct BBHashLevel {
    Bitarray *collision_free_set;
    size_t *popcounts;
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
    level->popcounts = NULL;
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
    if (level->popcounts != NULL) {
        free(level->popcounts);
    }
    if (level->next) {
        bbhash_level_free(level->next);
    }
    free(level);
}

constexpr size_t BLOCK_SIZE_IN_WORDS = 8;
constexpr size_t BLOCK_SIZE_IN_BITS = BLOCK_SIZE_IN_WORDS * 64; // 512 bits = 8*64

void bbhash_build_rank_checkpoints(BBHashLevel *level) {
    if (level == NULL || level->collision_free_set == NULL) return;

    Bitarray* ba = level->collision_free_set;
    size_t num_words = (ba->nbits + 63) / 64;
    size_t num_checkpoints = (num_words + BLOCK_SIZE_IN_WORDS - 1) / BLOCK_SIZE_IN_WORDS;

    level->popcounts = malloc(sizeof(size_t) * num_checkpoints);
    if (!level->popcounts) {
        /* handle error */ return;
    }

    size_t total_popcount = 0;
    size_t checkpoint_idx = 0;

    for (size_t i = 0; i < num_words; ++i) {
        if (i % BLOCK_SIZE_IN_WORDS == 0) {
            level->popcounts[checkpoint_idx++] = total_popcount;
        }
        total_popcount += stdc_count_ones(ba->bits[i]);
    }

    if (level->next) {
        bbhash_build_rank_checkpoints(level->next);
    }
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

    bbhash_build_rank_checkpoints(level0);

    return mphf;
}

/*  @brief
 *
 */
size_t bbhash_size_in_bits(const BBHash *mphf) {
    if (mphf == NULL) {
        return 0;
    }

    size_t total_bits = 0;
    struct BBHashLevel *current_level = mphf->levels;

    while (current_level != NULL) {
        if (current_level->collision_free_set) {
            size_t nbits = current_level->collision_free_set->nbits;

            // Stored in 64-bit words, so its allocated size is rounded up.
            size_t bitarray_storage_bits = ((nbits + 63) / 64) * 64;

            // Size of the popcounts checkpoint table.
            // One checkpoint (a size_t) for every `block_size_in_bits`.
            size_t num_checkpoints = (nbits + BLOCK_SIZE_IN_BITS - 1) / BLOCK_SIZE_IN_BITS;
            size_t popcounts_storage_bits = num_checkpoints * sizeof(size_t) * 8;
            total_bits += bitarray_storage_bits + popcounts_storage_bits;
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
            size_t rank = bitarray_rank(current_level->collision_free_set, current_level->popcounts, idx);
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

int bbhash_mphf_save(const BBHash *mphf, const char *filename) {
    if (!mphf || !filename) return -1;

    FILE *fp = fopen(filename, "wb");
    if (!fp) {
        perror("bbhash_mphf_save: fopen");
        return -1;
    }

    // --- 1. Count levels ---
    size_t num_levels = 0;
    for (BBHashLevel *level = mphf->levels; level != NULL; level = level->next) {
        num_levels++;
    }

    // --- 2. Write Header ---
    const char magic[4] = {'B', 'B', 'H', '1'};
    if (fwrite(magic, sizeof(char), 4, fp) != 4) goto write_error;

    uint64_t num_keys_u64 = mphf->num_keys;
    if (fwrite(&num_keys_u64, sizeof(uint64_t), 1, fp) != 1) goto write_error;

    uint64_t num_levels_u64 = num_levels;
    if (fwrite(&num_levels_u64, sizeof(uint64_t), 1, fp) != 1) goto write_error;

    // --- 3. Write Levels Data ---
    for (BBHashLevel *level = mphf->levels; level != NULL; level = level->next) {
        // Write metadata
        uint64_t seed_u64 = level->seed;
        uint64_t offset_u64 = level->level_offset;
        if (fwrite(&seed_u64, sizeof(uint64_t), 1, fp) != 1) goto write_error;
        if (fwrite(&offset_u64, sizeof(uint64_t), 1, fp) != 1) goto write_error;

        // Write bit array
        Bitarray *ba = level->collision_free_set;
        uint64_t nbits_u64 = ba->nbits;
        size_t n_words = (ba->nbits + 63) / 64;
        if (fwrite(&nbits_u64, sizeof(uint64_t), 1, fp) != 1) goto write_error;
        if (fwrite(ba->bits, sizeof(uint64_t), n_words, fp) != n_words) goto write_error;

        // Write popcounts table
        size_t num_checkpoints = (ba->nbits + BLOCK_SIZE_IN_BITS - 1) / BLOCK_SIZE_IN_BITS;
        uint64_t num_checkpoints_u64 = num_checkpoints;
        if (fwrite(&num_checkpoints_u64, sizeof(uint64_t), 1, fp) != 1) goto write_error;
        if (fwrite(level->popcounts, sizeof(size_t), num_checkpoints, fp) != num_checkpoints) goto write_error;
    }

    fclose(fp);
    return 0;

write_error:
    fprintf(stderr, "Error writing to MPHF file.\n");
    fclose(fp);
    return -1;
}


BBHash *bbhash_mphf_load(const char *filename) {
    if (!filename) return NULL;

    FILE *fp = fopen(filename, "rb");
    if (!fp) {
        perror("bbhash_mphf_load: fopen");
        return NULL;
    }

    // --- 1. Read and Validate Header ---
    char magic[4];
    if (fread(magic, sizeof(char), 4, fp) != 4) goto read_error;
    if (magic[0] != 'B' || magic[1] != 'B' || magic[2] != 'H' || magic[3] != '1') {
        fprintf(stderr, "Error: Invalid MPHF file format or version.\n");
        fclose(fp);
        return NULL;
    }

    uint64_t num_keys_u64, num_levels_u64;
    if (fread(&num_keys_u64, sizeof(uint64_t), 1, fp) != 1) goto read_error;
    if (fread(&num_levels_u64, sizeof(uint64_t), 1, fp) != 1) goto read_error;

    // --- 2. Allocate and Reconstruct MPHF ---
    BBHash *mphf = malloc(sizeof(BBHash));
    if (!mphf) goto alloc_error;
    mphf->num_keys = num_keys_u64;
    mphf->levels = NULL;

    BBHashLevel *current_level_tail = NULL;
    for (size_t i = 0; i < num_levels_u64; ++i) {
        BBHashLevel *level = bbhash_level_new(0); // Seed will be overwritten
        if (!level) {
            bbhash_free(mphf); // Clean up partially loaded structure
            goto alloc_error;
        }

        // Link the new level into the list
        if (current_level_tail == NULL) {
            mphf->levels = level;
        } else {
            current_level_tail->next = level;
        }
        current_level_tail = level;

        // Read metadata
        uint64_t seed_u64, offset_u64;
        if (fread(&seed_u64, sizeof(uint64_t), 1, fp) != 1) goto read_error_cleanup;
        if (fread(&offset_u64, sizeof(uint64_t), 1, fp) != 1) goto read_error_cleanup;
        level->seed = seed_u64;
        level->level_offset = offset_u64;

        // Read bit array
        uint64_t nbits_u64;
        if (fread(&nbits_u64, sizeof(uint64_t), 1, fp) != 1) goto read_error_cleanup;
        level->collision_free_set = bitarray_new(nbits_u64);
        if (!level->collision_free_set) goto read_error_cleanup;
        size_t n_words = (nbits_u64 + 63) / 64;
        if (fread(level->collision_free_set->bits, sizeof(uint64_t), n_words, fp) != n_words) goto read_error_cleanup;

        // Read popcounts table
        uint64_t num_checkpoints_u64;
        if (fread(&num_checkpoints_u64, sizeof(uint64_t), 1, fp) != 1) goto read_error_cleanup;
        level->popcounts = malloc(sizeof(size_t) * num_checkpoints_u64);
        if (!level->popcounts) goto read_error_cleanup;
        if (fread(level->popcounts, sizeof(size_t), num_checkpoints_u64, fp) != num_checkpoints_u64) goto read_error_cleanup;
    }

    fclose(fp);
    return mphf;

alloc_error:
    fprintf(stderr, "Memory allocation failed during MPHF load.\n");
    fclose(fp);
    return NULL;

read_error_cleanup:
    bbhash_free(mphf); // Free everything allocated so far
read_error:
    fprintf(stderr, "Error reading from MPHF file (file may be corrupt or truncated).\n");
    fclose(fp);
    return NULL;
}
