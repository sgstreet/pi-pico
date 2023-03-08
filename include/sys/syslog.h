#ifndef _SYSLOG_H_
#define _SYSLOG_H_

#include <compiler.h>

#define SYSLOG_NONE 0
#define SYSLOG_FATAL 1
#define SYSLOG_ERROR 2
#define SYSLOG_WARN 3
#define SYSLOG_INFO 4
#define SYSLOG_DEBUG 5
#define SYSLOG_TRACE 6

#define SYSLOG_BACKTRACE_SIZE 25

extern __attribute__((format(printf, 2, 3))) void syslog(int level, const char *fmt, ...);
extern __noreturn __attribute__((format(printf, 1, 2))) void syslog_fatal_abort(const char *fmt, ...);

#ifndef NDEBUG

#ifndef SYSLOG_LEVEL
	#define SYSLOG_LEVEL SYSLOG_TRACE
#endif

#else

#ifndef SYSLOG_LEVEL
	#define SYSLOG_LEVEL SYSLOG_INFO
#endif

#endif

#if LOG_LEVEL >= LOG_FATAL
	#define syslog_fatal(FMT, ...) do { syslog_fatal_abort(FMT, ##__VA_ARGS__); } while (0)
#else
	#define syslog_fatal(FMT, ...) do {} while (0)
#endif

#if LOG_LEVEL >= LOG_ERROR
	#define syslog_error(FMT, ...) syslog(SYSLOG_ERROR, FMT, ##__VA_ARGS__)
#else
	#define syslog_error(FMT, ...) do {} while (0)
#endif

#if LOG_LEVEL >= LOG_WARN
	#define syslog_warn(FMT, ...) syslog(SYSLOG_WARN, FMT, ##__VA_ARGS__)
#else
	#define syslog_warn(FMT, ...) do {} while (0)
#endif

#if LOG_LEVEL >= LOG_INFO
	#define syslog_info(FMT, ...) syslog(SYSLOG_INFO, FMT, ##__VA_ARGS__)
#else
	#define syslog_info(FMT, ...) do {} while (0)
#endif

#if LOG_LEVEL >= LOG_DEBUG
	#define syslog_debug(FMT, ...) syslog(SYSLOG_DEBUG, FMT, ##__VA_ARGS__)
#else
	#define syslog_debug(FMT, ...) do {} while (0)
#endif

#if LOG_LEVEL >= LOG_TRACE
	#define syslog_trace(FMT, ...) syslog(SYSLOG_TRACE, FMT, ##__VA_ARGS__)
#else
	#define syslog_trace(FMT, ...) do {} while (0)
#endif

#endif
