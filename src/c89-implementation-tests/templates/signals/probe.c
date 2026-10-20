#include <signal.h>
#include <stdlib.h>
static volatile sig_atomic_t seen;

static void handler(int sig) {
    seen = sig;
}

int main(void) {

#if TEST_MODE == 0
    if (signal(TEST_SIGNAL, SIG_DFL) != SIG_DFL)
        return 91;
    raise(TEST_SIGNAL);
    return 92;
#elif TEST_MODE == 1
    if (signal(TEST_SIGNAL, SIG_IGN) != SIG_DFL)
        return 91;
    if (raise(TEST_SIGNAL) || seen)
        return 92;
    if (signal(TEST_SIGNAL, SIG_DFL) != SIG_IGN)
        return 93;
    return 0;
#elif TEST_MODE == 2
    if (signal(TEST_SIGNAL, handler) != SIG_DFL)
        return 91;
    if (raise(TEST_SIGNAL) || seen != TEST_SIGNAL)
        return 92;
    if (signal(TEST_SIGNAL, SIG_IGN) != SIG_DFL)
        return 93;
    return 0;
#elif TEST_MODE == 3
    abort();
#elif TEST_MODE == 4
    signal(SIGABRT, SIG_IGN);
    abort();
#elif TEST_MODE == 5
    signal(SIGABRT, handler);
    abort();
#endif
    return 94;
}
