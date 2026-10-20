#ifndef __C1_STRING_H
#define __C1_STRING_H
#include <stddef.h>
void *memcpy(void *__c1_dest, const void *__c1_src, size_t __c1_count);
void *memmove(void *__c1_dest, const void *__c1_src, size_t __c1_count);
char *strcpy(char *__c1_dest, const char *__c1_src);
char *strncpy(char *__c1_dest, const char *__c1_src, size_t __c1_count);
char *strcat(char *__c1_dest, const char *__c1_src);
char *strncat(char *__c1_dest, const char *__c1_src, size_t __c1_count);
int memcmp(const void *__c1_left, const void *__c1_right, size_t __c1_count);
int strcmp(const char *__c1_left, const char *__c1_right);
int strcoll(const char *__c1_left, const char *__c1_right);
int strncmp(const char *__c1_left, const char *__c1_right, size_t __c1_count);
size_t strxfrm(char *__c1_dest, const char *__c1_src, size_t __c1_count);
void *memchr(const void *__c1_data, int __c1_value, size_t __c1_count);
char *strchr(const char *__c1_text, int __c1_value);
size_t strcspn(const char *__c1_text, const char *__c1_reject);
char *strpbrk(const char *__c1_text, const char *__c1_accept);
char *strrchr(const char *__c1_text, int __c1_value);
size_t strspn(const char *__c1_text, const char *__c1_accept);
char *strstr(const char *__c1_text, const char *__c1_needle);
char *strtok(char *__c1_text, const char *__c1_delimiters);
void *memset(void *__c1_dest, int __c1_value, size_t __c1_count);
char *strerror(int __c1_error_number);
size_t strlen(const char *__c1_text);
#endif
