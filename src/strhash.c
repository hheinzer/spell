#include "strhash.h"

size_t strhash(const char *str)
{
    // djb2 hash function (http://www.cse.yorku.ca/~oz/hash.html)
    size_t hash = 5381;
    int c;
    while ((c = *str++)) hash = ((hash << 5) + hash) + c; // hash * 33 + c
    return hash;
}
