#ifndef __C1_WCTYPE_H
#define __C1_WCTYPE_H
typedef unsigned int wint_t;
typedef unsigned long wctype_t;
typedef int wctrans_t;
#define WEOF ((wint_t)0xffffffffU)
int iswalnum(wint_t);
int iswalpha(wint_t);
int iswblank(wint_t);
int iswcntrl(wint_t);
int iswdigit(wint_t);
int iswgraph(wint_t);
int iswlower(wint_t);
int iswprint(wint_t);
int iswpunct(wint_t);
int iswspace(wint_t);
int iswupper(wint_t);
int iswxdigit(wint_t);
wctype_t wctype(const char *);
int iswctype(wint_t, wctype_t);
wint_t towlower(wint_t);
wint_t towupper(wint_t);
wctrans_t wctrans(const char *);
wint_t towctrans(wint_t, wctrans_t);
#endif
