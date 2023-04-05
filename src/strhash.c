#include "strhash.h"

__attribute__((unused)) static size_t djb2(const char *str)
{
    // source: http://www.cse.yorku.ca/~oz/hash.html
    size_t hash = 5381;
    unsigned char c;
    while ((c = *str++)) {
        hash = ((hash << 5) + hash) + c; // hash * 33 + c
    }
    return hash;
}

__attribute__((unused)) static size_t fnv(const char *str)
{
    // source: https://en.wikipedia.org/wiki/Fowler%E2%80%93Noll%E2%80%93Vo_hash_function
    size_t hash = 14695981039346656037ULL;
    unsigned char c;
    while ((c = *str++)) {
        hash ^= c;
        hash *= 1099511628211ULL;
    }
    return hash;
}

size_t strhash(const char *str)
{
    return fnv(str);
}
