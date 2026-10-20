#include <wchar.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>

int main(void) {
    FILE *f;
    wchar_t b[16];
    fpos_t pos;
    char name[L_tmpnam], bytes[16];
    int value;
    f = tmpfile();
    if (!f || fwide(f, 0))
        return 1;
    if (fwide(f, 1) <= 0 || fwide(f, -1) <= 0)
        return 2;
    if (fputws(L"ab\n", f) < 0 || putwc(255, f) != 255 || fputwc(0, f) != 0)
        return 3;
    if (fflush(f) || fseek(f, 0, SEEK_SET) || fwide(f, 0) <= 0)
        return 4;
    if (fgetws(b, 16, f) != b || wcscmp(b, L"ab\n"))
        return 5;
    if (fgetpos(f, &pos) || getwc(f) != 255 || fgetwc(f) != 0 || fgetwc(f) != WEOF || !feof(f))
        return 6;
    if (ungetwc(WEOF, f) != WEOF || !feof(f))
        return 7;
    if (ungetwc(0x1234, f) != 0x1234 || feof(f) || getwc(f) != 0x1234)
        return 8;
    if (fsetpos(f, &pos) || getwc(f) != 255)
        return 9;
    rewind(f);
    if (fwide(f, 0) <= 0 || getwc(f) != 'a')
        return 10;
    if (ungetwc('X', f) != 'X' || ungetwc('Y', f) != WEOF || getwc(f) != 'X' || getwc(f) != 'b')
        return 11;
    rewind(f);
    errno = 0;
    if (fputwc(0x1234, f) != WEOF || errno != EILSEQ || !ferror(f))
        return 12;
    clearerr(f);
    if (ferror(f) || feof(f) || fwide(f, 0) <= 0 || fclose(f))
        return 13;
    f = tmpfile();
    if (!f)
        return 14;
    if (fputs("", f) || fwide(f, 0) >= 0 || fclose(f))
        return 15;
    f = tmpfile();
    if (!f || fprintf(f, "") != 0 || fwide(f, 0) >= 0 || fclose(f))
        return 16;
    f = tmpfile();
    if (!f || fwrite("xyz", 1, 3, f) != 3 || fwide(f, 0) >= 0)
        return 17;
    rewind(f);
    if (fread(bytes, 1, 3, f) != 3 || memcmp(bytes, "xyz", 3) || fclose(f))
        return 18;
    f = tmpfile();
    if (!f || fprintf(f, "%d", 42) != 2)
        return 19;
    rewind(f);
    if (fscanf(f, "%d", &value) != 1 || value != 42 || fclose(f))
        return 20;
    if (!tmpnam(name))
        return 21;
    f = fopen(name, "w+");
    if (!f || fputws(L"x\ny", f) < 0 || fflush(f))
        return 22;
    rewind(f);
    if (getwc(f) != 'x' || getwc(f) != '\n' || getwc(f) != 'y')
        return 23;
    if (freopen(name, "rb", f) != f || fwide(f, 0))
        return 24;
    if (fread(bytes, 1, 4, f) != 4 || memcmp(bytes, "x\r\ny", 4) || fwide(f, 0) >= 0 || fclose(f))
        return 25;
    if (remove(name))
        return 26;
    f = tmpfile();
    if (!f || fputws(L"", f) < 0 || fwide(f, 0) <= 0)
        return 27;
    b[0] = 123;
    rewind(f);
    if (fgetws(b, 16, f) || b[0] != 123 || !feof(f) || fclose(f))
        return 28;
    return 0;
}
