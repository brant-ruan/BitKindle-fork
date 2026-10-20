#ifndef __C1_STDIO_C
#define __C1_STDIO_C
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <math.h>
#include <fenv.h>
#include <stdio.h>

#include "process.c"
/* Synchronous streams. Explicit setvbuf supports full/line/no buffering.
   Default buffering and process-exit flushing are installed on stream initialization. */
typedef void *__c1_file_open(const char *, unsigned int, unsigned int, void *, unsigned int,
                             unsigned int, void *);
typedef int __c1_file_read(void *, void *, unsigned int, unsigned int *, void *);
typedef int __c1_file_write(void *, const void *, unsigned int, unsigned int *, void *);
typedef int __c1_file_close(void *);
typedef int __c1_file_seek(void *, long long, long long *, unsigned int);
typedef unsigned int __c1_file_error(void);
typedef void *__c1_file_standard(int);
static FILE __c1_files[FOPEN_MAX];
static int __c1_files_ready;
static void __c1_file_finish(void);
static int __c1_file_drain(FILE *);
static int __c1_file_seek_to(FILE *, long long, int);

static int __c1_file_failure(FILE *f) {
    __c1_file_error *last = *(__c1_file_error **)0x4010d0;
    unsigned int e = last();
    errno = e == 2 || e == 3               ? 2
            : e == 5 || e == 32 || e == 33 ? 13
            : e == 6                       ? 9
            : e == 8 || e == 14            ? 12
            : e == 87                      ? 22
            : e == 112                     ? 28
                                           : 5;
    if (f)
        f->error = 1;
    return EOF;
}

static int __c1_file_invalid(FILE *f) {
    errno = 22;
    if (f)
        f->error = 1;
    return EOF;
}

static int __c1_file_read_end(void) {
    __c1_file_error *last = *(__c1_file_error **)0x4010d0;
    unsigned int e = last();
    /* A closed pipe reports ERROR_BROKEN_PIPE instead of a zero-byte success. */
    return e == 109 || e == 38;
}

FILE *__c1_standard_stream(int n) {
    __c1_file_standard *get = *(__c1_file_standard **)0x401040;
    int i;
    if (!__c1_files_ready) {
        for (i = 0; i < 3; i++) {
            __c1_files[i].handle = get(-10 - i);
            __c1_files[i].active = 1;
            __c1_files[i].readable = i == 0;
            __c1_files[i].writable = i != 0;
            __c1_files[i].pushed = -1;
            __c1_files[i].lookahead = -1;
        }
        __c1_files_ready = 1;
        __c1_exit_set_streams(__c1_file_finish);
        for (i = 0; i < 2; i++) {
            typedef int Console(void *, unsigned int *);
            Console *console = *(Console **)0x401110;
            unsigned int mode;
            setvbuf(__c1_files + i, NULL, console(__c1_files[i].handle, &mode) ? _IOLBF : _IOFBF,
                    BUFSIZ);
        }
    }
    return __c1_files + n;
}

static FILE *__c1_file_open_slot(const char *path, const char *mode, FILE *slot) {
    __c1_file_open *open = *(__c1_file_open **)0x401060;
    unsigned int access, creation;
    int plus = 0, binary = 0, first;
    void *handle;
    if (!path || !mode || !*mode) {
        errno = 22;
        return NULL;
    }
    first = *mode++;
    if (first != 'r' && first != 'w' && first != 'a') {
        errno = 22;
        return NULL;
    }
    while (*mode) {
        if (*mode == '+' && !plus)
            plus = 1;
        else if (*mode == 'b' && !binary)
            binary = 1;
        else {
            errno = 22;
            return NULL;
        }
        mode++;
    }
    access = (first == 'r' || plus ? 0x80000000U : 0) | (first != 'r' || plus ? 0x40000000U : 0);
    creation = first == 'r' ? 3 : first == 'w' ? 2 : 4;
    handle = open(path, access, 7, NULL, creation, 0x80, NULL);
    if (handle == (void *)-1) {
        __c1_file_failure(NULL);
        return NULL;
    }
    memset(slot, 0, sizeof(FILE));
    slot->handle = handle;
    slot->active = 1;
    slot->readable = first == 'r' || plus;
    slot->writable = first != 'r' || plus;
    slot->binary = binary;
    slot->append = first == 'a';
    slot->pushed = -1;
    slot->lookahead = -1;
    if (setvbuf(slot, NULL, _IOFBF, BUFSIZ)) {
        fclose(slot);
        return NULL;
    }
    if (slot->append && !plus) {
        __c1_file_seek *seek = *(__c1_file_seek **)0x4010a0;
        if (!seek(handle, 0, NULL, 2)) {
            __c1_file_failure(slot);
            fclose(slot);
            return NULL;
        }
    }
    return slot;
}

