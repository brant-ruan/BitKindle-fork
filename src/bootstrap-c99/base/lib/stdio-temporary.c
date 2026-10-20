#ifndef __C1_STDIO_TEMPORARY_C
#define __C1_STDIO_TEMPORARY_C
#include "stdio.c"
static unsigned long long __c1_tmp_serial, __c1_tmp_epoch;
static unsigned int __c1_tmp_pid;
static int __c1_tmp_ready;
static char __c1_tmp_result[L_tmpnam];

static char *__c1_tmp_hex(char *p, unsigned long long n, int digits) {
    int i;
    for (i = digits - 1; i >= 0; i--)
        *p++ = "0123456789abcdef"[(n >> (i * 4)) & 15];
    return p;
}

static int __c1_tmp_name(char *path, unsigned long long serial) {
    typedef unsigned int TempPath(unsigned int, char *);
    typedef unsigned int Pid(void);
    typedef unsigned long long Ticks(void);
    TempPath *get = *(TempPath **)0x401100;
    Pid *pid = *(Pid **)0x4010f8;
    Ticks *ticks = *(Ticks **)0x4010f0;
    unsigned int n;
    char *p;
    if (!__c1_tmp_ready) {
        __c1_tmp_pid = pid();
        __c1_tmp_epoch = ticks();
        __c1_tmp_ready = 1;
    }
    n = get(L_tmpnam, path);
    if (!n) {
        __c1_file_failure(NULL);
        return -1;
    }
    if (n > L_tmpnam - 50) {
        errno = 34;
        return -1;
    }
    p = path + n;
    *p++ = 'c';
    *p++ = '1';
    *p++ = '-';
    p = __c1_tmp_hex(p, __c1_tmp_pid, 8);
    *p++ = '-';
    p = __c1_tmp_hex(p, __c1_tmp_epoch, 16);
    *p++ = '-';
    p = __c1_tmp_hex(p, serial, 16);
    strcpy(p, ".tmp");
    return 0;
}

static void *__c1_tmp_create(char *path) {
    __c1_file_open *open = *(__c1_file_open **)0x401060;
    __c1_file_error *last = *(__c1_file_error **)0x4010d0;
    unsigned int error;
    void *handle;
    while (1) {
        if (!++__c1_tmp_serial) {
            errno = 34;
            return NULL;
        }
        if (__c1_tmp_name(path, __c1_tmp_serial))
            return NULL;
        /* CREATE_NEW never opens an existing candidate. Delete-on-close is attached
     atomically to that new handle, including the short-lived tmpnam probe. */
        handle = open(path, 0xc0010000U, 7, NULL, 1, 0x04000100, NULL);
        if (handle != (void *)-1)
            return handle;
        error = last();
        if (error != 80 && error != 183) {
            __c1_file_failure(NULL);
            return NULL;
        }
    }
}

char *tmpnam(char *s) {
    __c1_file_close *close = *(__c1_file_close **)0x401068;
    char path[L_tmpnam];
    void *handle = __c1_tmp_create(path);
    if (!handle)
        return NULL;
    if (!close(handle)) {
        __c1_file_failure(NULL);
        return NULL;
    }
    if (!s)
        s = __c1_tmp_result;
    strcpy(s, path);
    return s;
}

FILE *tmpfile(void) {
    FILE *f = NULL;
    void *handle;
    char path[L_tmpnam];
    int i;
    __c1_standard_stream(0);
    for (i = 0; i < FOPEN_MAX; i++)
        if (!__c1_files[i].active) {
            f = __c1_files + i;
            break;
        }
    if (!f) {
        errno = 24;
        return NULL;
    }
    handle = __c1_tmp_create(path);
    if (!handle)
        return NULL;
    memset(f, 0, sizeof(FILE));
    f->handle = handle;
    f->active = 1;
    f->readable = 1;
    f->writable = 1;
    f->binary = 1;
    f->pushed = -1;
    f->lookahead = -1;
    if (setvbuf(f, NULL, _IOFBF, BUFSIZ)) {
        fclose(f);
        return NULL;
    }
    return f;
}

#endif
