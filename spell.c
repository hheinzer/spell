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
    WordSet *ws = wordset_alloc(54 * n + 25);
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
    WordSet *ws = wordset_alloc((54 * n + 25) * (54 * n + 25));
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

    // return the unknown word
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
        char *word = correction(wc, test.pair[i].wrong);
        if (!word) {
            ++n_unkn;
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
        spelltest(wc, "spell-testset2.txt");
    }
    wordcounter_free(wc);
}

#if 0

double word_probability(const Counter *count, const char *word)
{
    return counter_get_count(count, word) / (double)count->len;
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

#endif
