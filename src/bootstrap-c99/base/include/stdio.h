#ifndef __C1_STDIO_H
#define __C1_STDIO_H
#include <stddef.h>
#include <stdarg.h>
#define EOF (-1)
#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2
#define FOPEN_MAX 32
#define FILENAME_MAX 260
#define BUFSIZ 4096
#define L_tmpnam 260
#define TMP_MAX 32767
#define _IONBF 0
#define _IOFBF 1
#define _IOLBF 2

typedef struct __c1_file {
    void *handle;
    int active, readable, writable, binary, append, eof, error;
    int pushed, lookahead;
    unsigned char *buffer;
    size_t capacity, readpos, readend, writeused;
    int owned, buffering, direction;
} FILE;

typedef struct {
    long long offset;
} fpos_t;

FILE *__c1_standard_stream(int);
#define stdin (__c1_standard_stream(0))
#define stdout (__c1_standard_stream(1))
#define stderr (__c1_standard_stream(2))
FILE *fopen(const char *, const char *);
FILE *freopen(const char *, const char *, FILE *);
int fclose(FILE *);
int fflush(FILE *);
int setvbuf(FILE *, char *, int, size_t);
void setbuf(FILE *, char *);
int fgetc(FILE *);
int getc(FILE *);
int getchar(void);
int ungetc(int, FILE *);
int fputc(int, FILE *);
int putc(int, FILE *);
int putchar(int);
char *fgets(char *, int, FILE *);
char *gets(char *);
int fputs(const char *, FILE *);
int puts(const char *);
size_t fread(void *, size_t, size_t, FILE *);
size_t fwrite(const void *, size_t, size_t, FILE *);
int fseek(FILE *, long, int);
long ftell(FILE *);
int fgetpos(FILE *, fpos_t *);
int fsetpos(FILE *, const fpos_t *);
void rewind(FILE *);
void clearerr(FILE *);
int feof(FILE *);
int ferror(FILE *);
int remove(const char *);
int rename(const char *, const char *);
void perror(const char *);
FILE *tmpfile(void);
char *tmpnam(char *);
int printf(const char *, ...);
int fprintf(FILE *, const char *, ...);
int sprintf(char *, const char *, ...);
int snprintf(char *, size_t, const char *, ...);
int vprintf(const char *, va_list);
int vfprintf(FILE *, const char *, va_list);
int vsprintf(char *, const char *, va_list);
int vsnprintf(char *, size_t, const char *, va_list);
int scanf(const char *, ...);
int fscanf(FILE *, const char *, ...);
int sscanf(const char *, const char *, ...);
int vscanf(const char *, va_list);
int vfscanf(FILE *, const char *, va_list);
int vsscanf(const char *, const char *, va_list);
#endif
