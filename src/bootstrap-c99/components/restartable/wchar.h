#ifndef __C1_WCHAR_H
#define __C1_WCHAR_H
/* Development header: strings and restartable conversions; numeric conversion and I/O pending. */
#include <stddef.h>
#include <stdint.h>
#include <wctype.h>
wchar_t *wcscpy(wchar_t *restrict, const wchar_t *restrict);
wchar_t *wcsncpy(wchar_t *restrict, const wchar_t *restrict, size_t);
wchar_t *wcscat(wchar_t *restrict, const wchar_t *restrict);
wchar_t *wcsncat(wchar_t *restrict, const wchar_t *restrict, size_t);
int wcscmp(const wchar_t *, const wchar_t *);
int wcsncmp(const wchar_t *, const wchar_t *, size_t);
int wcscoll(const wchar_t *, const wchar_t *);
size_t wcsxfrm(wchar_t *restrict, const wchar_t *restrict, size_t);
wchar_t *wcschr(const wchar_t *, wchar_t);
wchar_t *wcsrchr(const wchar_t *, wchar_t);
size_t wcscspn(const wchar_t *, const wchar_t *);
size_t wcsspn(const wchar_t *, const wchar_t *);
wchar_t *wcspbrk(const wchar_t *, const wchar_t *);
wchar_t *wcsstr(const wchar_t *, const wchar_t *);
wchar_t *wcstok(wchar_t *restrict, const wchar_t *restrict, wchar_t **restrict);
size_t wcslen(const wchar_t *);
wchar_t *wmemchr(const wchar_t *, wchar_t, size_t);
int wmemcmp(const wchar_t *, const wchar_t *, size_t);
wchar_t *wmemcpy(wchar_t *restrict, const wchar_t *restrict, size_t);
wchar_t *wmemmove(wchar_t *, const wchar_t *, size_t);
wchar_t *wmemset(wchar_t *, wchar_t, size_t);

typedef struct {
    unsigned int __state;
} mbstate_t;

wint_t btowc(int);
int wctob(wint_t);
int mbsinit(const mbstate_t *);
size_t mbrtowc(wchar_t *restrict, const char *restrict, size_t, mbstate_t *restrict);
size_t mbrlen(const char *restrict, size_t, mbstate_t *restrict);
size_t wcrtomb(char *restrict, wchar_t, mbstate_t *restrict);
size_t mbsrtowcs(wchar_t *restrict, const char **restrict, size_t, mbstate_t *restrict);
size_t wcsrtombs(char *restrict, const wchar_t **restrict, size_t, mbstate_t *restrict);
#endif