FILE *fopen(const char *path, const char *mode) {
    int i;
    __c1_standard_stream(0);
    for (i = 0; i < FOPEN_MAX; i++)
        if (!__c1_files[i].active)
            return __c1_file_open_slot(path, mode, __c1_files + i);
    errno = 24;
    return NULL;
}

int fclose(FILE *f) {
    __c1_file_close *close = *(__c1_file_close **)0x401068;
    int result, flushed;
    if (!f || !f->active)
        return __c1_file_invalid(f);
    flushed = __c1_file_drain(f);
    result = close(f->handle);
    if (!result)
        __c1_file_failure(f);
    if (f->owned)
        free(f->buffer);
    f->buffer = NULL;
    f->capacity = 0;
    f->owned = 0;
    f->active = 0;
    f->pushed = -1;
    f->lookahead = -1;
    return result && flushed == 0 ? 0 : EOF;
}

FILE *freopen(const char *path, const char *mode, FILE *f) {
    if (!f) {
        errno = 22;
        return NULL;
    }
    if (f->active)
        fclose(f);
    return __c1_file_open_slot(path, mode, f);
}

int fflush(FILE *f) {
    int i, result = 0;
    if (!f) {
        for (i = 0; i < FOPEN_MAX; i++)
            if (__c1_files[i].active && __c1_files[i].direction == 2)
                if (__c1_file_drain(__c1_files + i))
                    result = EOF;
        return result;
    }
    if (!f->active)
        return __c1_file_invalid(f);
    return __c1_file_drain(f);
}

int setvbuf(FILE *f, char *buffer, int mode, size_t size) {
    unsigned char *p = NULL;
    int owned = 0;
    if (!f || !f->active || mode < 0 || mode > 2 || (mode != _IONBF && !size)) {
        errno = 22;
        return -1;
    }
    /* A successful call is permitted only before stream I/O. */
    if (f->direction) {
        errno = 22;
        return -1;
    }
    if (mode != _IONBF) {
        p = (unsigned char *)buffer;
        if (!p) {
            p = malloc(size);
            if (!p)
                return -1;
            owned = 1;
        }
    }
    if (f->owned)
        free(f->buffer);
    f->buffer = p;
    f->owned = owned;
    f->buffering = mode;
    f->capacity = mode == _IONBF ? 0 : size;
    f->readpos = 0;
    f->readend = 0;
    f->writeused = 0;
    return 0;
}

void setbuf(FILE *f, char *buffer) {
    setvbuf(f, buffer, buffer ? _IOFBF : _IONBF, BUFSIZ);
}

static void __c1_file_finish(void) {
    int i;
    for (i = 0; i < FOPEN_MAX; i++)
        if (__c1_files[i].active)
            fclose(__c1_files + i);
}

static void __c1_file_prompt_flush(FILE *f) {
    int i;
    if (f->buffering == _IOFBF)
        return;
    for (i = 0; i < FOPEN_MAX; i++)
        if (__c1_files[i].active && __c1_files[i].buffering == _IOLBF &&
            __c1_files[i].direction == 2)
            __c1_file_drain(__c1_files + i);
}

