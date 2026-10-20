#include <wchar.h>
#include <time.h>
#include <string.h>

int main(void) {
    const char *formats[] = {"%a|%A|%b|%B|%h|%c|%C|%d|%D|%e|%F|%g|%G|%V",
                             "%H|%I|%j|%m|%M|%n|%p|%r|%R|%S|%t|%T|%u|%U|%w|%W|%x|%X|%y|%Y|%z|%Z|%%",
                             "%Ec|%EC|%Ex|%EX|%Ey|%EY",
                             "%Od|%Oe|%OH|%OI|%Om|%OM|%OS|%Ou|%OU|%OV|%Ow|%OW|%Oy"};
    struct tm t = {0};
    char narrow[512];
    wchar_t wide[512], format[512];
    size_t n, result, cap;
    int i, j;
    t.tm_year = 116;
    t.tm_mon = 0;
    t.tm_mday = 1;
    t.tm_wday = 5;
    t.tm_yday = 0;
    t.tm_hour = 13;
    t.tm_min = 4;
    t.tm_sec = 5;
    t.tm_isdst = -1;
    for (i = 0; i < 4; i++) {
        for (j = 0; formats[i][j]; j++)
            format[j] = (unsigned char)formats[i][j];
        format[j] = 0;
        n = strftime(narrow, 512, formats[i], &t);
        if (!n)
            return 1;
        for (cap = 0; cap <= n + 1; cap++) {
            for (j = 0; j < 512; j++)
                wide[j] = 0xabcd;
            result = wcsftime(wide, cap, format, &t);
            if (cap <= n) {
                if (result)
                    return 2;
            } else {
                if (result != n || wide[n])
                    return 3;
                for (j = 0; j < (int)n; j++)
                    if (wide[j] != (unsigned char)narrow[j])
                        return 4;
            }
            for (j = (int)cap; j < 512; j++)
                if (wide[j] != 0xabcd)
                    return 5;
        }
    }
    if (wcsftime(wide, 512, L"\x4e2d %F %G-W%V-%u \xffff", &t) != 25)
        return 6;
    if (wcscmp(wide, L"\x4e2d 2016-01-01 2015-W53-5 \xffff"))
        return 7;
    if (wcsftime(wide, 512, L"a%zb%Zc", &t) != 3 || wcscmp(wide, L"abc"))
        return 8;
    wide[0] = 42;
    if (wcsftime(wide, 1, L"", &t) || wide[0])
        return 9;
    wide[0] = 42;
    if (wcsftime(wide, 0, L"", &t) || wide[0] != 42)
        return 10;
    if (wcsftime(wide, 512, L"%\x0141", &t))
        return 11;
    return 0;
}
