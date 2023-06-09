/*
 * - malloc, calloc, and realloc that assert memory allocation did not fail
 * - general purpose memory duplication function
 */
#ifndef MEMORY_H
#define MEMORY_H

#include <assert.h>
#include <stdlib.h>
#include <string.h>

inline void *malloc_assert(size_t size)
{
    void *ptr = malloc(size);
    assert(ptr);
    return ptr;
}
#define malloc malloc_assert

inline void *calloc_assert(size_t nmemb, size_t size)
{
    void *ptr = calloc(nmemb, size);
    assert(ptr);
    return ptr;
}
#define calloc calloc_assert

inline void *realloc_assert(void *ptr, size_t size)
{
    ptr = realloc(ptr, size);
    assert(ptr);
    return ptr;
}
#define realloc realloc_assert

void *memdup(const void *src, size_t size);

char *strdup(const char *src);

#endif