static int __c1_file_raw_get(FILE *f) {
    __c1_file_read *read = *(__c1_file_read **)0x401048;
    unsigned char c;
    unsigned int n;
    int pending = f->lookahead;
    f->lookahead = -1;
    if (pending >= 0)
        return pending;
    if (pending == -2)
        return EOF;
    if (pending == -3)
        return -2;
    if (f->writeused && __c1_file_drain(f))
        return -2;
    f->direction = 1;
    if (f->capacity) {
        if (f->readpos == f->readend) {
            f->readpos = 0;
            f->readend = 0;
            __c1_file_prompt_flush(f);
            if (!read(f->handle, f->buffer,
                      f->capacity > 1048576 ? 1048576 : (unsigned int)f->capacity, &n, NULL)) {
                if (__c1_file_read_end())
                    return EOF;
                __c1_file_failure(f);
                return -2;
            }
            if (!n)
                return EOF;
            f->readend = n;
        }
        return f->buffer[f->readpos++];
    }
    __c1_file_prompt_flush(f);
    if (!read(f->handle, &c, 1, &n, NULL)) {
        if (__c1_file_read_end())
            return EOF;
        __c1_file_failure(f);
        return -2;
    }
    return n ? c : EOF;
}

int fgetc(FILE *f) {
    int c, d;
    if (!f || !f->active || !f->readable)
        return __c1_file_invalid(f);
    if (f->pushed >= 0) {
        c = f->pushed;
        f->pushed = -1;
        return c;
    }
    if (f->eof)
        return EOF;
    c = __c1_file_raw_get(f);
    if (c < 0) {
        if (c == EOF)
            f->eof = 1;
        else
            f->error = 1;
        return EOF;
    }
    if (!f->binary && c == '\r') {
        d = __c1_file_raw_get(f);
        if (d == '\n')
            return '\n';
        f->lookahead = d == EOF ? -2 : d == -2 ? -3 : d;
    }
    return c;
}

int getc(FILE *f) {
    return fgetc(f);
}

int getchar(void) {
    return fgetc(stdin);
}

int ungetc(int c, FILE *f) {
    if (c == EOF)
        return EOF;
    if (!f || !f->active || !f->readable || f->pushed >= 0)
        return EOF;
    f->pushed = (unsigned char)c;
    f->eof = 0;
    return f->pushed;
}

static int __c1_file_seek_to(FILE *f, long long offset, int origin) {
    __c1_file_seek *seek = *(__c1_file_seek **)0x4010a0;
    if (!f || !f->active || origin < 0 || origin > 2)
        return __c1_file_invalid(f);
    if (__c1_file_drain(f))
        return EOF;
    if (origin == SEEK_CUR) {
        offset -= (long long)(f->readend - f->readpos);
        if (f->lookahead >= 0)
            offset--;
        if (f->pushed >= 0)
            offset--;
    }
    if (!seek(f->handle, offset, NULL, origin))
        return __c1_file_failure(f);
    f->pushed = -1;
    f->lookahead = -1;
    f->eof = 0;
    f->readpos = 0;
    f->readend = 0;
    return 0;
}

static long long __c1_file_position(FILE *f) {
    __c1_file_seek *seek = *(__c1_file_seek **)0x4010a0;
    long long position;
    if (!f || !f->active) {
        __c1_file_invalid(f);
        return -1;
    }
    if (!seek(f->handle, 0, &position, SEEK_CUR)) {
        __c1_file_failure(f);
        return -1;
    }
    position -= (long long)(f->readend - f->readpos);
    position += (long long)f->writeused;
    if (f->lookahead >= 0)
        position--;
    if (f->pushed >= 0)
        position--;
    return position < 0 ? 0 : position;
}

