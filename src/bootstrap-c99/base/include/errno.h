#ifndef __C1_ERRNO_H
#define __C1_ERRNO_H
int *__c1_errno_location(void);
#define errno (*__c1_errno_location())
#define EDOM 33
#define ERANGE 34
#define EILSEQ 42
/* Implementation extension used for invalid conversion bases. */
#define EINVAL 22
/* File-system error extensions returned by this implementation. */
#define ENOENT 2
#define EACCES 13
#endif
