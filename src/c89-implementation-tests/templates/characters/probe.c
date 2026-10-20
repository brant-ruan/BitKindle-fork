#include <stddef.h>
#include <limits.h>
#if '\377' != -1 || '\xff' != -1 || 'A' != 65
#error character value differs in preprocessing
#endif
#if '\a' != 7 || '\b' != 8 || '\f' != 12 || '\n' != 10
#error control escape differs in preprocessing
#endif
static unsigned char narrow[] = "\a\b\f\n\r\t\v\\\'\"\?\0\177\200\377\xff";
static wchar_t wide[] = L"\0\xff\x100\xffff";

int main(void) {
    int i;
    unsigned char expected[] = {7, 8, 12, 10, 13, 9, 11, 92, 39, 34, 63, 0, 127, 128, 255, 255, 0};
    if (sizeof(narrow) != sizeof(expected))
        return 1;
    for (i = 0; i < (int)sizeof(expected); i++)
        if (narrow[i] != expected[i])
            return 2;
    if ('\377' != -1 || '\xff' != -1 || '\200' != -128)
        return 3;
    if ('\a' != 7 || '\b' != 8 || '\f' != 12 || '\n' != 10 || '\r' != 13 || '\t' != 9 || '\v' != 11)
        return 4;
    if ('AB' != 0x4142 || 'ABCD' != 0x41424344 || 'ABCDE' != 0x42434445)
        return 5;
    if (L'AB' != 66 || L'\xffff' != 65535 || L'\x100' != 256)
        return 6;
    if (sizeof(wide) != 10 || wide[0] != 0 || wide[1] != 255 || wide[2] != 256 ||
        wide[3] != 65535 || wide[4] != 0)
        return 7;
    if (sizeof(L'A') != sizeof(wchar_t) || sizeof('A') != sizeof(int))
        return 8;
    return 0;
}
