#ifndef __C1_STDLIB_H
#define __C1_STDLIB_H
#include <stddef.h>

/* Implemented interfaces; see LIBRARY.md for remaining library work. */
typedef struct {
    int quot;
    int rem;
} div_t;

typedef struct {
    long quot;
    long rem;
} ldiv_t;

typedef struct {
    long long quot;
    long long rem;
} lldiv_t;

void *malloc(size_t size);
void *calloc(size_t count, size_t size);
void *realloc(void *block, size_t size);
void free(void *block);
int atoi(const char *text);
long atol(const char *text);
long long atoll(const char *text);
long strtol(const char *text, char **end, int base);
unsigned long strtoul(const char *text, char **end, int base);
long long strtoll(const char *text, char **end, int base);
unsigned long long strtoull(const char *text, char **end, int base);
void qsort(void *base, size_t count, size_t size, int (*compare)(const void *, const void *));
void *bsearch(const void *key, const void *base, size_t count, size_t size,
              int (*compare)(const void *, const void *));
#define MB_CUR_MAX ((size_t)1)
int mblen(const char *text, size_t count);
int mbtowc(wchar_t *dest, const char *text, size_t count);
int wctomb(char *dest, wchar_t value);
size_t mbstowcs(wchar_t *dest, const char *text, size_t count);
size_t wcstombs(char *dest, const wchar_t *text, size_t count);
int abs(int value);
long labs(long value);
long long llabs(long long value);
div_t div(int numer, int denom);
ldiv_t ldiv(long numer, long denom);
lldiv_t lldiv(long long numer, long long denom);
int rand(void);
void srand(unsigned int seed);
#define RAND_MAX 32767
#define EXIT_SUCCESS 0
#define EXIT_FAILURE 1
double atof(const char *);
double strtod(const char *, char **);
float strtof(const char *, char **);
long double strtold(const char *, char **);
int atexit(void (*)(void));
void exit(int);
void _Exit(int);
char *getenv(const char *);
void abort(void);
int system(const char *);
#endif
