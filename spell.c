/*
 * Spelling corrector based on Peter Norvigs article
 * (https://norvig.com/spell-correct.html)
 */
#include <stdio.h>
#include <time.h>

#include "word_counter.h"
#include "word_set.h"
#include "word_vector.h"

int is_known(const WordCounter *wc, const char *word)
{
    return wordcounter_get_count(wc, word);
}

WordSet *edit_once(const char *word)
{
    const size_t n = strlen(word);
    WordSet *ws = wordset_alloc(2 * (54 * n + 25));
    char *new_word = calloc(2 * n, sizeof(*new_word));
    for (size_t i = 0; i < n + 1; ++i) {
        // splits
        const size_t nl = i;
        const size_t nr = n - i;
        const char *left = &word[0];
        const char *right = &word[i];

        // deletes
        if (nr > 0) {
            memcpy(new_word, left, nl);
            memcpy(&new_word[nl], &right[1], nr - 1);
            new_word[n - 1] = 0;
            wordset_insert(ws, new_word);
        }

        // transposes
        if (nr > 1) {
            memcpy(new_word, left, nl);
            new_word[nl] = right[1];
            new_word[nl + 1] = right[0];
            memcpy(&new_word[nl + 2], &right[2], nr - 2);
            new_word[n] = 0;
            wordset_insert(ws, new_word);
        }

        // replaces
        if (nr > 0) {
            for (char letter = 'a'; letter <= 'z'; ++letter) {
                memcpy(new_word, left, nl);
                new_word[nl] = letter;
                memcpy(&new_word[nl + 1], &right[1], nr - 1);
                new_word[n] = 0;
                wordset_insert(ws, new_word);
            }
        }

        // inserts
        for (char letter = 'a'; letter <= 'z'; ++letter) {
            memcpy(new_word, left, nl);
            new_word[nl] = letter;
            memcpy(&new_word[nl + 1], right, nr);
            new_word[n + 1] = 0;
            wordset_insert(ws, new_word);
        }
    }
    free(new_word);
    return ws;
}

WordVector *known_words(const WordCounter *wc, WordSet *ws)
{
    WordVector *wv = wordvector_alloc(ws->len / 10);
    for (size_t i = 0; i < ws->size; ++i) {
        if (!ws->word[i]) continue;
        if (is_known(wc, ws->word[i])) wordvector_insert(wv, ws->word[i]);
    }
    return wv;
}

int cmp_size_t_dsc(const void *a_, const void *b_)
{
    const size_t a = *(size_t *)a_;
    const size_t b = *(size_t *)b_;
    return (a < b) - (a > b);
}

const char *max_probability(const WordCounter *wc, const WordVector *wv)
{
    struct {
        size_t count;
        const char *word;
    } *tuple = calloc(wv->len, sizeof(*tuple));
    for (size_t i = 0; i < wv->len; ++i) {
        tuple[i].count = wordcounter_get_count(wc, wv->word[i]);
        tuple[i].word = wv->word[i];
    }
    qsort(tuple, wv->len, sizeof(*tuple), cmp_size_t_dsc);
    const char *ret = tuple[0].word;
    free(tuple);
    return ret;
}

WordSet *edit_twice(const char *word)
{
    const size_t n = strlen(word);
    WordSet *ws = wordset_alloc(2 * (54 * n + 25) * (54 * n + 25));
    WordSet *ws1 = edit_once(word);
    for (size_t i = 0; i < ws1->size; ++i) {
        if (!ws1->word[i]) continue;
        WordSet *ws2 = edit_once(ws1->word[i]);
        for (size_t j = 0; j < ws2->size; ++j) {
            if (!ws2->word[j]) continue;
            wordset_insert(ws, ws2->word[j]);
        }
        wordset_free(ws2);
    }
    wordset_free(ws1);
    return ws;
}

char *correction(const WordCounter *wc, const char *word)
{
    // if word is known, return it
    if (is_known(wc, word)) return strdup(word);

    char *ret = 0;

    // if there are known words with edit distance 1, return the most probable
    WordSet *ws1 = edit_once(word);
    WordVector *wv1 = known_words(wc, ws1);
    if (wv1->len) {
        ret = strdup(max_probability(wc, wv1));
        goto cleanup1;
    }

    // if there are known words with edit distance 2, return the most probable
    WordSet *ws2 = edit_twice(word);
    WordVector *wv2 = known_words(wc, ws2);
    if (wv2->len) {
        ret = strdup(max_probability(wc, wv2));
        goto cleanup2;
    }

    // return unknown word (as 0)
    ret = 0;

cleanup2:
    wordset_free(ws2);
    wordvector_free(wv2);

cleanup1:
    wordset_free(ws1);
    wordvector_free(wv1);

    return ret;
}

void spelltest(const WordCounter *wc, const char *fname)
{
    // create test structure
    struct {
        size_t len;
        size_t size;
        struct {
            char *right;
            char *wrong;
        } *pair;
    } test;
    test.len = 0;
    test.size = 500;
    test.pair = calloc(test.size, sizeof(*test.pair));

    // read test file
    FILE *file = fopen(fname, "r");
    assert(file);
    char line[256] = "";
    while (fgets(line, sizeof(line), file)) {
        if (test.len >= test.size) {
            test.size *= 2;
            test.pair = realloc(test.pair, test.size * sizeof(*test.pair));
        }
        char *tok = strtok(line, " :\n");
        char *right = tok;
        while ((tok = strtok(0, " :\n"))) {
            test.pair[test.len].right = strdup(right);
            test.pair[test.len].wrong = strdup(tok);
            ++test.len;
        }
    }
    fclose(file);

    // perform test
    size_t n_good = 0;
    size_t n_unkn = 0;
    const double t0 = clock();
    for (size_t i = 0; i < test.len; ++i) {
        if (!is_known(wc, test.pair[i].right)) ++n_unkn;
        char *word = correction(wc, test.pair[i].wrong);
        if (!word) {
            continue;
        } else if (!strcmp(word, test.pair[i].right)) {
            ++n_good;
        }
        free(word);
    }
    const double dt = clock() - t0;

    // print result
    printf("%.0f%% of %zu correct (%.0f%% unknown) at %.0f words per second\n",
        100 * n_good / (double)test.len, test.len,
        100 * n_unkn / (double)test.len, test.len / dt * CLOCKS_PER_SEC);

    // cleanup
    for (size_t i = 0; i < test.len; ++i) {
        free(test.pair[i].right);
        free(test.pair[i].wrong);
    }
    free(test.pair);
}

int main(int argc, char **argv)
{
    WordCounter *wc = wordcounter_from_file("big.txt", 100000);
    if (argc > 1) { // correct individual words from command line
        for (int i = 0; i < argc - 1; ++i) {
            char *word = correction(wc, argv[1 + i]);
            if (!word) {
                printf("Word is unknown: %s\n", argv[1 + i]);
                continue;
            } else if (!strcmp(word, argv[1 + i])) {
                printf("Word is correct: %s\n", word);
            } else {
                printf("Did you mean: %s\n", word);
            }
            free(word);
        }
    } else { // perform spell test
        spelltest(wc, "spell-testset1.txt");
        // spelltest(wc, "spell-testset2.txt");
    }
    wordcounter_free(wc);
}
