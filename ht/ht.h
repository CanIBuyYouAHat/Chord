#ifndef HT_H
#define HT_H

#include <stdint.h>

// Temporary max table size
#define TABLE_SIZE 100

typedef struct Hash_node {
  char *key;
  int32_t value;
  struct Hash_node *next;
} Hash_node;

typedef struct Hashtable {
  Hash_node **table;
} Hashtable;

uint32_t hash(const char *key);
Hashtable *create_table();
void insert(Hashtable *ht, const char *key, int32_t value);
int32_t lookup(Hashtable *ht, const char *key);
int delete(Hashtable *ht, const char *key);
void free_table(Hashtable *ht);

#endif
