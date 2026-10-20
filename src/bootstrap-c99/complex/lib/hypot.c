#include "src/bootstrap-c99/complex/lib/hypot-core.h"

float hypotf(float x, float y) {
    return __cx_hypotf(x, y);
}

double hypot(double x, double y) {
    return __cx_hypot(x, y);
}

long double hypotl(long double x, long double y) {
    return __cx_hypotl(x, y);
}
