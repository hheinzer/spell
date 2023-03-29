/*
 * word vector, implemented as a dynamically sized array
 */
#ifndef WORD_VECTOR_H
#define WORD_VECTOR_H

#include <stdlib.h>
#include <string.h>

#include "memory.h"

typedef struct WordVector {
    size_t len;
    size_t size;
    char **word;
} WordVector;

WordVector *wordvector_alloc(size_t size);

void wordvector_free(WordVector *wv);

void wordvector_insert(WordVector *wv, const char *word);

#endif
