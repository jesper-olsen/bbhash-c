# BBHash - Minimal Perfect Hash Function in C23

Fast, space-efficient C23 implementation of the BBHash algorithm for constructing minimal perfect hash functions (MPHFs).

## What is BBHash?
BBHash is an algorithm that creates a minimal perfect hash function - a hash function that maps a set of N keys
to exactly N consecutive integers (0 to N-1) with no collisions and no gaps. This makes it ideal for creating
compact, fast lookup structures.

## Key Features

* Minimal & Perfect: Maps N keys to [0,N-1] with no collisions.
* Fast Construction: Builds MPHFs for millions of keys in seconds.
* Space Efficient: Typically 2-4 bits per key.
* Tunable: Trade off construction speed vs. space with gamma parameter.

## Building

```sh
make
```

This produces the `example` `example_strings` executables.

## Examples

This repository includes two executables to demonstrate the library's functionality.

### Example 1: Large-Scale Integer MPHF (`./example`)

This example demonstrates the raw performance and scalability of BBHash by building an MPHF for
a large set of randomly generated 64-bit integers.

**Usage**

```sh
./example <num_elements> [options]
```

### Options

* `<num_elements>` - Number of keys to build the MPHF for (required).
* `-g, --gamma <float>` - Set gamma parameter (default: 2.0).
* `-v, --validate` - Verify the MPHF is correct after construction.
* `-h, --help` - Show help message.

### Sample Run (100 Million Keys)

Use gamma=1.0 for maximum space efficiency:
```sh
./example 100000000 --gamma 1.0 --validate
```

```
--- BBHash C23 Demo ---
Parameters: nelem = 100000000, gamma = 1.00, validate = yes
-----------------------

Generating and de-duplicating initial key set...
Found 0 duplicated elements.

Constructing MPHF...
Level 0; placed 36787707; offset 0
Level 1; placed 23256085; offset 36787707
Level 2; placed 14704375; offset 60043792
Level 3; placed 9291914; offset 74748167
Level 4; placed 5872295; offset 84040081
Level 5; placed 3711838; offset 89912376
Level 6; placed 2344934; offset 93624214
Level 7; placed 1483296; offset 95969148
Level 8; placed 937436; offset 97452444
Level 9; placed 591966; offset 98389880
Level 10; placed 374119; offset 98981846
Level 11; placed 236738; offset 99355965
Level 12; placed 149228; offset 99592703
Level 13; placed 94883; offset 99741931
Level 14; placed 59939; offset 99836814
Level 15; placed 37950; offset 99896753
Level 16; placed 23810; offset 99934703
Level 17; placed 15296; offset 99958513
Level 18; placed 9686; offset 99973809
Level 19; placed 6090; offset 99983495
Level 20; placed 3754; offset 99989585
Level 21; placed 2482; offset 99993339
Level 22; placed 1520; offset 99995821
Level 23; placed 934; offset 99997341
Level 24; placed 644; offset 99998275
Level 25; placed 400; offset 99998919
Level 26; placed 250; offset 99999319
Level 27; placed 168; offset 99999569
Level 28; placed 96; offset 99999737
Level 29; placed 52; offset 99999833
Level 30; placed 44; offset 99999885
Level 31; placed 28; offset 99999929
Level 32; placed 25; offset 99999957
Level 33; placed 15; offset 99999982
Level 34; placed 3; offset 99999997
BBHash constructed perfect hash for 100000000 keys in 5.55 seconds (CPU time).
BBHash total size: 305782208 bits (36.45 MB)
BBHash bits/elem : 3.0578

Validating MPHF...
Validation successful: all keys map to a unique index in [0, 100000000).
```

## Example 2: String Vocabulary Lookup (./example_strings)

This executable demonstrates a practical, real-world use case: building a fast lookup table for a
fixed set of strings. It uses the command vocabulary from the classic text adventure "Colossal Cave Adventure".

This demonstrates the crucial pattern for non-integer keys:

1. Hash each string to a unique `uint64_t`.
2. Build the MPHF on the set of hashes.
3. Use the MPHF to map a string to a unique index `[0,N-1]`.
4. Use the index to lookup up data in an auxiliary array, with a final check to prevent collisions from OOV words.


**Usage**

```sh
./example_strings
```

**Sample Output**

```
--- BBHash C23 Demo ---
Parameters: nelem = 282, gamma = 1.00
-----------------------

Hashing vocabulary of 282 words and checking for hash collisions...
No hash collisions found.

Constructing MPHF...
Level 0; placed 107; offset 0
Level 1; placed 76; offset 107
Level 2; placed 34; offset 183
Level 3; placed 25; offset 217
Level 4; placed 17; offset 242
Level 5; placed 14; offset 259
Level 6; placed 9; offset 273
BBHash constructed perfect hash for 282 keys in 0.00 seconds.
BBHash total size: 1408 bits (4.9929 bits/elem)

Validating MPHF...
Validation successful.

Building value table and testing lookup...
  Lookup for 'xyzzy': SUCCESS, found at index 181
  Lookup for 'XYZZY': FAILED, key not in set. (MPHF returned index 89 which holds 'gully')
  Lookup for 'troll': SUCCESS, found at index 219
  Lookup for 'dragons': FAILED, key not in set. (MPHF returned index 150 which holds 'axe')
```


## References

* Original paper: ["Fast and scalable minimal perfect hashing for massive key sets" (Limasset et al., 2017)](http://drops.dagstuhl.de/opus/volltexte/2017/7619/pdf/LIPIcs-SEA-2017-25.pdf)
* Original C++ [implementation](https://github.com/rizkg/BBHash)


## License

This project is licensed under the MIT License. See the [LICENSE](LICENSE) file for details.
