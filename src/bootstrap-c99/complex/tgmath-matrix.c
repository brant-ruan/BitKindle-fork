static int macro_matrix(void) {
    int e = 0, f = 0;
    if (acos((float)0.25) != (acosf)((float)0.25))
        return 1;
    if (acos((float complex)(0.25 + 0.5 * I)) != (cacosf)((float complex)(0.25 + 0.5 * I)))
        return 1;
    if (acos((double)0.25) != (acos)((double)0.25))
        return 1;
    if (acos((double complex)(0.25 + 0.5 * I)) != (cacos)((double complex)(0.25 + 0.5 * I)))
        return 1;
    if (acos((long double)0.25) != (acosl)((long double)0.25))
        return 1;
    if (acos((long double complex)(0.25 + 0.5 * I)) !=
        (cacosl)((long double complex)(0.25 + 0.5 * I)))
        return 1;
    if (asin((float)0.25) != (asinf)((float)0.25))
        return 1;
    if (asin((float complex)(0.25 + 0.5 * I)) != (casinf)((float complex)(0.25 + 0.5 * I)))
        return 1;
    if (asin((double)0.25) != (asin)((double)0.25))
        return 1;
    if (asin((double complex)(0.25 + 0.5 * I)) != (casin)((double complex)(0.25 + 0.5 * I)))
        return 1;
    if (asin((long double)0.25) != (asinl)((long double)0.25))
        return 1;
    if (asin((long double complex)(0.25 + 0.5 * I)) !=
        (casinl)((long double complex)(0.25 + 0.5 * I)))
        return 1;
    if (atan((float)0.25) != (atanf)((float)0.25))
        return 1;
    if (atan((float complex)(0.25 + 0.5 * I)) != (catanf)((float complex)(0.25 + 0.5 * I)))
        return 1;
    if (atan((double)0.25) != (atan)((double)0.25))
        return 1;
    if (atan((double complex)(0.25 + 0.5 * I)) != (catan)((double complex)(0.25 + 0.5 * I)))
        return 1;
    if (atan((long double)0.25) != (atanl)((long double)0.25))
        return 1;
    if (atan((long double complex)(0.25 + 0.5 * I)) !=
        (catanl)((long double complex)(0.25 + 0.5 * I)))
        return 1;
    if (cos((float)0.25) != (cosf)((float)0.25))
        return 1;
    if (cos((float complex)(0.25 + 0.5 * I)) != (ccosf)((float complex)(0.25 + 0.5 * I)))
        return 1;
    if (cos((double)0.25) != (cos)((double)0.25))
        return 1;
    if (cos((double complex)(0.25 + 0.5 * I)) != (ccos)((double complex)(0.25 + 0.5 * I)))
        return 1;
    if (cos((long double)0.25) != (cosl)((long double)0.25))
        return 1;
    if (cos((long double complex)(0.25 + 0.5 * I)) !=
        (ccosl)((long double complex)(0.25 + 0.5 * I)))
        return 1;
    if (sin((float)0.25) != (sinf)((float)0.25))
        return 1;
    if (sin((float complex)(0.25 + 0.5 * I)) != (csinf)((float complex)(0.25 + 0.5 * I)))
        return 1;
    if (sin((double)0.25) != (sin)((double)0.25))
        return 1;
    if (sin((double complex)(0.25 + 0.5 * I)) != (csin)((double complex)(0.25 + 0.5 * I)))
        return 1;
    if (sin((long double)0.25) != (sinl)((long double)0.25))
        return 1;
    if (sin((long double complex)(0.25 + 0.5 * I)) !=
        (csinl)((long double complex)(0.25 + 0.5 * I)))
        return 1;
    if (tan((float)0.25) != (tanf)((float)0.25))
        return 1;
    if (tan((float complex)(0.25 + 0.5 * I)) != (ctanf)((float complex)(0.25 + 0.5 * I)))
        return 1;
    if (tan((double)0.25) != (tan)((double)0.25))
        return 1;
    if (tan((double complex)(0.25 + 0.5 * I)) != (ctan)((double complex)(0.25 + 0.5 * I)))
        return 1;
    if (tan((long double)0.25) != (tanl)((long double)0.25))
        return 1;
    if (tan((long double complex)(0.25 + 0.5 * I)) !=
        (ctanl)((long double complex)(0.25 + 0.5 * I)))
        return 1;
    if (cosh((float)0.25) != (coshf)((float)0.25))
        return 1;
    if (cosh((float complex)(0.25 + 0.5 * I)) != (ccoshf)((float complex)(0.25 + 0.5 * I)))
        return 1;
    if (cosh((double)0.25) != (cosh)((double)0.25))
        return 1;
    if (cosh((double complex)(0.25 + 0.5 * I)) != (ccosh)((double complex)(0.25 + 0.5 * I)))
        return 1;
    if (cosh((long double)0.25) != (coshl)((long double)0.25))
        return 1;
    if (cosh((long double complex)(0.25 + 0.5 * I)) !=
        (ccoshl)((long double complex)(0.25 + 0.5 * I)))
        return 1;
    if (sinh((float)0.25) != (sinhf)((float)0.25))
        return 1;
    if (sinh((float complex)(0.25 + 0.5 * I)) != (csinhf)((float complex)(0.25 + 0.5 * I)))
        return 1;
    if (sinh((double)0.25) != (sinh)((double)0.25))
        return 1;
    if (sinh((double complex)(0.25 + 0.5 * I)) != (csinh)((double complex)(0.25 + 0.5 * I)))
        return 1;
    if (sinh((long double)0.25) != (sinhl)((long double)0.25))
        return 1;
    if (sinh((long double complex)(0.25 + 0.5 * I)) !=
        (csinhl)((long double complex)(0.25 + 0.5 * I)))
        return 1;
    if (tanh((float)0.25) != (tanhf)((float)0.25))
        return 1;
    if (tanh((float complex)(0.25 + 0.5 * I)) != (ctanhf)((float complex)(0.25 + 0.5 * I)))
        return 1;
    if (tanh((double)0.25) != (tanh)((double)0.25))
        return 1;
    if (tanh((double complex)(0.25 + 0.5 * I)) != (ctanh)((double complex)(0.25 + 0.5 * I)))
        return 1;
    if (tanh((long double)0.25) != (tanhl)((long double)0.25))
        return 1;
    if (tanh((long double complex)(0.25 + 0.5 * I)) !=
        (ctanhl)((long double complex)(0.25 + 0.5 * I)))
        return 1;
    if (exp((float)0.25) != (expf)((float)0.25))
        return 1;
    if (exp((float complex)(0.25 + 0.5 * I)) != (cexpf)((float complex)(0.25 + 0.5 * I)))
        return 1;
    if (exp((double)0.25) != (exp)((double)0.25))
        return 1;
    if (exp((double complex)(0.25 + 0.5 * I)) != (cexp)((double complex)(0.25 + 0.5 * I)))
        return 1;
    if (exp((long double)0.25) != (expl)((long double)0.25))
        return 1;
    if (exp((long double complex)(0.25 + 0.5 * I)) !=
        (cexpl)((long double complex)(0.25 + 0.5 * I)))
        return 1;
    if (log((float)0.25) != (logf)((float)0.25))
        return 1;
    if (log((float complex)(0.25 + 0.5 * I)) != (clogf)((float complex)(0.25 + 0.5 * I)))
        return 1;
    if (log((double)0.25) != (log)((double)0.25))
        return 1;
    if (log((double complex)(0.25 + 0.5 * I)) != (clog)((double complex)(0.25 + 0.5 * I)))
        return 1;
    if (log((long double)0.25) != (logl)((long double)0.25))
        return 1;
    if (log((long double complex)(0.25 + 0.5 * I)) !=
        (clogl)((long double complex)(0.25 + 0.5 * I)))
        return 1;
    if (sqrt((float)0.25) != (sqrtf)((float)0.25))
        return 1;
    if (sqrt((float complex)(0.25 + 0.5 * I)) != (csqrtf)((float complex)(0.25 + 0.5 * I)))
        return 1;
    if (sqrt((double)0.25) != (sqrt)((double)0.25))
        return 1;
    if (sqrt((double complex)(0.25 + 0.5 * I)) != (csqrt)((double complex)(0.25 + 0.5 * I)))
        return 1;
    if (sqrt((long double)0.25) != (sqrtl)((long double)0.25))
        return 1;
    if (sqrt((long double complex)(0.25 + 0.5 * I)) !=
        (csqrtl)((long double complex)(0.25 + 0.5 * I)))
        return 1;
    if (fabs((float)0.25) != (fabsf)((float)0.25))
        return 1;
    if (fabs((float complex)(0.25 + 0.5 * I)) != (cabsf)((float complex)(0.25 + 0.5 * I)))
        return 1;
    if (fabs((double)0.25) != (fabs)((double)0.25))
        return 1;
    if (fabs((double complex)(0.25 + 0.5 * I)) != (cabs)((double complex)(0.25 + 0.5 * I)))
        return 1;
    if (fabs((long double)0.25) != (fabsl)((long double)0.25))
        return 1;
    if (fabs((long double complex)(0.25 + 0.5 * I)) !=
        (cabsl)((long double complex)(0.25 + 0.5 * I)))
        return 1;
    if (pow((float)0.25, (float)0.5) != (powf)((float)0.25, (float)0.5))
        return 1;
    if (pow((float complex)(0.25 + 0.5 * I), (float complex)(0.5 + 0.25 * I)) !=
        (cpowf)((float complex)(0.25 + 0.5 * I), (float complex)(0.5 + 0.25 * I)))
        return 1;
    if (pow((double)0.25, (double)0.5) != (pow)((double)0.25, (double)0.5))
        return 1;
    if (pow((double complex)(0.25 + 0.5 * I), (double complex)(0.5 + 0.25 * I)) !=
        (cpow)((double complex)(0.25 + 0.5 * I), (double complex)(0.5 + 0.25 * I)))
        return 1;
    if (pow((long double)0.25, (long double)0.5) != (powl)((long double)0.25, (long double)0.5))
        return 1;
    if (pow((long double complex)(0.25 + 0.5 * I), (long double complex)(0.5 + 0.25 * I)) !=
        (cpowl)((long double complex)(0.25 + 0.5 * I), (long double complex)(0.5 + 0.25 * I)))
        return 1;
    if (carg((float)0.25) != (cargf)((float)0.25))
        return 1;
    if (carg((float complex)(0.25 + 0.5 * I)) != (cargf)((float complex)(0.25 + 0.5 * I)))
        return 1;
    if (carg((double)0.25) != (carg)((double)0.25))
        return 1;
    if (carg((double complex)(0.25 + 0.5 * I)) != (carg)((double complex)(0.25 + 0.5 * I)))
        return 1;
    if (carg((long double)0.25) != (cargl)((long double)0.25))
        return 1;
    if (carg((long double complex)(0.25 + 0.5 * I)) !=
        (cargl)((long double complex)(0.25 + 0.5 * I)))
        return 1;
    if (cimag((float)0.25) != (cimagf)((float)0.25))
        return 1;
    if (cimag((float complex)(0.25 + 0.5 * I)) != (cimagf)((float complex)(0.25 + 0.5 * I)))
        return 1;
    if (cimag((double)0.25) != (cimag)((double)0.25))
        return 1;
    if (cimag((double complex)(0.25 + 0.5 * I)) != (cimag)((double complex)(0.25 + 0.5 * I)))
        return 1;
    if (cimag((long double)0.25) != (cimagl)((long double)0.25))
        return 1;
    if (cimag((long double complex)(0.25 + 0.5 * I)) !=
        (cimagl)((long double complex)(0.25 + 0.5 * I)))
        return 1;
    if (conj((float)0.25) != (conjf)((float)0.25))
        return 1;
    if (conj((float complex)(0.25 + 0.5 * I)) != (conjf)((float complex)(0.25 + 0.5 * I)))
        return 1;
    if (conj((double)0.25) != (conj)((double)0.25))
        return 1;
    if (conj((double complex)(0.25 + 0.5 * I)) != (conj)((double complex)(0.25 + 0.5 * I)))
        return 1;
    if (conj((long double)0.25) != (conjl)((long double)0.25))
        return 1;
    if (conj((long double complex)(0.25 + 0.5 * I)) !=
        (conjl)((long double complex)(0.25 + 0.5 * I)))
        return 1;
    if (cproj((float)0.25) != (cprojf)((float)0.25))
        return 1;
    if (cproj((float complex)(0.25 + 0.5 * I)) != (cprojf)((float complex)(0.25 + 0.5 * I)))
        return 1;
    if (cproj((double)0.25) != (cproj)((double)0.25))
        return 1;
    if (cproj((double complex)(0.25 + 0.5 * I)) != (cproj)((double complex)(0.25 + 0.5 * I)))
        return 1;
    if (cproj((long double)0.25) != (cprojl)((long double)0.25))
        return 1;
    if (cproj((long double complex)(0.25 + 0.5 * I)) !=
        (cprojl)((long double complex)(0.25 + 0.5 * I)))
        return 1;
    if (creal((float)0.25) != (crealf)((float)0.25))
        return 1;
    if (creal((float complex)(0.25 + 0.5 * I)) != (crealf)((float complex)(0.25 + 0.5 * I)))
        return 1;
    if (creal((double)0.25) != (creal)((double)0.25))
        return 1;
    if (creal((double complex)(0.25 + 0.5 * I)) != (creal)((double complex)(0.25 + 0.5 * I)))
        return 1;
    if (creal((long double)0.25) != (creall)((long double)0.25))
        return 1;
    if (creal((long double complex)(0.25 + 0.5 * I)) !=
        (creall)((long double complex)(0.25 + 0.5 * I)))
        return 1;
    if (log10((float)0.25) != (log10f)((float)0.25))
        return 1;
    if (log10((double)0.25) != (log10)((double)0.25))
        return 1;
    if (log10((long double)0.25) != (log10l)((long double)0.25))
        return 1;
    if (log2((float)0.25) != (log2f)((float)0.25))
        return 1;
    if (log2((double)0.25) != (log2)((double)0.25))
        return 1;
    if (log2((long double)0.25) != (log2l)((long double)0.25))
        return 1;
    if (log1p((float)0.25) != (log1pf)((float)0.25))
        return 1;
    if (log1p((double)0.25) != (log1p)((double)0.25))
        return 1;
    if (log1p((long double)0.25) != (log1pl)((long double)0.25))
        return 1;
    if (expm1((float)0.25) != (expm1f)((float)0.25))
        return 1;
    if (expm1((double)0.25) != (expm1)((double)0.25))
        return 1;
    if (expm1((long double)0.25) != (expm1l)((long double)0.25))
        return 1;
    if (floor((float)0.25) != (floorf)((float)0.25))
        return 1;
    if (floor((double)0.25) != (floor)((double)0.25))
        return 1;
    if (floor((long double)0.25) != (floorl)((long double)0.25))
        return 1;
    if (ceil((float)0.25) != (ceilf)((float)0.25))
        return 1;
    if (ceil((double)0.25) != (ceil)((double)0.25))
        return 1;
    if (ceil((long double)0.25) != (ceill)((long double)0.25))
        return 1;
    if (trunc((float)0.25) != (truncf)((float)0.25))
        return 1;
    if (trunc((double)0.25) != (trunc)((double)0.25))
        return 1;
    if (trunc((long double)0.25) != (truncl)((long double)0.25))
        return 1;
    if (round((float)0.25) != (roundf)((float)0.25))
        return 1;
    if (round((double)0.25) != (round)((double)0.25))
        return 1;
    if (round((long double)0.25) != (roundl)((long double)0.25))
        return 1;
    if (ilogb((float)0.25) != (ilogbf)((float)0.25))
        return 1;
    if (ilogb((double)0.25) != (ilogb)((double)0.25))
        return 1;
    if (ilogb((long double)0.25) != (ilogbl)((long double)0.25))
        return 1;
    if (logb((float)0.25) != (logbf)((float)0.25))
        return 1;
    if (logb((double)0.25) != (logb)((double)0.25))
        return 1;
    if (logb((long double)0.25) != (logbl)((long double)0.25))
        return 1;
    if (rint((float)0.25) != (rintf)((float)0.25))
        return 1;
    if (rint((double)0.25) != (rint)((double)0.25))
        return 1;
    if (rint((long double)0.25) != (rintl)((long double)0.25))
        return 1;
    if (nearbyint((float)0.25) != (nearbyintf)((float)0.25))
        return 1;
    if (nearbyint((double)0.25) != (nearbyint)((double)0.25))
        return 1;
    if (nearbyint((long double)0.25) != (nearbyintl)((long double)0.25))
        return 1;
    if (lrint((float)0.25) != (lrintf)((float)0.25))
        return 1;
    if (lrint((double)0.25) != (lrint)((double)0.25))
        return 1;
    if (lrint((long double)0.25) != (lrintl)((long double)0.25))
        return 1;
    if (llrint((float)0.25) != (llrintf)((float)0.25))
        return 1;
    if (llrint((double)0.25) != (llrint)((double)0.25))
        return 1;
    if (llrint((long double)0.25) != (llrintl)((long double)0.25))
        return 1;
    if (lround((float)0.25) != (lroundf)((float)0.25))
        return 1;
    if (lround((double)0.25) != (lround)((double)0.25))
        return 1;
    if (lround((long double)0.25) != (lroundl)((long double)0.25))
        return 1;
    if (llround((float)0.25) != (llroundf)((float)0.25))
        return 1;
    if (llround((double)0.25) != (llround)((double)0.25))
        return 1;
    if (llround((long double)0.25) != (llroundl)((long double)0.25))
        return 1;
    if (atan2((float)0.25, (float)0.5) != (atan2f)((float)0.25, (float)0.5))
        return 1;
    if (atan2((double)0.25, (double)0.5) != (atan2)((double)0.25, (double)0.5))
        return 1;
    if (atan2((long double)0.25, (long double)0.5) != (atan2l)((long double)0.25, (long double)0.5))
        return 1;
    if (fmod((float)0.25, (float)0.5) != (fmodf)((float)0.25, (float)0.5))
        return 1;
    if (fmod((double)0.25, (double)0.5) != (fmod)((double)0.25, (double)0.5))
        return 1;
    if (fmod((long double)0.25, (long double)0.5) != (fmodl)((long double)0.25, (long double)0.5))
        return 1;
    if (copysign((float)0.25, (float)0.5) != (copysignf)((float)0.25, (float)0.5))
        return 1;
    if (copysign((double)0.25, (double)0.5) != (copysign)((double)0.25, (double)0.5))
        return 1;
    if (copysign((long double)0.25, (long double)0.5) !=
        (copysignl)((long double)0.25, (long double)0.5))
        return 1;
    if (nextafter((float)0.25, (float)0.5) != (nextafterf)((float)0.25, (float)0.5))
        return 1;
    if (nextafter((double)0.25, (double)0.5) != (nextafter)((double)0.25, (double)0.5))
        return 1;
    if (nextafter((long double)0.25, (long double)0.5) !=
        (nextafterl)((long double)0.25, (long double)0.5))
        return 1;
    if (fmin((float)0.25, (float)0.5) != (fminf)((float)0.25, (float)0.5))
        return 1;
    if (fmin((double)0.25, (double)0.5) != (fmin)((double)0.25, (double)0.5))
        return 1;
    if (fmin((long double)0.25, (long double)0.5) != (fminl)((long double)0.25, (long double)0.5))
        return 1;
    if (fmax((float)0.25, (float)0.5) != (fmaxf)((float)0.25, (float)0.5))
        return 1;
    if (fmax((double)0.25, (double)0.5) != (fmax)((double)0.25, (double)0.5))
        return 1;
    if (fmax((long double)0.25, (long double)0.5) != (fmaxl)((long double)0.25, (long double)0.5))
        return 1;
    if (frexp((float)0.25, &e) != (frexpf)((float)0.25, &f) || e != f)
        return 1;
    if (frexp((double)0.25, &e) != (frexp)((double)0.25, &f) || e != f)
        return 1;
    if (frexp((long double)0.25, &e) != (frexpl)((long double)0.25, &f) || e != f)
        return 1;
    if (ldexp((float)0.25, 2) != (ldexpf)((float)0.25, 2))
        return 1;
    if (ldexp((double)0.25, 2) != (ldexp)((double)0.25, 2))
        return 1;
    if (ldexp((long double)0.25, 2) != (ldexpl)((long double)0.25, 2))
        return 1;
    if (scalbn((float)0.25, 2) != (scalbnf)((float)0.25, 2))
        return 1;
    if (scalbn((double)0.25, 2) != (scalbn)((double)0.25, 2))
        return 1;
    if (scalbn((long double)0.25, 2) != (scalbnl)((long double)0.25, 2))
        return 1;
    if (scalbln((float)0.25, 2) != (scalblnf)((float)0.25, 2))
        return 1;
    if (scalbln((double)0.25, 2) != (scalbln)((double)0.25, 2))
        return 1;
    if (scalbln((long double)0.25, 2) != (scalblnl)((long double)0.25, 2))
        return 1;
    if (nexttoward((float)0.25, 0.5L) != (nexttowardf)((float)0.25, 0.5L))
        return 1;
    if (nexttoward((double)0.25, 0.5L) != (nexttoward)((double)0.25, 0.5L))
        return 1;
    if (nexttoward((long double)0.25, 0.5L) != (nexttowardl)((long double)0.25, 0.5L))
        return 1;
    if (asinh((float)0.25) != (asinhf)((float)0.25))
        return 1;
    if (asinh((float complex)(0.25 + 0.5 * I)) != (casinhf)((float complex)(0.25 + 0.5 * I)))
        return 1;
    if (asinh((double)0.25) != (asinh)((double)0.25))
        return 1;
    if (asinh((double complex)(0.25 + 0.5 * I)) != (casinh)((double complex)(0.25 + 0.5 * I)))
        return 1;
    if (asinh((long double)0.25) != (asinhl)((long double)0.25))
        return 1;
    if (asinh((long double complex)(0.25 + 0.5 * I)) !=
        (casinhl)((long double complex)(0.25 + 0.5 * I)))
        return 1;
    if (acosh((float)2.0) != (acoshf)((float)2.0))
        return 1;
    if (acosh((float complex)(0.25 + 0.5 * I)) != (cacoshf)((float complex)(0.25 + 0.5 * I)))
        return 1;
    if (acosh((double)2.0) != (acosh)((double)2.0))
        return 1;
    if (acosh((double complex)(0.25 + 0.5 * I)) != (cacosh)((double complex)(0.25 + 0.5 * I)))
        return 1;
    if (acosh((long double)2.0) != (acoshl)((long double)2.0))
        return 1;
    if (acosh((long double complex)(0.25 + 0.5 * I)) !=
        (cacoshl)((long double complex)(0.25 + 0.5 * I)))
        return 1;
    if (atanh((float)0.25) != (atanhf)((float)0.25))
        return 1;
    if (atanh((float complex)(0.25 + 0.5 * I)) != (catanhf)((float complex)(0.25 + 0.5 * I)))
        return 1;
    if (atanh((double)0.25) != (atanh)((double)0.25))
        return 1;
    if (atanh((double complex)(0.25 + 0.5 * I)) != (catanh)((double complex)(0.25 + 0.5 * I)))
        return 1;
    if (atanh((long double)0.25) != (atanhl)((long double)0.25))
        return 1;
    if (atanh((long double complex)(0.25 + 0.5 * I)) !=
        (catanhl)((long double complex)(0.25 + 0.5 * I)))
        return 1;
    if (fdim(0.5f, 0.25f) != (fdimf)(0.5f, 0.25f))
        return 1;
    if (fdim(0.5, 0.25) != (fdim)(0.5, 0.25))
        return 1;
    if (fdim(0.5L, 0.25L) != (fdiml)(0.5L, 0.25L))
        return 1;
    if (hypot(3.0f, 4.0f) != (hypotf)(3.0f, 4.0f))
        return 1;
    if (hypot(3.0, 4.0) != (hypot)(3.0, 4.0))
        return 1;
    if (hypot(3.0L, 4.0L) != (hypotl)(3.0L, 4.0L))
        return 1;
    if (remainder((float)7, (float)2) != (remainderf)((float)7, (float)2))
        return 1;
    if (remquo((float)7, (float)2, &e) != (remquof)((float)7, (float)2, &f) || e != f)
        return 1;
    if (remainder((double)7, (double)2) != (remainder)((double)7, (double)2))
        return 1;
    if (remquo((double)7, (double)2, &e) != (remquo)((double)7, (double)2, &f) || e != f)
        return 1;
    if (remainder((long double)7, (long double)2) != (remainderl)((long double)7, (long double)2))
        return 1;
    if (remquo((long double)7, (long double)2, &e) !=
            (remquol)((long double)7, (long double)2, &f) ||
        e != f)
        return 1;
    if (cbrt(8.0f) != (cbrtf)(8.0f))
        return 1;
    if (cbrt(8.0) != (cbrt)(8.0))
        return 1;
    if (cbrt(8.0L) != (cbrtl)(8.0L))
        return 1;
    if (exp2(3.0f) != (exp2f)(3.0f))
        return 1;
    if (exp2(3.0) != (exp2)(3.0))
        return 1;
    if (exp2(3.0L) != (exp2l)(3.0L))
        return 1;
    if (fma(2.0f, 3.0f, 1.0f) != (fmaf)(2.0f, 3.0f, 1.0f))
        return 1;
    if (fma(2.0, 3.0, 1.0) != (fma)(2.0, 3.0, 1.0))
        return 1;
    if (fma(2.0L, 3.0L, 1.0L) != (fmal)(2.0L, 3.0L, 1.0L))
        return 1;
    if (erf(1.0f) != (erff)(1.0f) || erf(1.0) != (erf)(1.0) || erf(1.0L) != (erfl)(1.0L))
        return 1;
    if (erfc(1.0f) != (erfcf)(1.0f) || erfc(1.0) != (erfc)(1.0) || erfc(1.0L) != (erfcl)(1.0L))
        return 1;
    if (lgamma(1.5f) != (lgammaf)(1.5f) || lgamma(1.5) != (lgamma)(1.5) ||
        lgamma(1.5L) != (lgammal)(1.5L))
        return 1;
    if (tgamma(1.5f) != (tgammaf)(1.5f) || tgamma(1.5) != (tgamma)(1.5) ||
        tgamma(1.5L) != (tgammal)(1.5L))
        return 1;
    return 0;
}
