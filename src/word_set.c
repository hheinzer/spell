#include "word_set.h"

WordSet *wordset_alloc(size_t size)
{
    WordSet *ws = malloc(sizeof(*ws));
    ws->len = 0;
    ws->size = size;
    ws->word = calloc(ws->size, sizeof(*ws->word));
    return ws;
}

void wordset_free(WordSet *ws)
{
    for (size_t i = 0; i < ws->size; ++i) {
        free(ws->word[i]);
    }
    free(ws->word);
    free(ws);
}

static size_t get_index(WordSet *ws, const char *word)
{
    size_t i = strhash(word) % ws->size;
    while (ws->word[i] && strcmp(ws->word[i], word)) {
        i = (i + 1) % ws->size; // linear probing to find empty spot or same word
    }
    return i;
}

void wordset_insert(WordSet *ws, const char *word)
{
    assert(ws->len < ws->size);
    const size_t i = get_index(ws, word);
    if (!ws->word[i]) { // word does not yet exist
        ws->word[i] = strdup(word);
        ++ws->len;
    }
}
