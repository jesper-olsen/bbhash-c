#ifndef BITARRAY_H
#define BITARRAY_H

#include <stddef.h> // For size_t
#include <stdint.h> // For uint64_t
#include <stdbool.h> // For bool type
#include <stdlib.h>
#include <stdio.h>   // fprintf
#include <assert.h>


#if defined(__has_include) && __has_include(<stdbit.h>)
#include <stdbit.h>
#elif defined(__GNUC__)
static inline unsigned int stdc_count_ones(uint64_t x) {
    return (unsigned int)__builtin_popcountll(x);
}
#else
// Fast manual implementation 64-bit native
static inline unsigned int stdc_count_ones(uint64_t x) {
    x = x - ((x >> 1) & 0x5555555555555555ULL);
    x = (x & 0x3333333333333333ULL) + ((x >> 2) & 0x3333333333333333ULL);
    x = (x + (x >> 4)) & 0x0F0F0F0F0F0F0F0FULL;
    return (unsigned int)((x * 0x0101010101010101ULL) >> 56);
}
#endif

typedef struct {
    size_t nbits;
    uint64_t bits[];  // Flexible array member (C99) - must be the last element.
} Bitarray;

/**
 * Creates a new, zero-initialized bit array.
 * @param nbits The number of bits the array should hold.
 * @return A pointer to the new Bitarray, or NULL on allocation failure.
 */
static inline Bitarray *bitarray_new(size_t nbits) {
    size_t nwords = (nbits + 63) / 64;
    size_t total_size = sizeof(Bitarray) + nwords * sizeof(uint64_t);
    Bitarray *ba = calloc(1, total_size);
    if (ba == NULL) {
        fprintf(stderr, "Memory allocation failed for Bitarray!\n");
        return NULL;
    }
    ba->nbits = nbits;
    return ba;
}

/**
 * Shrinks the array.
 * @param nbits The reduced number of bits the array should hold.
 */
static inline void bitarray_shrink(Bitarray* ba, size_t nbits) {
    assert(ba != NULL && nbits <= ba->nbits);
    ba->nbits = nbits;
}

/**
 * Frees the memory allocated for the Bitarray.
 * @param ba A pointer to the Bitarray to be freed.
 */
static inline void bitarray_free(Bitarray *ba) {
    free(ba);
}

/**
 * Sets a bit at a specific position to 1.
 * @param ba A pointer to the Bitarray.
 * @param pos The zero-based index of the bit to set.
 */
static inline void bitarray_set(Bitarray *ba, size_t pos) {
    assert(ba != NULL && pos < ba->nbits);

    size_t word = pos / 64;      // Which uint64_t? (or: pos >> 6)
    size_t bit_in_word = pos % 64; // Which bit within it? (or: pos & 63)
    ba->bits[word] |= (1ULL << bit_in_word);
}

/**
 * Gets the value of a bit at a specific position.
 * @param ba A pointer to the Bitarray.
 * @param pos The zero-based index of the bit to get.
 * @return 1 if the bit is set, 0 if it is not. Returns 0 for out-of-bounds access.
 */
static inline int bitarray_get(const Bitarray *ba, size_t pos) {
    assert(ba != NULL && pos < ba->nbits);
    size_t word = pos >> 6;
    size_t bit_in_word = pos & 63;
    // Check if the bit is set by masking and shifting.
    return (ba->bits[word] >> bit_in_word) & 1;
}

/**
 * Clears a bit at a specific position to 0.
 * @param ba A pointer to the Bitarray.
 * @param pos The zero-based index of the bit to clear.
 */
static inline void bitarray_clear(Bitarray *ba, size_t pos) {
    assert(ba != NULL && pos < ba->nbits);
    size_t word = pos >> 6;
    size_t bit = pos & 63;
    ba->bits[word] &= ~(1ULL << bit);
}

/**
 * Clears all bits in the bitarray.
 * @param ba A pointer to the Bitarray.
 */
static inline void bitarray_clear_all(Bitarray *ba) {
    size_t nwords = (ba->nbits + 63) / 64;
    memset(ba->bits, 0, nwords * sizeof(uint64_t));
}

/**
 * Checks if no bits are set. Ignores unused bits in the final word.
 * @param ba A pointer to the Bitarray.
 * @return true if all bits are zero, false otherwise.
 */
static inline bool bitarray_is_zero(const Bitarray *ba) {
    assert(ba != NULL);

    if (ba->nbits == 0) {
        return true;
    }

    size_t nwords = (ba->nbits + 63) / 64;

    // Check all the words that are fully used.
    for (size_t i = 0; i < nwords - 1; i++) {
        if (ba->bits[i] != 0) {
            return false;
        }
    }

    // check the last word, which might be partially used.
    size_t bits_in_last_word = ba->nbits % 64;
    if (bits_in_last_word == 0) {
        return ba->bits[nwords - 1] == 0;
    }

    // mask for the used bits in the last word.
    uint64_t mask = (1ULL << bits_in_last_word) - 1;
    return (ba->bits[nwords - 1] & mask) == 0;
}

/**
 * @brief Calculates the rank of a bit position (number of set bits BEFORE this position).
 * This is an "exclusive" rank.
 * @param ba The constant bit array.
 * @param popcounts Precomputed cumulative popcounts array.
 * @param pos The bit position (0-indexed).
 * @return The number of set bits in bits[0...pos-1].
 */
static inline size_t bitarray_rank(const Bitarray *ba, const size_t *popcounts, size_t pos) {
    assert(ba != NULL && pos < ba->nbits);

    const size_t block_size_in_bits = 512;
    const size_t word_idx = pos >> 6;      // pos / 64
    const size_t bit_idx = pos & 63;       // pos % 64

    // pre-computed rank from the checkpoint table
    const size_t checkpoint_idx = pos / block_size_in_bits; // or word_idx / 8
    size_t rank = popcounts[checkpoint_idx];

    // Scan the words between the checkpoint and the current word
    const size_t start_word_in_block = checkpoint_idx * (block_size_in_bits / 64);
    for (size_t i = start_word_in_block; i < word_idx; ++i) {
        rank += stdc_count_ones(ba->bits[i]);
    }

    // Count bits in the final word (exclusive of pos)
    if (bit_idx > 0) {
        uint64_t last_word = ba->bits[word_idx];
        uint64_t mask = (1ULL << bit_idx) - 1; // Mask for bits [0...bit_idx-1]
        rank += stdc_count_ones(last_word & mask);
    }

    return rank;
}

/**
 * Performs a bitwise AND NOT operation. dest = src1 & ~src2.
 * Asserts that all three bit arrays are the same size.
 * It is safe for 'dest' to be the same as 'src1' or 'src2'.
 */
static inline void bitarray_andnot(Bitarray *dest, const Bitarray *src1, const Bitarray *src2) {
    assert(dest != NULL && src1 != NULL && src2 != NULL);
    assert(dest->nbits == src1->nbits && dest->nbits == src2->nbits);

    size_t nwords = (dest->nbits + 63) / 64;

    for (size_t i = 0; i < nwords; i++) {
        dest->bits[i] = src1->bits[i] & (~src2->bits[i]);
    }

    // Zero the unused bits in the last destination word
    size_t bits_in_last_word = dest->nbits % 64;
    if (bits_in_last_word > 0) {
        uint64_t mask = (1ULL << bits_in_last_word) - 1;
        dest->bits[nwords - 1] &= mask;
    }
}


#endif
