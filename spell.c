/*
 * Spelling corrector based on Peter Norvigs article
 * (https://norvig.com/spell-correct.html)
 */
#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define MAX_WORD_LEN 30
#define WORD_FMT "%29s"

size_t strhash(const char *str)
{
    // djb2 hash function (http://www.cse.yorku.ca/~oz/hash.html)
    size_t hash = 5381;
    int c;
    while ((c = *str++)) hash = ((hash << 5) + hash) + c; // hash * 33 + c
    return hash;
}

// fixed size word counter (hash table)
#define COUNTER_SIZE 60000
typedef struct Counter {
    size_t len; // number of items
    struct {
        const char word[MAX_WORD_LEN];
        size_t count;
    } item[COUNTER_SIZE];
} Counter;

void counter_insert(Counter *counter, const char *word)
{
    assert(counter->len < COUNTER_SIZE);
    // find empty spot or same word with linear probing; insert word; increase count
    size_t i = strhash(word) % COUNTER_SIZE;
    while (*counter->item[i].word && strcmp(counter->item[i].word, word)) {
        i = (i + 1) % COUNTER_SIZE;
    }
    if (!*counter->item[i].word) { // empty spot
        strncpy((char *)counter->item[i].word, word, MAX_WORD_LEN);
        counter->item[i].count = 1;
        ++counter->len;
    } else { // same word
        ++counter->item[i].count;
    }
}

Counter *counter_alloc(const char *fname)
{
    // create counter and insert all words (only a-z,A-Z) from file 'fname'
    Counter *counter = malloc(sizeof(*counter));
    assert(counter && "Could not allocate memory.");
    counter->len = 0;
    FILE *file = fopen(fname, "r");
    assert(file && "Could not open file.");
    char word[MAX_WORD_LEN] = "";
    size_t i = 0;
    int c = 0;
    while ((c = fgetc(file)) != EOF) {
        if (isalpha(c)) {
            assert(i < MAX_WORD_LEN);
            word[i++] = tolower(c); // make all words lowercase
        } else if (i > 0) {
            word[i] = 0;
            counter_insert(counter, word);
            i = 0;
        }
    }
    fclose(file);
    return counter;
}

size_t counter_get_count(const Counter *counter, const char *word)
{
    // find same word with linear probing, if not present return 0
    size_t i = strhash(word) % COUNTER_SIZE;
    while (*counter->item[i].word && strcmp(counter->item[i].word, word)) {
        i = (i + 1) % COUNTER_SIZE;
    }
    return (*counter->item[i].word ? counter->item[i].count : 0);
}

double word_probability(const Counter *count, const char *word)
{
    return counter_get_count(count, word) / (double)count->len;
}

// dynamically sized set of fixed size words (hash table)
typedef struct Set {
    size_t len; // number of words
    size_t size; // maximum number of set
    const char (*word)[MAX_WORD_LEN];
} Set;

Set *set_alloc(size_t min_size)
{
    Set *set = malloc(sizeof(*set));
    assert(set && "Could not allocate memory.");
    set->len = 0;
    set->size = 2 * min_size;
    set->word = calloc(set->size, sizeof(*set->word));
    assert(set->word && "Could not allocate memory.");
    return set;
}

void set_free(Set *set)
{
    free((char *)set->word);
    free(set);
}

void set_insert(Set *set, const char *word)
{
    assert(set->len < set->size);
    // find empty spot or same word with linear probing; insert word
    size_t i = strhash(word) % set->size;
    while (*set->word[i] && strcmp(set->word[i], word)) {
        i = (i + 1) % set->size;
    }
    if (!*set->word[i]) { // empty spot
        strncpy((char *)set->word[i], word, MAX_WORD_LEN);
        ++set->len;
    }
}

Set *word_edit_once(const char *word)
{
    assert(word && strlen(word));
    // create all edits that are one edit away from 'word'; filter by known words
    const size_t n = strlen(word);
    Set *edit = set_alloc(54 * n + 25);
    char new_word[MAX_WORD_LEN] = "";
    for (size_t i = 0; i < n + 1; ++i) {
        // splits
        const size_t nl = i;
        const size_t nr = n - i;
        const char *left = &word[0];
        const char *right = &word[i];

        // deletes
        if (nr > 0) {
            snprintf(new_word, MAX_WORD_LEN, "%.*s%.*s",
                (int)nl, left, (int)nr - 1, &right[1]);
            set_insert(edit, new_word);
        }

        // transposes
        if (nr > 1) {
            snprintf(new_word, MAX_WORD_LEN, "%.*s%c%c%.*s",
                (int)nl, left, right[1], right[0], (int)nr - 2, &right[2]);
            set_insert(edit, new_word);
        }

        // replaces
        if (nr > 0) {
            for (char letter = 'a'; letter <= 'z'; ++letter) {
                snprintf(new_word, MAX_WORD_LEN, "%.*s%c%.*s",
                    (int)nl, left, letter, (int)nr - 1, &right[1]);
                set_insert(edit, new_word);
            }
        }

        // inserts
        for (char letter = 'a'; letter <= 'z'; ++letter) {
            snprintf(new_word, MAX_WORD_LEN, "%.*s%c%.*s",
                (int)nl, left, letter, (int)nr, right);
            set_insert(edit, new_word);
        }
    }
    return edit;
}

Set *word_edit_twice(const char *word)
{
    const size_t n = strlen(word);
    Set *edit = set_alloc((54 * n + 25) * (54 * n + 25));
    Set *edit1 = word_edit_once(word);
    for (size_t i = 0; i < edit1->size; ++i) {
        if (!*edit1->word[i]) continue;
        Set *edit2 = word_edit_once(edit1->word[i]);
        for (size_t j = 0; j < edit2->size; ++j) {
            if (!*edit2->word[j]) continue;
            set_insert(edit, edit2->word[j]);
        }
        set_free(edit2);
    }
    set_free(edit1);
    return edit;
}

