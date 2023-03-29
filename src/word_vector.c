#include "word_vector.h"

WordVector *wordvector_alloc(size_t size)
{
    WordVector *wv = malloc(sizeof(*wv));
    wv->len = 0;
    wv->size = size;
    wv->word = calloc(wv->size, sizeof(*wv->word));
    return wv;
}

void wordvector_free(WordVector *wv)
{
    for (size_t i = 0; i < wv->len; ++i) {
        free(wv->word[i]);
    }
    free(wv->word);
    free(wv);
}

void wordvector_insert(WordVector *wv, const char *word)
{
    if (wv->len >= wv->size) {
        wv->size *= 2;
        wv->word = realloc(wv->word, wv->size * sizeof(*wv->word));
    }
    wv->word[wv->len++] = strdup(word);
}
