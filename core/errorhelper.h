// SPDX-License-Identifier: GPL-2.0
#ifndef ERROR_HELPER_H
#define ERROR_HELPER_H

// error reporting functions

#include <string>

#ifdef __GNUC__
#define __printf(x, y) __attribute__((__format__(__printf__, x, y)))
#else
#define __printf(x, y)
#endif

extern int verbose;
extern int __printf(1, 2) report_error(const char *fmt, ...);
extern void __printf(1, 2) report_info(const char *fmt, ...);
extern void set_error_cb(void(*cb)(std::string s));	// Callback takes ownership of passed string

#endif
