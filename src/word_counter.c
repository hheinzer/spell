#include "word_counter.h"

WordCounter *wordcounter_alloc(size_t size)
{
    WordCounter *wc = malloc(sizeof(*wc));
    wc->len = 0;
    wc->size = size;
    wc->item = calloc(wc->size, sizeof(*wc->item));
    return wc;
}

void wordcounter_free(WordCounter *wc)
{
    for (size_t i = 0; i < wc->size; ++i) {
        free(wc->item[i].word);
    }
    free(wc->item);
    free(wc);
}

static size_t get_index(const WordCounter *wc, const char *word)
{
    size_t i = strhash(word) % wc->size;
    while (wc->item[i].word && strcmp(wc->item[i].word, word)) {
        i = (i + 1) % wc->size; // linear probing to find empty spot or same word
    }
    return i;
}

void wordcounter_insert(WordCounter *wc, const char *word)
{
    assert(wc->len < wc->size);
    const size_t i = get_index(wc, word);
    if (!wc->item[i].word) { // word does not yet exist
        wc->item[i].word = strdup(word);
        wc->item[i].count = 1;
        ++wc->len;
    } else { // word already exists
        ++wc->item[i].count;
    }
}

WordCounter *wordcounter_from_file(const char *fname, size_t size)
{
    FILE *file = fopen(fname, "r");
    assert(file);
    WordCounter *wc = wordcounter_alloc(size);
    char word[256] = "";
    size_t word_len = 0;
    int c = 0;
    while ((c = fgetc(file)) != EOF) {
        if (isalpha(c)) {
            assert(word_len < sizeof(word) - 1);
            word[word_len++] = tolower(c); // make all word lowercase
        } else if (word_len > 0) {
            word[word_len] = 0;
            wordcounter_insert(wc, word);
            word_len = 0;
        }
    }
    fclose(file);
    return wc;
}

size_t wordcounter_get_count(const WordCounter *wc, const char *word)
{
    const size_t i = get_index(wc, word);
    return (wc->item[i].word ? wc->item[i].count : 0);
}
