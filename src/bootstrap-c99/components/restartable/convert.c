#include "wchar.h"
#include <stdio.h>
#include <errno.h>

/* Existing C locale: byte values 0..255 map to identical wide values. */
wint_t btowc(int c) {
    return c == EOF ? WEOF : (unsigned char)c;
}

int wctob(wint_t c) {
    return c <= 255 ? (int)c : EOF;
}

int mbsinit(const mbstate_t *s) {
    return !s || !s->__state;
}

size_t mbrtowc(wchar_t *restrict out, const char *restrict text, size_t n,
               mbstate_t *restrict state) {
    static mbstate_t internal;
    unsigned char c;
    if (!state)
        state = &internal;
    if (!text) {
        state->__state = 0;
        return 0;
    }
    if (!n)
        return (size_t)-2;
    c = (unsigned char)*text;
    if (out)
        *out = c;
    state->__state = 0;
    return c ? 1 : 0;
}

size_t mbrlen(const char *restrict text, size_t n, mbstate_t *restrict state) {
    static mbstate_t internal;
    return mbrtowc(NULL, text, n, state ? state : &internal);
}

size_t wcrtomb(char *restrict out, wchar_t c, mbstate_t *restrict state) {
    static mbstate_t internal;
    if (!state)
        state = &internal;
    if (!out) {
        state->__state = 0;
        return 1;
    }
    if (c > 255) {
        errno = EILSEQ;
        return (size_t)-1;
    }
    *out = (char)c;
    state->__state = 0;
    return 1;
}

size_t mbsrtowcs(wchar_t *restrict out, const char **restrict source, size_t n,
                 mbstate_t *restrict state) {
    static mbstate_t internal;
    const char *p = *source;
    size_t i = 0;
    unsigned char c;
    if (!state)
        state = &internal;
    while (!out || i < n) {
        c = (unsigned char)*p;
        if (out)
            out[i] = c;
        state->__state = 0;
        if (!c) {
            if (out)
                *source = NULL;
            return i;
        }
        p++;
        i++;
    }
    if (out)
        *source = p;
    return i;
}

size_t wcsrtombs(char *restrict out, const wchar_t **restrict source, size_t n,
                 mbstate_t *restrict state) {
    static mbstate_t internal;
    const wchar_t *p = *source;
    size_t i = 0;
    wchar_t c;
    if (!state)
        state = &internal;
    while (!out || i < n) {
        c = *p;
        if (c > 255) {
            if (out)
                *source = p;
            errno = EILSEQ;
            return (size_t)-1;
        }
        if (out)
            out[i] = (char)c;
        state->__state = 0;
        if (!c) {
            if (out)
                *source = NULL;
            return i;
        }
        p++;
        i++;
    }
    if (out)
        *source = p;
    return i;
}
