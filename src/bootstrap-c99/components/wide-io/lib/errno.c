#include <errno.h>
#include <string.h>
static int __c1_errno_value;

int *__c1_errno_location(void) {
    return &__c1_errno_value;
}

char *strerror(int error_number) {
    if (error_number == 0)
        return "No error";
    if (error_number == EDOM)
        return "Domain error";
    if (error_number == ERANGE)
        return "Range error";
    if (error_number == EILSEQ)
        return "Illegal byte sequence";
    if (error_number == EINVAL)
        return "Invalid argument";
    return "Unknown error";
}
