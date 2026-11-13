#include "hash_table.h"

#include <stdlib.h>
#include <string.h>

typedef struct Entry {
    char *key;
    int value;
    struct Entry *next;
} Entry;

struct HashTable {
    Entry **buckets;
    size_t bucket_count;
    size_t size;
};

unsigned long ht_hash(const char *key) {
    unsigned long h = 5381UL;
    for (const unsigned char *p = (const unsigned char *)key; *p != '\0'; p++) {
        h = ((h << 5) + h) + *p; /* h * 33 + c */
    }
    return h;
}

HashTable *ht_create(size_t initial_buckets) {
    HashTable *ht = (HashTable *)malloc(sizeof(HashTable));
    if (ht == NULL) {
        return NULL;
    }
    ht->bucket_count = initial_buckets == 0 ? HT_DEFAULT_BUCKETS : initial_buckets;
    ht->size = 0;
    ht->buckets = (Entry **)calloc(ht->bucket_count, sizeof(Entry *));
    if (ht->buckets == NULL) {
        free(ht);
        return NULL;
    }
    return ht;
}

void ht_destroy(HashTable *ht) {
    if (ht == NULL) {
        return;
    }
    for (size_t i = 0; i < ht->bucket_count; i++) {
        Entry *e = ht->buckets[i];
        while (e != NULL) {
            Entry *next = e->next;
            free(e->key);
            free(e);
            e = next;
        }
    }
    free(ht->buckets);
    free(ht);
}

static Entry *find(const HashTable *ht, const char *key) {
    for (Entry *e = ht->buckets[ht_hash(key) % ht->bucket_count]; e != NULL; e = e->next) {
        if (strcmp(e->key, key) == 0) {
            return e;
        }
    }
    return NULL;
}

/* Doubles the bucket array and relinks every entry (no reallocation of entries). */
static bool rehash(HashTable *ht) {
    size_t new_count = ht->bucket_count * 2;
    Entry **nb = (Entry **)calloc(new_count, sizeof(Entry *));
    if (nb == NULL) {
        return false;
    }
    for (size_t i = 0; i < ht->bucket_count; i++) {
        Entry *e = ht->buckets[i];
        while (e != NULL) {
            Entry *next = e->next;
            size_t idx = ht_hash(e->key) % new_count;
            e->next = nb[idx];
            nb[idx] = e;
            e = next;
        }
    }
    free(ht->buckets);
    ht->buckets = nb;
    ht->bucket_count = new_count;
    return true;
}

bool ht_insert(HashTable *ht, const char *key, int value) {
    if (ht == NULL || key == NULL) {
        return false;
    }
    Entry *existing = find(ht, key);
    if (existing != NULL) {
        existing->value = value;
        return true;
    }
    Entry *e = (Entry *)malloc(sizeof(Entry));
    if (e == NULL) {
        return false;
    }
    size_t len = strlen(key) + 1;
    e->key = (char *)malloc(len);
    if (e->key == NULL) {
        free(e);
        return false;
    }
    memcpy(e->key, key, len);
    e->value = value;
    size_t idx = ht_hash(key) % ht->bucket_count;
    e->next = ht->buckets[idx];
    ht->buckets[idx] = e;
    ht->size++;
    if (ht_load_factor(ht) >= HT_MAX_LOAD) {
        (void)rehash(ht); /* on failure the table stays valid, just denser */
    }
    return true;
}

bool ht_get(const HashTable *ht, const char *key, int *out_value) {
    if (ht == NULL || key == NULL) {
        return false;
    }
    const Entry *e = find(ht, key);
    if (e == NULL) {
        return false;
    }
    if (out_value != NULL) {
        *out_value = e->value;
    }
    return true;
}

bool ht_contains(const HashTable *ht, const char *key) {
    return ht_get(ht, key, NULL);
}

bool ht_remove(HashTable *ht, const char *key) {
    if (ht == NULL || key == NULL) {
        return false;
    }
    Entry **link = &ht->buckets[ht_hash(key) % ht->bucket_count];
    while (*link != NULL) {
        if (strcmp((*link)->key, key) == 0) {
            Entry *victim = *link;
            *link = victim->next;
            free(victim->key);
            free(victim);
            ht->size--;
            return true;
        }
        link = &(*link)->next;
    }
    return false;
}

size_t ht_size(const HashTable *ht) {
    return ht == NULL ? 0 : ht->size;
}

size_t ht_bucket_count(const HashTable *ht) {
    return ht == NULL ? 0 : ht->bucket_count;
}

float ht_load_factor(const HashTable *ht) {
    if (ht == NULL || ht->bucket_count == 0) {
        return 0.0f;
    }
    return (float)ht->size / (float)ht->bucket_count;
}
