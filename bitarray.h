#ifndef BITARRAY_H
#define BITARRAY_H

#include <stddef.h> // For size_t
#include <stdint.h> // For uint64_t
#include <stdbool.h> // For bool type

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
Bitarray *bitarray_new(size_t nbits);

/**
 * Shrinks the array.
 * @param nbits The reduced number of bits the array should hold.
 */
void bitarray_shrink(Bitarray *ba, size_t nbits);


/**
 * Frees the memory allocated for the Bitarray.
 * @param ba A pointer to the Bitarray to be freed.
 */
void bitarray_free(Bitarray *ba);

/**
 * Sets a bit at a specific position to 1.
 * @param ba A pointer to the Bitarray.
 * @param pos The zero-based index of the bit to set.
 */
void bitarray_set(Bitarray *ba, size_t pos);

/**
 * Gets the value of a bit at a specific position.
 * @param ba A pointer to the Bitarray.
 * @param pos The zero-based index of the bit to get.
 * @return 1 if the bit is set, 0 if it is not. Returns 0 for out-of-bounds access.
 */
int bitarray_get(const Bitarray *ba, size_t pos);

/**
 * Clears a bit at a specific position to 0.
 * @param ba A pointer to the Bitarray.
 * @param pos The zero-based index of the bit to clear.
 */
void bitarray_clear(Bitarray *ba, size_t pos);

/**
 * Clears all bits in the bitarray.
 * @param ba A pointer to the Bitarray.
 */
void bitarray_clear_all(Bitarray *ba);

/**
 * Checks if no bits are set. Ignores unused bits in the final word.
 * @param ba A pointer to the Bitarray.
 * @return true if all bits are zero, false otherwise.
 */
bool bitarray_is_zero(const Bitarray *ba);


/**
 * @brief Calculates the rank of a bit position (number of set bits up to and including that position).
 * @param bits The constant bit array.
 * @param pos The bit position (0-indexed).
 * @return The number of set bits in bits[0...pos].
 */
size_t bitarray_rank(const Bitarray *ba, size_t pos);


/**
 * Performs a bitwise AND NOT operation. dest = src1 & ~src2.
 * Asserts that all three bit arrays are the same size.
 * It is safe for 'dest' to be the same as 'src1' or 'src2'.
 */
void bitarray_andnot(Bitarray *dest, const Bitarray *src1, const Bitarray *src2);

#endif
