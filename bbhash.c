#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "bitarray.h"
#include "bbhash.h"

constexpr size_t MIN_BITARRAY_SIZE = 64;

// MurmurHash3 128-bit for strings, return 64 bits
uint64_t murmur3_string(const char *key, uint64_t seed) {
    size_t len = strlen(key);
    const uint8_t *data = (const uint8_t *)key;
    const int nblocks = len / 16;

    uint64_t h1 = seed;
    uint64_t h2 = seed;

    const uint64_t c1 = 0x87c37b91114253d5;
    const uint64_t c2 = 0x4cf5ad432745937f;

    // Process 16-byte blocks
    const uint64_t *blocks = (const uint64_t *)data;
    for(int i = 0; i < nblocks; i++) {
        uint64_t k1 = blocks[i * 2 + 0];
        uint64_t k2 = blocks[i * 2 + 1];

        k1 *= c1;
        k1 = (k1 << 31) | (k1 >> 33);
        k1 *= c2;
        h1 ^= k1;
        h1 = (h1 << 27) | (h1 >> 37);
        h1 += h2;
        h1 = h1 * 5 + 0x52dce729;

        k2 *= c2;
        k2 = (k2 << 33) | (k2 >> 31);
        k2 *= c1;
        h2 ^= k2;
        h2 = (h2 << 31) | (h2 >> 33);
        h2 += h1;
        h2 = h2 * 5 + 0x38495ab5;
    }

    // Process remaining bytes (tail)
    const uint8_t *tail = data + nblocks * 16;
    uint64_t k1 = 0, k2 = 0;

    switch(len & 15) {
    case 15:
        k2 ^= ((uint64_t)tail[14]) << 48;
        [[fallthrough]];
    case 14:
        k2 ^= ((uint64_t)tail[13]) << 40;
        [[fallthrough]];
    case 13:
        k2 ^= ((uint64_t)tail[12]) << 32;
        [[fallthrough]];
    case 12:
        k2 ^= ((uint64_t)tail[11]) << 24;
        [[fallthrough]];
    case 11:
        k2 ^= ((uint64_t)tail[10]) << 16;
        [[fallthrough]];
    case 10:
        k2 ^= ((uint64_t)tail[ 9]) << 8;
        [[fallthrough]];
    case  9:
        k2 ^= ((uint64_t)tail[ 8]) << 0;
        k2 *= c2;
        k2 = (k2 << 33) | (k2 >> 31);
        k2 *= c1;
        h2 ^= k2;
        [[fallthrough]];
    case  8:
        k1 ^= ((uint64_t)tail[ 7]) << 56;
        [[fallthrough]];
    case  7:
        k1 ^= ((uint64_t)tail[ 6]) << 48;
        [[fallthrough]];
    case  6:
        k1 ^= ((uint64_t)tail[ 5]) << 40;
        [[fallthrough]];
    case  5:
        k1 ^= ((uint64_t)tail[ 4]) << 32;
        [[fallthrough]];
    case  4:
        k1 ^= ((uint64_t)tail[ 3]) << 24;
        [[fallthrough]];
    case  3:
        k1 ^= ((uint64_t)tail[ 2]) << 16;
        [[fallthrough]];
    case  2:
        k1 ^= ((uint64_t)tail[ 1]) << 8;
        [[fallthrough]];
    case  1:
        k1 ^= ((uint64_t)tail[ 0]) << 0;
        k1 *= c1;
        k1 = (k1 << 31) | (k1 >> 33);
        k1 *= c2;
        h1 ^= k1;
    }

    // Finalization
    h1 ^= len;
    h2 ^= len;
    h1 += h2;
    h2 += h1;

    // fmix64
    h1 ^= h1 >> 33;
    h1 *= 0xff51afd7ed558ccd;
    h1 ^= h1 >> 33;
    h1 *= 0xc4ceb9fe1a85ec53;
    h1 ^= h1 >> 33;

    return h1; // Just return first 64 bits
}

// simple string-to-uint64_6 for strings
uint64_t fnv1a_string(const char *key, uint64_t seed) {
    uint64_t hash = 0xcbf29ce484222325ULL ^ seed; // FNV offset basis
    const uint64_t prime = 0x100000001b3ULL;       // FNV prime

    while (*key) {
        hash ^= (uint64_t)(unsigned char)(*key++);
        hash *= prime;
    }

    return hash;
}

// fmix64 function from MurmurHash3 by Austin Appleby
uint64_t hash_with_seed(uint64_t key, uint64_t seed) {
    key ^= seed;
    key ^= key >> 33;
    key *= 0xff51afd7ed558ccd;
    key ^= key >> 33;
    key *= 0xc4ceb9fe1a85ec53;
    key ^= key >> 33;
    return key;
}

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

size_t calc_level_size(size_t unplaced) {
    double gamma = 1.0;
    size_t n = (size_t) unplaced * gamma;
    return n < MIN_BITARRAY_SIZE ? MIN_BITARRAY_SIZE : n;
}

BBHashLevel *bbhash_mphf_create(const uint64_t data[], size_t nelem) {
    BBHashLevel *level0 = NULL;
    BBHashLevel *current_level = NULL;
    Bitarray* used_slots = bitarray_new(nelem);
    Bitarray* has_been_placed = bitarray_new(nelem);
    size_t placed = 0;  // number of keys perfectly mapped
    size_t unplaced = nelem - placed;
    uint64_t current_seed = 41;

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

        size_t level_size = calc_level_size(unplaced);
        bitarray_shrink(used_slots, level_size);
        bitarray_clear_all(used_slots);
        Bitarray* colliding_slots = bitarray_new(level_size); // collisions

        for (size_t i = 0; i < nelem; i++) {
            if (bitarray_get(has_been_placed, i) == 1) continue;

            uint64_t hash = hash_with_seed(data[i], current_level->seed);
            size_t idx = hash % level_size;
            if (bitarray_get(used_slots, idx) == 1) {
                bitarray_set(colliding_slots, idx);
            } else {
                bitarray_set(used_slots, idx);
            }
        }

        bitarray_andnot(colliding_slots, used_slots, colliding_slots);
        current_level->collision_free_set = colliding_slots;

        // update has_been_placed
        size_t rank = 0;
        for (size_t i = 0; i < nelem; i++) {
            if (bitarray_get(has_been_placed, i) == 1) {
                continue; // Already placed, skip.
            }

            uint64_t hash = hash_with_seed(data[i], current_seed);
            size_t idx = hash % level_size;

            if (bitarray_get(current_level->collision_free_set, idx) == 1) {
                bitarray_set(has_been_placed, i);
                rank++;
            }
        }

        printf("Collision-free rank (%zu): %zu; offset %zu\n", current_level->seed, rank, current_level->level_offset);
        placed += rank;
        unplaced = nelem - placed;
    }

    bitarray_free(used_slots);
    bitarray_free(has_been_placed);
    return level0;
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
size_t bbhash_mphf_query(BBHashLevel *level0, uint64_t key) {
    BBHashLevel *current_level = level0;

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
