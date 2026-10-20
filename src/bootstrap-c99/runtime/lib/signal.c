#ifndef __C1_SIGNAL_C
#define __C1_SIGNAL_C
#include <signal.h>
#include <stdlib.h>
#include <errno.h>
typedef void __c1_signal_function(int);
static __c1_signal_function *__c1_signal_handlers[23];

static int __c1_signal_valid(int sig) {
    return sig == SIGINT || sig == SIGILL || sig == SIGFPE || sig == SIGSEGV || sig == SIGTERM ||
           sig == SIGABRT;
}

void (*signal(int sig, void (*handler)(int)))(int) {
    __c1_signal_function *old;
    if (!__c1_signal_valid(sig) || handler == SIG_ERR) {
        errno = EINVAL;
        return SIG_ERR;
    }
    old = __c1_signal_handlers[sig];
    __c1_signal_handlers[sig] = handler;
    return old;
}

int raise(int sig) {
    __c1_signal_function *handler;
    if (!__c1_signal_valid(sig)) {
        errno = EINVAL;
        return 1;
    }
    handler = __c1_signal_handlers[sig];
    if (handler == SIG_IGN)
        return 0;
    if (handler == SIG_DFL) {
        _Exit(sig == SIGABRT ? 3 : 128 + sig);
        return 1;
    }
    /* This implementation resets disposition before invoking a handler. */
    __c1_signal_handlers[sig] = SIG_DFL;
    handler(sig);
    return 0;
}

void abort(void) {
    raise(SIGABRT);
    /* Returning from, or ignoring, SIGABRT cannot make abort return. */
    _Exit(3);
}
#endif
