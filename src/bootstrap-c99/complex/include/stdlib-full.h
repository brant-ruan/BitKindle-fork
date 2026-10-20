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

void *malloc(size_t __c1_size);
void *calloc(size_t __c1_count, size_t __c1_size);
void *realloc(void *__c1_block, size_t __c1_size);
void free(void *__c1_block);
int atoi(const char *__c1_text);
long atol(const char *__c1_text);
long long atoll(const char *__c1_text);
long strtol(const char *__c1_text, char **__c1_end, int __c1_base);
unsigned long strtoul(const char *__c1_text, char **__c1_end, int __c1_base);
long long strtoll(const char *__c1_text, char **__c1_end, int __c1_base);
unsigned long long strtoull(const char *__c1_text, char **__c1_end, int __c1_base);
void qsort(void *__c1_base, size_t __c1_count, size_t __c1_size,
           int (*__c1_compare)(const void *, const void *));
void *bsearch(const void *__c1_key, const void *__c1_base, size_t __c1_count, size_t __c1_size,
              int (*__c1_compare)(const void *, const void *));
#define MB_CUR_MAX ((size_t)1)
int mblen(const char *__c1_text, size_t __c1_count);
int mbtowc(wchar_t *__c1_dest, const char *__c1_text, size_t __c1_count);
int wctomb(char *__c1_dest, wchar_t __c1_value);
size_t mbstowcs(wchar_t *__c1_dest, const char *__c1_text, size_t __c1_count);
size_t wcstombs(char *__c1_dest, const wchar_t *__c1_text, size_t __c1_count);
int abs(int __c1_value);
long labs(long __c1_value);
long long llabs(long long __c1_value);
div_t div(int __c1_numer, int __c1_denom);
ldiv_t ldiv(long __c1_numer, long __c1_denom);
lldiv_t lldiv(long long __c1_numer, long long __c1_denom);
int rand(void);
void srand(unsigned int __c1_seed);
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
