#ifndef LOG_H_INCLUDED
#define LOG_H_INCLUDED

#define USE_BOOST_LOG

#ifdef USE_BOOST_LOG
enum LOG_LEVEL
{
    LOG_LEVEL_TRACE,
    LOG_LEVEL_DEBUG,
    LOG_LEVEL_INFO,
    LOG_LEVEL_WARNING,
    LOG_LEVEL_ERROR,
    LOG_LEVEL_FATAL
};

void init_boost_log();
int boost_log_printf(int level, const char *fmt, ...);

#define init_log    init_boost_log
#define log_printf  boost_log_printf
#else

#include <stdio.h>

#define init_log()
#define log_printf fprintf

#define LOG_LEVEL_TRACE     stdout
#define LOG_LEVEL_DEBUG     stdout
#define LOG_LEVEL_INFO      stdout
#define LOG_LEVEL_WARNING   stdout
#define LOG_LEVEL_ERROR     stderr
#define LOG_LEVEL_FATAL     stderr

#endif // HAVE_FILE_LOG

#endif // LOG_H_INCLUDED
