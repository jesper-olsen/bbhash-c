#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include "bitarray.h"

Bitarray *bitarray_new(size_t nbits) {
    size_t nwords = (nbits + 63) / 64;

    Bitarray *ba = malloc(sizeof(Bitarray) + nwords * sizeof(uint64_t));

    if (ba == NULL) {
        fprintf(stderr, "Memory allocation failed for Bitarray!\n");
        return NULL;
    }

    ba->nbits = nbits;
    memset(ba->bits, 0, nwords * sizeof(uint64_t));

    return ba;
}

void bitarray_shrink(Bitarray* ba, size_t nbits) {
    assert(ba != NULL && nbits <= ba->nbits);
    ba->nbits = nbits;
}

void bitarray_free(Bitarray *ba) {
    free(ba);
}

void bitarray_set(Bitarray *ba, size_t pos) {
    assert(ba != NULL && pos < ba->nbits);

    size_t word = pos / 64;      // Which uint64_t? (or: pos >> 6)
    size_t bit_in_word = pos % 64; // Which bit within it? (or: pos & 63)
    ba->bits[word] |= (1ULL << bit_in_word);
}

void bitarray_clear_all(Bitarray *ba) {
    size_t nwords = (ba->nbits + 63) / 64;
    memset(ba->bits, 0, nwords * sizeof(uint64_t));
}

int bitarray_get(const Bitarray *ba, size_t pos) {
    assert(ba != NULL && pos < ba->nbits);
    size_t word = pos >> 6;
    size_t bit_in_word = pos & 63;
    // Check if the bit is set by masking and shifting.
    return (ba->bits[word] >> bit_in_word) & 1;
}

void bitarray_clear(Bitarray *ba, size_t pos) {
    assert(ba != NULL && pos < ba->nbits);
    size_t word = pos >> 6;
    size_t bit = pos & 63;
    ba->bits[word] &= ~(1ULL << bit);
}

bool bitarray_is_zero(const Bitarray *ba) {
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

// rank - 0-based
size_t bitarray_rank(const Bitarray *ba, size_t pos) {
    assert(ba != NULL && pos < ba->nbits);

    size_t word_idx = pos >> 6; // pos / 64
    size_t bit_idx = pos & 63;  // pos % 64
    size_t rank = 0;

    // TODO - this can be done faster by pre-calculating a table with the sum...
    for (size_t i = 0; i < word_idx; i++) {
        rank += stdc_count_ones(ba->bits[i]);
    }

    // Count bits in the final word (word_idx) up to bit_idx (exclusive)
    if (bit_idx > 0) {
        uint64_t last_word = ba->bits[word_idx];
        // Create a mask for the lower 'bit_idx' bits (i.e., bits 0 to bit_idx-1)
        uint64_t mask = (1ULL << bit_idx) - 1; 
        rank += stdc_count_ones(last_word & mask);
    }

    return rank;
}

void bitarray_andnot(Bitarray *dest, const Bitarray *src1, const Bitarray *src2) {
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
