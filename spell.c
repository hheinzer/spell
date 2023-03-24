/*
 * Spelling corrector based on Peter Norvigs article
 * (https://norvig.com/spell-correct.html)
 */
#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

size_t strhash(const char *str)
{
    // djb2 hash function (http://www.cse.yorku.ca/~oz/hash.html)
    size_t hash = 5381;
    int c;
    while ((c = *str++)) hash = ((hash << 5) + hash) + c; // hash * 33 + c
    return hash;
}

// fixed size word counter (hash table)
#define MAX_KEY_LEN 30
#define COUNTER_SIZE 60000
typedef struct Counter {
    size_t len; // number of items
    size_t sum; // sum of item counts
    struct {
        const char key[MAX_KEY_LEN];
        size_t count;
    } item[COUNTER_SIZE];
} Counter;

void counter_insert(Counter *cntr, const char *key)
{
    assert(strlen(key) < MAX_KEY_LEN);
    assert(cntr->len < COUNTER_SIZE);
    // find empty spot or same key with linear probing; insert key; increase count & sum
    size_t i = strhash(key) % COUNTER_SIZE;
    while (*cntr->item[i].key && strcmp(cntr->item[i].key, key)) {
        i = (i + 1) % COUNTER_SIZE;
    }
    if (!*cntr->item[i].key) { // empty spot
        strncpy((char *)cntr->item[i].key, key, MAX_KEY_LEN);
        cntr->item[i].count = 1;
        ++cntr->len;
    } else { // same key
        ++cntr->item[i].count;
    }
    ++cntr->sum;
}

Counter *counter_alloc(const char *fname)
{
    // create counter and insert all words (only a-z,A-Z) from file 'fname'
    Counter *cntr = calloc(1, sizeof(*cntr));
    assert(cntr && "Could not allocate memory.");
    FILE *file = fopen(fname, "r");
    assert(file && "Could not open file.");
    char word[256] = { 0 };
    size_t i = 0;
    int c = 0;
    while ((c = fgetc(file)) != EOF) {
        if (isalpha(c)) {
            assert(i < sizeof(word));
            word[i++] = tolower(c); // make all words lowercase
        } else if (i > 0) {
            word[i] = 0;
            counter_insert(cntr, word);
            i = 0;
        }
    }
    fclose(file);
    return cntr;
}

int cmp_size_t_dsc(const void *a_, const void *b_)
{
    // compare two variables of type size_t
    const size_t a = *(size_t *)a_;
    const size_t b = *(size_t *)b_;
    return (a < b) - (a > b);
}

void counter_most_common(const Counter *cntr, size_t n)
{
    // create array from counter items; sort w.r.t count; print most common 'n'
    struct {
        size_t count;
        const char *key;
    } *map = calloc(cntr->len, sizeof(*map));
    assert(map && "Could not allocate memory.");
    for (size_t i = 0, j = 0; i < COUNTER_SIZE; ++i) {
        if (*cntr->item[i].key) {
            map[j].count = cntr->item[i].count;
            map[j].key = cntr->item[i].key;
            ++j;
        }
    }
    qsort(map, cntr->len, sizeof(*map), cmp_size_t_dsc);
    for (size_t i = 0; i < n; ++i) {
        printf("'%s': %zu\n", map[i].key, map[i].count);
    }
    free(map);
}

size_t counter_get_count(const Counter *cntr, const char *key)
{
    // find same key with linear probing, if not present return 0
    size_t i = strhash(key) % COUNTER_SIZE;
    while (*cntr->item[i].key && strcmp(cntr->item[i].key, key)) {
        i = (i + 1) % COUNTER_SIZE;
    }
    return (*cntr->item[i].key ? cntr->item[i].count : 0);
}

double counter_get_ratio(const Counter *cntr, const char *key)
{
    return counter_get_count(cntr, key) / (double)cntr->sum;
}

int main(int argc, char **argv)
{
    Counter *cntr = counter_alloc("big.txt");

    printf("%zu\n", cntr->len);
    printf("%zu\n", cntr->sum);
    counter_most_common(cntr, 20);
    printf("%zu\n", counter_get_count(cntr, "the"));
    printf("%g\n", counter_get_ratio(cntr, "the"));
    printf("%g\n", counter_get_ratio(cntr, "outrivaled"));
    printf("%g\n", counter_get_ratio(cntr, "unmentioned"));

    free(cntr);
}
