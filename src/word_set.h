/*
 * word set, implemented as hash map using linear probing
 */
#ifndef WORD_SET_H
#define WORD_SET_H

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "memory.h"
#include "strhash.h"

typedef struct WordSet {
    size_t len;
    size_t size;
    char **word;
} WordSet;

WordSet *wordset_alloc(size_t size);

void wordset_free(WordSet *ws);

void wordset_insert(WordSet *ws, const char *word);

#endif
