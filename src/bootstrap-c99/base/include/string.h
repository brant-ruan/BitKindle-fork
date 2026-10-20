#ifndef __C1_STRING_H
#define __C1_STRING_H
#include <stddef.h>
void *memcpy(void *dest, const void *src, size_t count);
void *memmove(void *dest, const void *src, size_t count);
char *strcpy(char *dest, const char *src);
char *strncpy(char *dest, const char *src, size_t count);
char *strcat(char *dest, const char *src);
char *strncat(char *dest, const char *src, size_t count);
int memcmp(const void *left, const void *right, size_t count);
int strcmp(const char *left, const char *right);
int strcoll(const char *left, const char *right);
int strncmp(const char *left, const char *right, size_t count);
size_t strxfrm(char *dest, const char *src, size_t count);
void *memchr(const void *data, int value, size_t count);
char *strchr(const char *text, int value);
size_t strcspn(const char *text, const char *reject);
char *strpbrk(const char *text, const char *accept);
char *strrchr(const char *text, int value);
size_t strspn(const char *text, const char *accept);
char *strstr(const char *text, const char *needle);
char *strtok(char *text, const char *delimiters);
void *memset(void *dest, int value, size_t count);
char *strerror(int error_number);
size_t strlen(const char *text);
#endif
