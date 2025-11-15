#ifndef BBHASH_H
#define BBHASH_H

typedef struct BBHash BBHash;

BBHash *bbhash_mphf_create(const uint64_t data[], size_t unplaced, double gamma, bool verbose);
size_t bbhash_size_in_bits(const BBHash *mphf);
size_t bbhash_mphf_query(const BBHash *level, uint64_t key);
void bbhash_free(BBHash *mphf);

/**
 * @brief Saves a constructed BBHash MPHF to a file.
 * @param mphf The MPHF to save.
 * @param filename The path to the output file.
 * @return 0 on success, -1 on failure.
 */
int bbhash_mphf_save(const BBHash *mphf, const char *filename);

/**
 * @brief Loads a BBHash MPHF from a file.
 * @param filename The path to the file to load.
 * @return A pointer to the loaded BBHash structure, or NULL on failure.
 */
BBHash *bbhash_mphf_load(const char *filename);

#endif