// dynamically sized array of fixed size words
typedef struct Vector {
    size_t len; // number of words
    size_t size; // maximum number of words
    char (*word)[MAX_WORD_LEN];
} Vector;

Vector *vector_alloc(size_t size)
{
    Vector *vec = malloc(sizeof(*vec));
    assert(vec && "Could not alloc memory.");
    vec->len = 0;
    vec->size = size;
    vec->word = calloc(size, sizeof(*vec->word));
    assert(vec->word && "Could not alloc memory.");
    return vec;
}

void vector_free(Vector *vec)
{
    free(vec->word);
    free(vec);
}

void vector_insert(Vector *vec, const char *word)
{
    if (vec->len >= vec->size) {
        vec->size *= 2;
        vec->word = realloc(vec->word, vec->size * sizeof(*vec->word));
        assert(vec->word && "Could not realloc memory.");
    }
    strncpy(vec->word[vec->len++], word, MAX_WORD_LEN);
}

Vector *word_known(const Counter *counter, const Set *edit)
{
    Vector *known = vector_alloc(100);
    for (size_t i = 0; i < edit->size; ++i) {
        if (!*edit->word[i]) continue;
        if (counter_get_count(counter, edit->word[i])) {
            vector_insert(known, edit->word[i]);
        }
    }
    return known;
}

int cmp_double_dsc(const void *a_, const void *b_)
{
    const double a = *(double *)a_;
    const double b = *(double *)b_;
    return (a < b) - (a > b);
}

char *word_max_probability(const Vector *vec, const Counter *counter)
{
    // pair words with their probability; sort; return word with highest probability
    struct {
        double probability;
        char *word;
    } *pair = calloc(vec->len, sizeof(*pair));
    assert(pair && "Could not allocate memory.");
    for (size_t i = 0; i < vec->len; ++i) {
        pair[i].probability = word_probability(counter, vec->word[i]);
        pair[i].word = vec->word[i];
    }
    qsort(pair, vec->len, sizeof(*pair), cmp_double_dsc);
    char *max = pair[0].word;
    free(pair);
    return max;
}

void word_correction(const Counter *counter, char *correction, const char *word)
{
    // if known, no correction needed
    if (counter_get_count(counter, word)) {
        strncpy(correction, word, MAX_WORD_LEN);
        return;
    }

    // if edit once exists, return most probable
    Set *edit1 = word_edit_once(word);
    Vector *known1 = word_known(counter, edit1);
    if (known1->len) {
        strncpy(correction, word_max_probability(known1, counter), MAX_WORD_LEN);
        goto cleanup_known1;
    }

    // if edit twice exists, return most probable
    Set *edit2 = word_edit_twice(word);
    Vector *known2 = word_known(counter, edit2);
    if (known2->len) {
        strncpy(correction, word_max_probability(known2, counter), MAX_WORD_LEN);
        goto cleanup_known2;
    }

    // return unknown word
    strncpy(correction, word, MAX_WORD_LEN);

cleanup_known2:
    set_free(edit2);
    vector_free(known2);

cleanup_known1:
    set_free(edit1);
    vector_free(known1);
}

void spelltest(const Counter *counter, const char *fname)
{
    // allocate test structure
    const size_t n_max = 500;
    size_t n = 0;
    struct {
        char correct[MAX_WORD_LEN];
        Vector *incorrect;
    } *test = calloc(n_max, sizeof(*test));

    // parse testset
    FILE *file = fopen(fname, "r");
    assert(file && "Could not open file.");
    char line[256] = "";
    while (fgets(line, sizeof(line), file)) {
        assert(n < n_max);
        char *tok = strtok(line, " :\n");
        sscanf(tok, WORD_FMT, test[n].correct);
        test[n].incorrect = vector_alloc(10);
        while ((tok = strtok(0, " :\n"))) {
            vector_insert(test[n].incorrect, tok);
        }
        ++n;
    }
    fclose(file);

    // perform test
    size_t n_test = 0;
    size_t n_good = 0;
    size_t n_unknown = 0;
    char correction[MAX_WORD_LEN] = "";
    const double t0 = clock();
    for (size_t i = 0; i < n; ++i) {
        for (size_t j = 0; j < test[i].incorrect->len; ++j) {
            word_correction(counter, correction, test[i].incorrect->word[j]);
            ++n_test;
            if (!strncmp(test[i].correct, correction, MAX_WORD_LEN)) {
                ++n_good;
            } else {
                if (!counter_get_count(counter, test[i].correct)) {
                    ++n_unknown;
                }
            }
        }
    }
    const double dt = clock() - t0;

    // print result
    printf("%.0f%% of %zu correct (%.0f%% unknown) at %.0f words per second\n",
        100 * n_good / (double)n_test, n_test,
        100 * n_unknown / (double)n_test, n_test / dt * CLOCKS_PER_SEC);

    // cleanup
    for (size_t i = 0; i < n; ++i) {
        vector_free(test[i].incorrect);
    }
    free(test);
}

int main(int argc, char **argv)
{
    // create word counter
    Counter *counter = counter_alloc("big.txt");

    if (argc == 2) { // command line mode
        // get input word
        char word[MAX_WORD_LEN] = "";
        sscanf(argv[1], WORD_FMT, word);

        // get correction
        char correction[MAX_WORD_LEN] = "";
        word_correction(counter, correction, word);
        printf("%s\n", correction);

    } else { // spell test mode
        spelltest(counter, "spell-testset1.txt");
        spelltest(counter, "spell-testset2.txt");
    }

    // cleanup
    free(counter);
}
