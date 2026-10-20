#include <stdlib.h>
#include <errno.h>

/* Stateless C locale: each unsigned byte maps to the same wchar_t value. */
int mbtowc(wchar_t *dest, const char *text, size_t count) {
    unsigned char value;
    if (!text)
        return 0;
    if (!count) {
        errno = EILSEQ;
        return -1;
    }
    value = (unsigned char)*text;
    if (dest)
        *dest = value;
    return value ? 1 : 0;
}

int mblen(const char *text, size_t count) {
    return mbtowc(NULL, text, count);
}

int wctomb(char *dest, wchar_t value) {
    if (!dest)
        return 0;
    if (value > 255) {
        errno = EILSEQ;
        return -1;
    }
    *dest = (char)value;
    return 1;
}

size_t mbstowcs(wchar_t *dest, const char *text, size_t count) {
    size_t i;
    for (i = 0; i < count; i++) {
        dest[i] = (unsigned char)text[i];
        if (!dest[i])
            return i;
    }
    return count;
}

size_t wcstombs(char *dest, const wchar_t *text, size_t count) {
    size_t i;
    for (i = 0; i < count; i++) {
        if (text[i] > 255) {
            errno = EILSEQ;
            return (size_t)-1;
        }
        dest[i] = (char)text[i];
        if (!text[i])
            return i;
    }
    return count;
}
