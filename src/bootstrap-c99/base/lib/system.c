#ifndef __C1_SYSTEM_C
#define __C1_SYSTEM_C
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "stdio.c"

struct __c1_startup {
    unsigned int size;
    char *reserved, *desktop, *title;
    unsigned int x, y, width, height, columns, rows, fill, flags;
    unsigned short show, reserved_count;
    unsigned char *reserved_bytes;
    void *input, *output, *error;
};

struct __c1_process {
    void *process, *thread;
    unsigned int process_id, thread_id;
};

static char *__c1_command_processor(void) {
    typedef unsigned int Directory(char *, unsigned int);
    Directory *directory = *(Directory **)0x401150;
    unsigned int capacity, used;
    char *path;
    capacity = directory(NULL, 0);
    if (!capacity) {
        __c1_file_failure(NULL);
        return NULL;
    }
    if (capacity > 32758) {
        errno = EINVAL;
        return NULL;
    }
    path = malloc((size_t)capacity + 9);
    if (!path)
        return NULL;
    used = directory(path, capacity);
    if (!used) {
        __c1_file_failure(NULL);
        free(path);
        return NULL;
    }
    if (used >= capacity) {
        free(path);
        errno = EINVAL;
        return NULL;
    }
    memcpy(path + used, "\\cmd.exe", 9);
    return path;
}

int system(const char *command) {
    typedef unsigned int Attributes(const char *);
    typedef int Create(const char *, char *, void *, void *, int, unsigned int, void *,
                       const char *, struct __c1_startup *, struct __c1_process *);
    typedef unsigned int Wait(void *, unsigned int);
    typedef int ExitCode(void *, unsigned int *);
    Attributes *attributes = *(Attributes **)0x401148;
    Create *create = *(Create **)0x401158;
    Wait *wait = *(Wait **)0x401160;
    ExitCode *exit_code = *(ExitCode **)0x401168;
    __c1_file_close *close = *(__c1_file_close **)0x401068;
    __c1_file_standard *standard = *(__c1_file_standard **)0x401040;
    struct __c1_startup startup;
    struct __c1_process child;
    char *path, *line;
    size_t path_size, command_size;
    unsigned int flags, status;
    int result;
    path = __c1_command_processor();
    if (!path)
        return command ? -1 : 0;
    if (!command) {
        flags = attributes(path);
        free(path);
        return flags != 0xffffffffU && !(flags & 16);
    }
    path_size = strlen(path);
    command_size = strlen(command);
    if (command_size > 32767 || path_size + command_size + 16 > 32767) {
        free(path);
        errno = EINVAL;
        return -1;
    }
    line = malloc(path_size + command_size + 16);
    if (!line) {
        free(path);
        return -1;
    }
    line[0] = '"';
    memcpy(line + 1, path, path_size);
    memcpy(line + path_size + 1, "\" /d /s /c \"", 12);
    memcpy(line + path_size + 13, command, command_size);
    line[path_size + command_size + 13] = '"';
    line[path_size + command_size + 14] = 0;
    memset(&startup, 0, sizeof(startup));
    memset(&child, 0, sizeof(child));
    startup.size = sizeof(startup);
    startup.flags = 0x101;
    startup.show = 0;
    startup.input = standard(-10);
    startup.output = standard(-11);
    startup.error = standard(-12);
    result = create(path, line, NULL, NULL, 1, 0, NULL, NULL, &startup, &child);
    if (!result)
        __c1_file_failure(NULL);
    free(line);
    free(path);
    if (!result)
        return -1;
    close(child.thread);
    if (wait(child.process, 0xffffffffU) != 0) {
        __c1_file_failure(NULL);
        close(child.process);
        return -1;
    }
    result = exit_code(child.process, &status);
    if (!result)
        __c1_file_failure(NULL);
    close(child.process);
    return result ? (int)status : -1;
}
#endif