static size_t __c1_file_raw_write(FILE *f, const unsigned char *p, size_t size) {
    __c1_file_write *write = *(__c1_file_write **)0x401050;
    __c1_file_seek *seek = *(__c1_file_seek **)0x4010a0;
    size_t done = 0;
    unsigned int count, n;
    if (f->lookahead >= 0 || f->pushed >= 0 || f->readend != f->readpos) {
        if (__c1_file_seek_to(f, 0, SEEK_CUR))
            return 0;
    }
    f->lookahead = -1;
    while (done < size) {
        count = size - done > 1048576 ? 1048576 : (unsigned int)(size - done);
        if (f->append && !seek(f->handle, 0, NULL, SEEK_END)) {
            __c1_file_failure(f);
            break;
        }
        if (!write(f->handle, p + done, count, &n, NULL)) {
            __c1_file_failure(f);
            break;
        }
        if (!n) {
            f->error = 1;
            errno = 5;
            break;
        }
        done += n;
    }
    return done;
}

static int __c1_file_drain(FILE *f) {
    size_t n, used = f->writeused;
    if (!used)
        return 0;
    f->writeused = 0;
    n = __c1_file_raw_write(f, f->buffer, used);
    if (n == used)
        return 0;
    memmove(f->buffer, f->buffer + n, used - n);
    f->writeused = used - n;
    return EOF;
}

static size_t __c1_file_output(FILE *f, const unsigned char *p, size_t size) {
    __c1_file_seek *seek = *(__c1_file_seek **)0x4010a0;
    size_t done = 0, n;
    unsigned char c;
    if (f->lookahead >= 0 || f->pushed >= 0 || f->readend != f->readpos)
        if (__c1_file_seek_to(f, 0, SEEK_CUR))
            return 0;
    f->direction = 2;
    f->lookahead = -1;
    if (!f->capacity)
        return __c1_file_raw_write(f, p, size);
    if (f->append && !f->writeused && !seek(f->handle, 0, NULL, SEEK_END)) {
        __c1_file_failure(f);
        return 0;
    }
    while (done < size) {
        if (f->writeused == f->capacity && __c1_file_drain(f))
            break;
        if (f->buffering == _IOLBF) {
            c = p[done];
            f->buffer[f->writeused++] = c;
            done++;
            if ((c == '\n' || f->writeused == f->capacity) && __c1_file_drain(f))
                return done - 1;
        } else {
            n = size - done;
            if (n > f->capacity - f->writeused)
                n = f->capacity - f->writeused;
            memcpy(f->buffer + f->writeused, p + done, n);
            f->writeused += n;
            done += n;
            if (f->writeused == f->capacity && __c1_file_drain(f))
                return done ? done - 1 : 0;
        }
    }
    return done;
}

int fputc(int value, FILE *f) {
    unsigned char text[2];
    int c = (unsigned char)value;
    if (!f || !f->active || !f->writable)
        return __c1_file_invalid(f);
    if (!f->binary && c == '\n') {
        text[0] = '\r';
        text[1] = '\n';
        if (__c1_file_output(f, text, 2) != 2)
            return EOF;
    } else {
        text[0] = (unsigned char)c;
        if (__c1_file_output(f, text, 1) != 1)
            return EOF;
    }
    return c;
}

int putc(int c, FILE *f) {
    return fputc(c, f);
}

int putchar(int c) {
    return fputc(c, stdout);
}

size_t fread(void *data, size_t size, size_t count, FILE *f) {
    __c1_file_read *read = *(__c1_file_read **)0x401048;
    unsigned char *p = data;
    size_t wanted, done = 0;
    unsigned int chunk, n;
    int c;
    if (!size || !count)
        return 0;
    if (!f || !f->active || !f->readable || count > (size_t)-1 / size) {
        __c1_file_invalid(f);
        return 0;
    }
    wanted = size * count;
    if (!f->binary || f->capacity) {
        while (done < wanted) {
            c = fgetc(f);
            if (c == EOF)
                break;
            p[done++] = (unsigned char)c;
        }
    } else {
        f->direction = 1;
        if (f->pushed >= 0) {
            p[done++] = (unsigned char)f->pushed;
            f->pushed = -1;
        }
        while (done < wanted && !f->eof) {
            chunk = wanted - done > 1048576 ? 1048576 : (unsigned int)(wanted - done);
            __c1_file_prompt_flush(f);
            if (!read(f->handle, p + done, chunk, &n, NULL)) {
                if (__c1_file_read_end())
                    f->eof = 1;
                else
                    __c1_file_failure(f);
                break;
            }
            if (!n) {
                f->eof = 1;
                break;
            }
            done += n;
        }
    }
    return done / size;
}

