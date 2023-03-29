/*
 * word counter, implemented as hash map using linear probing
 */
#ifndef WORD_COUNTER_H
#define WORD_COUNTER_H

#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "memory.h"
#include "strhash.h"

typedef struct WordCounter {
    size_t len;
    size_t size;
    struct {
        char *word;
        size_t count;
    } *item;
} WordCounter;

WordCounter *wordcounter_alloc(size_t size);

void wordcounter_free(WordCounter *wc);

void wordcounter_insert(WordCounter *wc, const char *word);

WordCounter *wordcounter_from_file(const char *fname, size_t size);

size_t wordcounter_get_count(const WordCounter *wc, const char *word);

#endif
