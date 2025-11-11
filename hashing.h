#ifndef HASHING_H
#define HASHING_H

#include <stdint.h>

// simple string-to-uint64_6 for strings
uint64_t fnv1a_string(const char *key, uint64_t seed);

// MurmurHash3 128-bit for strings, return 64 bits
uint64_t murmur3_string(const char *key, uint64_t seed);

/**
 * @brief fmix64 function from MurmurHash3 by Austin Appleby.
 * 
 * Defined as static inline in the header for performance, allowing the compiler
 * to inline this critical function into hot loops.
 */
static inline uint64_t hash_with_seed(uint64_t key, uint64_t seed) {
    key ^= seed;
    key ^= key >> 33;
    key *= 0xff51afd7ed558ccd;
    key ^= key >> 33;
    key *= 0xc4ceb9fe1a85ec53;
    key ^= key >> 33;
    return key;
}

#endif // HASHING_H
