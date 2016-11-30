#ifndef LOG_H_INCLUDED
#define LOG_H_INCLUDED

enum LOG_LEVEL
{
    LOG_LEVEL_TRACE,
    LOG_LEVEL_DEBUG,
    LOG_LEVEL_INFO,
    LOG_LEVEL_WARNING,
    LOG_LEVEL_ERROR,
    LOG_LEVEL_FATAL
};

void init_log();
int log_printf(int level, const char *fmt, ...);

#endif // LOG_H_INCLUDED
