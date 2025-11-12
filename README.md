# BBHash - Minimal Perfect Hash Function inC23

 fast, space-efficient C23 implementation of the BBHash algorithm for constructing minimal perfect hash functions (MPHFs).

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

This produces the `example` executable.

## Usage

### Basic Usage

```sh
./example <num_elements> [options]
```

### Options

* `<num_elements> - Number of keys to build the MPHF for (required).
* `-g, --gamma <float>` - Set gamma parameter (default: 2.0).
* `-v, --validate` - Verify the MPHF is correct after construction.
* `-h, --help` - Show help message.

## Examples

Build MPHF for 10 million keys with default setting:
```sh
./example 10000000
```

Use gamma=1.0 for maximum space efficiency:
```sh
./example 10000000 -g 1.0
```

## Sample Output

```
% ./example 10000000
--- BBHash C23 Demo ---
Parameters: nelem = 10000000, gamma = 2.00, validate = no
-----------------------

Generating and de-duplicating initial key set...
Found 0 duplicated elements.

Constructing MPHF...
Level 0; placed 6065602; offset 0
Level 1; placed 2388544; offset 6065602
Level 2; placed 937095; offset 8454146
Level 3; placed 370300; offset 9391241
Level 4; placed 144923; offset 9761541
Level 5; placed 56641; offset 9906464
Level 6; placed 22242; offset 9963105
Level 7; placed 8999; offset 9985347
Level 8; placed 3458; offset 9994346
Level 9; placed 1322; offset 9997804
Level 10; placed 496; offset 9999126
Level 11; placed 217; offset 9999622
Level 12; placed 86; offset 9999839
Level 13; placed 39; offset 9999925
Level 14; placed 20; offset 9999964
Level 15; placed 12; offset 9999984
Level 16; placed 4; offset 9999996
BBHash constructed perfect hash for 10000000 keys in 0.15 seconds (CPU time).
BBHash total size: 32963984 bits (3.93 MB)
BBHash bits/elem : 3.2964
```

Generated on a Macbook Air M1.

## References

* Original paper: ["Fast and scalable minimal perfect hashing for massive key sets" (Limasset et al., 2017)](https://github.com/rizkg/BBHash#:~:text=http%3A//drops.dagstuhl.de/opus/volltexte/2017/7619/pdf/LIPIcs%2DSEA%2D2017%2D25.pdf)
* Original C++ [implementation](https://github.com/rizkg/BBHash)


