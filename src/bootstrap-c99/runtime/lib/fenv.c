#include <fenv.h>

static unsigned int __c1_fe_read(void) {
    return ((unsigned int (*)(void)) "\x0f\xae\x5c\x24\x08\x8b\x44\x24\x08\xc3")();
}

static void __c1_fe_write(unsigned int value) {
    ((void (*)(unsigned int)) "\x89\x4c\x24\x08\x0f\xae\x54\x24\x08\xc3")(value);
}

int feclearexcept(int excepts) {
    __c1_fe_write(__c1_fe_read() & ~(unsigned int)(excepts & FE_ALL_EXCEPT));
    return 0;
}

int fegetexceptflag(fexcept_t *flag, int excepts) {
    *flag = __c1_fe_read() & (unsigned int)(excepts & FE_ALL_EXCEPT);
    return 0;
}

int fesetexceptflag(const fexcept_t *flag, int excepts) {
    unsigned int mask = (unsigned int)(excepts & FE_ALL_EXCEPT);
    __c1_fe_write((__c1_fe_read() & ~mask) | (*flag & mask));
    return 0;
}

int fetestexcept(int excepts) {
    return (int)(__c1_fe_read() & (unsigned int)(excepts & FE_ALL_EXCEPT));
}

int fegetround(void) {
    return (int)(__c1_fe_read() & 0x6000U);
}

int fesetround(int round) {
    if (round != FE_TONEAREST && round != FE_DOWNWARD && round != FE_UPWARD &&
        round != FE_TOWARDZERO)
        return 1;
    __c1_fe_write((__c1_fe_read() & ~0x6000U) | (unsigned int)round);
    return 0;
}

int fegetenv(fenv_t *env) {
    env->__mxcsr = __c1_fe_read();
    return 0;
}

int feholdexcept(fenv_t *env) {
    env->__mxcsr = __c1_fe_read();
    /* Mask every SSE exception and clear all six hardware status flags. */
    __c1_fe_write((env->__mxcsr | 0x1f80U) & ~0x3fU);
    return 0;
}

int fesetenv(const fenv_t *env) {
    __c1_fe_write(env == FE_DFL_ENV ? 0x1f80U : env->__mxcsr);
    return 0;
}

int feraiseexcept(int excepts) {
    void (*divide)(unsigned long long, unsigned long long);
    void (*multiply)(unsigned long long, unsigned long long);
    /* Genuine SSE arithmetic raises exceptions, including a hardware trap
       if the caller has unmasked that exception in its environment. */
    divide = (void (*)(
        unsigned long long,
        unsigned long long)) "\x66\x48\x0f\x6e\xc1\x66\x48\x0f\x6e\xca\xf2\x0f\x5e\xc1\xc3";
    multiply = (void (*)(
        unsigned long long,
        unsigned long long)) "\x66\x48\x0f\x6e\xc1\x66\x48\x0f\x6e\xca\xf2\x0f\x59\xc1\xc3";
    if (excepts & FE_INVALID)
        divide(0, 0);
    if (excepts & FE_DIVBYZERO)
        divide(0x3ff0000000000000ULL, 0);
    if (excepts & FE_OVERFLOW)
        multiply(0x7fefffffffffffffULL, 0x4000000000000000ULL);
    if (excepts & FE_UNDERFLOW)
        multiply(0x0010000000000000ULL, 0x0010000000000000ULL);
    if (excepts & FE_INEXACT)
        divide(0x3ff0000000000000ULL, 0x4008000000000000ULL);
    return 0;
}

int feupdateenv(const fenv_t *env) {
    int raised = fetestexcept(FE_ALL_EXCEPT);
    if (fesetenv(env))
        return 1;
    return feraiseexcept(raised);
}
