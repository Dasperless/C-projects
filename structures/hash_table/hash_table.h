#ifndef HASH_TABLE_H
#define HASH_TABLE_H

#include <stdbool.h>
#include <stddef.h>

/* String-keyed hash table (separate chaining, DJB2) that doubles its bucket
 * array and rehashes when the load factor reaches HT_MAX_LOAD (0.75). */
#define HT_MAX_LOAD 0.75f
#define HT_DEFAULT_BUCKETS 8u

typedef struct HashTable HashTable;

/* initial_buckets == 0 selects HT_DEFAULT_BUCKETS. */
HashTable *ht_create(size_t initial_buckets);
void ht_destroy(HashTable *ht);

/* Inserts or updates. Keys are copied; the caller keeps ownership of `key`. */
bool ht_insert(HashTable *ht, const char *key, int value);
bool ht_get(const HashTable *ht, const char *key, int *out_value);
bool ht_remove(HashTable *ht, const char *key);
bool ht_contains(const HashTable *ht, const char *key);

size_t ht_size(const HashTable *ht);
size_t ht_bucket_count(const HashTable *ht);
float ht_load_factor(const HashTable *ht);

/* DJB2 hash, exposed so tests can construct colliding keys. */
unsigned long ht_hash(const char *key);

#endif /* HASH_TABLE_H */
