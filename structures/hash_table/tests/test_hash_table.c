#undef NDEBUG
#include "hash_table.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void test_basic_and_null(void) {
    HashTable *ht = ht_create(0);
    int v = 0;
    assert(ht != NULL && ht_bucket_count(ht) == HT_DEFAULT_BUCKETS);
    assert(!ht_get(ht, "a", &v) && !ht_remove(ht, "a") && !ht_contains(ht, "a"));
    assert(ht_insert(ht, "alice", 1));
    assert(ht_insert(ht, "bob", 2));
    assert(ht_get(ht, "alice", &v) && v == 1);
    assert(ht_contains(ht, "bob"));
    assert(ht_insert(ht, "alice", 10)); /* update, not duplicate */
    assert(ht_size(ht) == 2);
    assert(ht_get(ht, "alice", &v) && v == 10);
    assert(ht_remove(ht, "alice") && !ht_remove(ht, "alice"));
    assert(ht_size(ht) == 1);
    ht_destroy(ht);
    ht_destroy(NULL);
    assert(!ht_insert(NULL, "a", 1) && !ht_insert(NULL, NULL, 1));
    assert(ht_size(NULL) == 0 && ht_load_factor(NULL) == 0.0f);
}

static void test_djb2_known_values(void) {
    assert(ht_hash("") == 5381UL);
    assert(ht_hash("a") == 5381UL * 33 + 'a');
}

/* Insert keys that all land in the same bucket and verify chain integrity. */
static void test_collisions(void) {
    HashTable *ht = ht_create(64);
    char keys[8][16];
    size_t n = 0;
    unsigned long target = ht_hash("key0") % 64;
    for (int i = 0; n < 8 && i < 100000; i++) {
        char k[16];
        snprintf(k, sizeof(k), "key%d", i);
        if (ht_hash(k) % 64 == target) {
            strcpy(keys[n++], k);
        }
    }
    assert(n == 8);
    for (size_t i = 0; i < n; i++) {
        assert(ht_insert(ht, keys[i], (int)i));
    }
    assert(ht_bucket_count(ht) == 64); /* 8/64 load: no rehash, all in one chain */
    for (size_t i = 0; i < n; i++) {
        int v = -1;
        assert(ht_get(ht, keys[i], &v) && v == (int)i);
    }
    /* remove middle of chain, others survive */
    assert(ht_remove(ht, keys[3]));
    assert(!ht_contains(ht, keys[3]));
    for (size_t i = 0; i < n; i++) {
        if (i != 3) {
            assert(ht_contains(ht, keys[i]));
        }
    }
    ht_destroy(ht);
}

static void test_rehash_integrity(void) {
    HashTable *ht = ht_create(2);
    size_t prev_buckets = ht_bucket_count(ht);
    int resizes = 0;
    char key[32];
    for (int i = 0; i < 1000; i++) {
        snprintf(key, sizeof(key), "user-%d", i);
        assert(ht_insert(ht, key, i));
        assert(ht_load_factor(ht) < HT_MAX_LOAD);
        if (ht_bucket_count(ht) != prev_buckets) {
            assert(ht_bucket_count(ht) == prev_buckets * 2);
            prev_buckets = ht_bucket_count(ht);
            resizes++;
        }
    }
    assert(resizes >= 8);
    assert(ht_size(ht) == 1000);
    for (int i = 0; i < 1000; i++) {
        int v = -1;
        snprintf(key, sizeof(key), "user-%d", i);
        assert(ht_get(ht, key, &v) && v == i);
    }
    for (int i = 0; i < 1000; i += 2) {
        snprintf(key, sizeof(key), "user-%d", i);
        assert(ht_remove(ht, key));
    }
    assert(ht_size(ht) == 500);
    ht_destroy(ht);
}

/* Keys are copied: mutating/freeing the caller's buffer must not affect the table. */
static void test_key_ownership(void) {
    HashTable *ht = ht_create(4);
    char *k = (char *)malloc(8);
    strcpy(k, "temp");
    assert(ht_insert(ht, k, 7));
    memset(k, 'x', 4);
    free(k);
    int v = 0;
    assert(ht_get(ht, "temp", &v) && v == 7);
    ht_destroy(ht); /* ASan verifies no leaks / double frees */
}

int main(void) {
    test_basic_and_null();
    test_djb2_known_values();
    test_collisions();
    test_rehash_integrity();
    test_key_ownership();
    printf("All hash table tests passed.\n");
    return 0;
}
