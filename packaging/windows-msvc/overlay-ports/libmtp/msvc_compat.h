#ifndef MSVC_COMPAT_H_GUARD
#define MSVC_COMPAT_H_GUARD

#include <stdint.h>
#include <basetsd.h>
#include <winsock2.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>
#include <malloc.h> // Required for native Win32 _alloca stack pool processing

typedef SSIZE_T ssize_t;

static __inline char* strndup(const char* s, size_t n) {
    size_t len = strlen(s);
    if (len > n) len = n;
    char* new_str = (char*)malloc(len + 1);
    if (!new_str) return (void*)0;
    new_str[len] = '\0';
    return (char*)memcpy(new_str, s, len);
}

#ifndef GETTIMEOFDAY_MSVC_GUARD
#define GETTIMEOFDAY_MSVC_GUARD
static __inline int gettimeofday(struct timeval* tv, void* tz) {
    union { long long ns100; FILETIME ft; } now;
    GetSystemTimeAsFileTime(&now.ft);
    tv->tv_usec = (long)((now.ns100 / 10LL) % 1000000LL);
    tv->tv_sec = (long)((now.ns100 - 116444736000000000LL) / 10000000LL);
    return 0;
}
#endif

#endif
