#include "ht.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

uint32_t hash(const char *key) {
  uint32_t hash = 5381;
  int c;
  while ((c = *key++)) {
         hash = ((hash << 5) + hash) + c;
  }
  return hash % TABLE_SIZE;
}

Hashtable *create_table() {
  Hashtable *ht = (Hashtable *)malloc(sizeof(Hashtable));
  ht->table = (Hash_node **)malloc(sizeof(Hash_node *) * TABLE_SIZE);
  for (int i = 0; i < TABLE_SIZE; i++) {
       ht->table[i] = NULL;
  }
  
  return ht;
}

void insert(Hashtable *ht, const char *key, int32_t value) {
  uint32_t index = hash(key);
  Hash_node *new_node = (Hash_node *)malloc(sizeof(Hash_node));
  new_node->key = strdup(key);
  new_node->value = value;
  new_node->next = ht->table[index];
  ht->table[index] = new_node;
}

int32_t lookup(Hashtable *ht, const char *key) {
  uint32_t index = hash(key);
  Hash_node *node = ht->table[index];
  while (node) {
    if (strcmp(node->key, key) == 0) {
      return node->value;
    }
    node = node->next;
  }
  return -1;
}

int delete(Hashtable *ht, const char *key) {
  uint32_t index = hash(key);
  Hash_node *node = ht->table[index];
  Hash_node *prev = NULL;
  while (node) {
   if (strcmp(node->key, key) == 0) {
      if (prev) {
        prev->next = node->next;
      } else {
        ht->table[index] = node->next;
      }
      free(node->key);
      free(node);
      return 1;
    }
    prev = node;
    node = node->next;
  }

  // Hash_node not found
  return 0;
}

void free_table(Hashtable *ht) {
  for (int i = 0; i < TABLE_SIZE; i++) {
       Hash_node *node = ht->table[i];
       while (node) {
        Hash_node *temp = node;
        node = node->next;

        free(temp->key);
        free(temp);
       }
  }
  free(ht->table);
  free(ht);
}

     

