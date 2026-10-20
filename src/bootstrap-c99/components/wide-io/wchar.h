#ifndef __C1_WCHAR_H
#define __C1_WCHAR_H
/* Development header: strings and restartable conversions; formatted wide I/O pending. */
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
long wcstol(const wchar_t *restrict, wchar_t **restrict, int);
unsigned long wcstoul(const wchar_t *restrict, wchar_t **restrict, int);
long long wcstoll(const wchar_t *restrict, wchar_t **restrict, int);
unsigned long long wcstoull(const wchar_t *restrict, wchar_t **restrict, int);
double wcstod(const wchar_t *restrict, wchar_t **restrict);
float wcstof(const wchar_t *restrict, wchar_t **restrict);
long double wcstold(const wchar_t *restrict, wchar_t **restrict);
#include <time.h>
size_t wcsftime(wchar_t *restrict, size_t, const wchar_t *restrict, const struct tm *restrict);
#include <stdio.h>
int fwide(FILE *, int);
wint_t fgetwc(FILE *);
wint_t getwc(FILE *);
wint_t getwchar(void);
wint_t fputwc(wchar_t, FILE *);
wint_t putwc(wchar_t, FILE *);
wint_t putwchar(wchar_t);
wchar_t *fgetws(wchar_t *restrict, int, FILE *restrict);
int fputws(const wchar_t *restrict, FILE *restrict);
wint_t ungetwc(wint_t, FILE *);
#endif
