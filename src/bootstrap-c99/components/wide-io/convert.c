#include "wchar.h"
#include <errno.h>

int fwide(FILE *f, int mode) {
    if (!f || !f->active)
        return 0;
    if (!f->orientation && mode)
        f->orientation = mode > 0 ? 1 : -1;
    return f->orientation;
}

wint_t fgetwc(FILE *f) {
    int c;
    if (__c1_file_orient(f, 1))
        return WEOF;
    c = __c1_file_get(f);
    return c == EOF ? WEOF : (wint_t)c;
}

wint_t getwc(FILE *f) {
    return fgetwc(f);
}

wint_t getwchar(void) {
    return fgetwc(stdin);
}

wint_t fputwc(wchar_t c, FILE *f) {
    if (__c1_file_orient(f, 1))
        return WEOF;
    if (c > 255) {
        errno = EILSEQ;
        f->error = 1;
        return WEOF;
    }
    return __c1_file_put(c, f) == EOF ? WEOF : (wint_t)c;
}

wint_t putwc(wchar_t c, FILE *f) {
    return fputwc(c, f);
}

wint_t putwchar(wchar_t c) {
    return fputwc(c, stdout);
}

wchar_t *fgetws(wchar_t *s, int n, FILE *f) {
    int i = 0;
    wint_t c;
    if (__c1_file_orient(f, 1))
        return NULL;
    if (n <= 0) {
        errno = EINVAL;
        return NULL;
    }
    while (i < n - 1) {
        c = fgetwc(f);
        if (c == WEOF) {
            if (!i || !feof(f))
                return NULL;
            break;
        }
        s[i++] = (wchar_t)c;
        if (c == '\n')
            break;
    }
    s[i] = 0;
    return s;
}

int fputws(const wchar_t *s, FILE *f) {
    if (__c1_file_orient(f, 1))
        return -1;
    while (*s)
        if (fputwc(*s++, f) == WEOF)
            return -1;
    return 0;
}

wint_t ungetwc(wint_t c, FILE *f) {
    if (c == WEOF)
        return WEOF;
    if (__c1_file_orient(f, 1))
        return WEOF;
    if (!f->readable || f->pushed >= 0 || c > 65535)
        return WEOF;
    f->pushed = (int)c;
    f->eof = 0;
    return c;
}
