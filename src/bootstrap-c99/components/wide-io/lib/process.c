#include "winimports.h"
#ifndef __C1_PROCESS_C
#define __C1_PROCESS_C
#include <stdlib.h>
typedef void __c1_exit_function(void);
static __c1_exit_function *__c1_exit_handlers[32];
static int __c1_exit_count;
static __c1_exit_function *__c1_exit_streams;

static void __c1_exit_dispatch(void) {
    __c1_exit_function *f;
    while (__c1_exit_count) {
        f = __c1_exit_handlers[--__c1_exit_count];
        f();
    }
    if (__c1_exit_streams)
        __c1_exit_streams();
}

static void __c1_exit_install(void) {
    /* Object startup calls exit(), which dispatches these callbacks. */
}

static void __c1_exit_set_streams(__c1_exit_function *f) {
    __c1_exit_streams = f;
    __c1_exit_install();
}

int atexit(void (*f)(void)) {
    if (!f || __c1_exit_count == 32)
        return 1;
    __c1_exit_handlers[__c1_exit_count++] = f;
    __c1_exit_install();
    return 0;
}

void _Exit(int status) {
    typedef void Stop(unsigned int);
    Stop *stop = (Stop *)__c1_win_ExitProcess;
    stop((unsigned int)status);
    while (1) {
    }
}

void exit(int status) {
    __c1_exit_dispatch();
    _Exit(status);
}
#endif
