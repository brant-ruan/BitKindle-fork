#include <locale.h>
#include <limits.h>

/* All supported categories use the C locale; "" selects that environment. */
char *setlocale(int category, const char *locale) {
    if (category < LC_ALL || category > LC_TIME)
        return NULL;
    if (!locale || !locale[0] || (locale[0] == 67 && !locale[1]))
        return "C";
    return NULL;
}
static struct lconv __c1_c_locale = {".",      "",       "",       "",       "",       "",
                                     "",       "",       "",       "",       CHAR_MAX, CHAR_MAX,
                                     CHAR_MAX, CHAR_MAX, CHAR_MAX, CHAR_MAX, CHAR_MAX, CHAR_MAX,
                                     CHAR_MAX, CHAR_MAX, CHAR_MAX, CHAR_MAX, CHAR_MAX, CHAR_MAX};

struct lconv *localeconv(void) {
    return &__c1_c_locale;
}
