#include <assert.h>
#include "bitarray.h"

int main() {
    // Create a bit array with num_bits bits
    size_t num_bits = 100;
    Bitarray *my_bits = bitarray_new(num_bits);
    if (my_bits == NULL) {
        return 1; // Exit if creation failed
    }

    // Set some bits
    bitarray_set(my_bits, 0);
    bitarray_set(my_bits, 1);
    bitarray_set(my_bits, 63);
    bitarray_set(my_bits, 64);
    bitarray_set(my_bits, 99);

    // Check 
    assert(bitarray_get(my_bits,0)==1);
    assert(bitarray_get(my_bits,1)==1);
    assert(bitarray_get(my_bits,2)==0);
    assert(bitarray_get(my_bits,63)==1);
    assert(bitarray_get(my_bits,64)==1);
    assert(bitarray_get(my_bits,99)==1);

    // Clear a bit and check again
    bitarray_clear(my_bits, 1);
    assert(bitarray_get(my_bits,1)==0);

    // Clean up
    bitarray_free(my_bits);

    return 0;
}