size_t fwrite(const void *data, size_t size, size_t count, FILE *f) {
    const unsigned char *p = data;
    size_t wanted, done = 0;
    if (!size || !count)
        return 0;
    if (!f || !f->active || !f->writable || count > (size_t)-1 / size) {
        __c1_file_invalid(f);
        return 0;
    }
    wanted = size * count;
    if (f->binary)
        done = __c1_file_output(f, p, wanted);
    else
        while (done < wanted) {
            if (fputc(p[done], f) == EOF)
                break;
            done++;
        }
    return done / size;
}

char *fgets(char *s, int n, FILE *f) {
    int i = 0, c;
    if (n <= 0) {
        errno = 22;
        return NULL;
    }
    while (i < n - 1) {
        c = fgetc(f);
        if (c == EOF) {
            if (!i || !feof(f))
                return NULL;
            break;
        }
        s[i++] = (char)c;
        if (c == '\n')
            break;
    }
    s[i] = 0;
    return s;
}

int fputs(const char *s, FILE *f) {
    while (*s)
        if (fputc((unsigned char)*s++, f) == EOF)
            return EOF;
    return 0;
}

char *gets(char *s) {
    int c;
    char *p = s;
    FILE *f = stdin;
    while (1) {
        c = fgetc(f);
        if (c == EOF) {
            if (p == s || !feof(f))
                return NULL;
            break;
        }
        if (c == '\n')
            break;
        *p++ = (char)c;
    }
    *p = 0;
    return s;
}

void perror(const char *s) {
    int saved = errno;
    FILE *f = stderr;
    if (s && *s) {
        fputs(s, f);
        fputs(": ", f);
    }
    fputs(strerror(saved), f);
    fputc('\n', f);
    errno = saved;
}

int puts(const char *s) {
    if (fputs(s, stdout) == EOF || fputc('\n', stdout) == EOF)
        return EOF;
    return 0;
}

int fseek(FILE *f, long offset, int origin) {
    return __c1_file_seek_to(f, offset, origin);
}

long ftell(FILE *f) {
    long long p = __c1_file_position(f);
    if (p < 0)
        return -1;
    if (p > 2147483647LL) {
        errno = ERANGE;
        return -1;
    }
    return (long)p;
}

int fgetpos(FILE *f, fpos_t *position) {
    long long p = __c1_file_position(f);
    if (p < 0)
        return -1;
    position->offset = p;
    return 0;
}

int fsetpos(FILE *f, const fpos_t *position) {
    return __c1_file_seek_to(f, position->offset, SEEK_SET);
}

void clearerr(FILE *f) {
    f->error = 0;
    f->eof = 0;
    if (f->lookahead < -1)
        f->lookahead = -1;
}

void rewind(FILE *f) {
    fseek(f, 0, SEEK_SET);
    clearerr(f);
}

int feof(FILE *f) {
    return f->eof != 0;
}

int ferror(FILE *f) {
    return f->error != 0;
}

int remove(const char *path) {
    typedef int Delete(const char *);
    typedef unsigned int Attributes(const char *);
    Delete *delete = *(Delete **)0x4010b0;
    Delete *directory = *(Delete **)0x401140;
    Attributes *attributes = *(Attributes **)0x401148;
    unsigned int flags;
    if (!path)
        return __c1_file_invalid(NULL);
    flags = attributes(path);
    if (flags == 0xffffffffU)
        return __c1_file_failure(NULL);
    return (flags & 16 ? directory(path) : delete (path)) ? 0 : __c1_file_failure(NULL);
}

int rename(const char *old, const char *newname) {
    typedef int Move(const char *, const char *, unsigned int);
    Move *move = *(Move **)0x4010b8;
    return move(old, newname, 1) ? 0 : __c1_file_failure(NULL);
}

#endif
