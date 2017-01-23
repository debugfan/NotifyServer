/*
 *          Copyright Andrey Semashev 2007 - 2015.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
#include "log.h"
#include <boost/date_time/posix_time/posix_time_types.hpp>
#include <boost/log/core.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/sinks/text_file_backend.hpp>
#include <boost/log/utility/setup/file.hpp>
#include <boost/log/utility/setup/console.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/sources/severity_logger.hpp>
#include <boost/log/sources/record_ostream.hpp>
#include <boost/log/support/date_time.hpp>

namespace logging = boost::log;
namespace src = boost::log::sources;
namespace sinks = boost::log::sinks;
namespace keywords = boost::log::keywords;
namespace expr = boost::log::expressions;

src::severity_logger<logging::trivial::severity_level> g_logger;

void init_boost_log()
{
#define LOG_FORMAT_STRING "[%TimeStamp%]: %Message%"

    logging::add_console_log(std::cout,
#if defined(USE_SIMPLE_LOG_FORMAT)
                            keywords::format = LOG_FORMAT_STRING
#else
                            keywords::format =
                            (
                                expr::stream
                                    << expr::format_date_time<boost::posix_time::ptime>("TimeStamp",
                                                                                        "[%Y-%m-%d %H:%M:%S]")
                                    << ": " << expr::smessage
                            )
#endif // USE_SIMPLE_TIME_FORMAT
                             );
    logging::add_file_log
    (
        /*< file name pattern >*/
        keywords::file_name = "NotifyServer_%N.log",
        /*< rotate files every 10 MiB... >*/
        keywords::rotation_size = 10 * 1024 * 1024,
        /*< ...or at midnight >*/
        keywords::time_based_rotation = sinks::file::rotation_at_time_point(0, 0, 0),
        /*< log record format >*/
#if defined(USE_SIMPLE_LOG_FORMAT)
        keywords::format = LOG_FORMAT_STRING,
#else
        keywords::format =
        (
            expr::stream
                << expr::format_date_time<boost::posix_time::ptime>("TimeStamp",
                                                                    "[%Y-%m-%d %H:%M:%S]")
                << ": " << expr::smessage
        ),
#endif // defined
        // other args
        keywords::open_mode = (std::ios::out | std::ios::app),
        keywords::auto_flush = true
    );

    logging::core::get()->set_filter
    (
        logging::trivial::severity >= logging::trivial::info
    );

    logging::add_common_attributes();
}

logging::trivial::severity_level map_log_level(FILE *fp)
{
    logging::trivial::severity_level r;
    if(stdout == fp)
    {
        r = logging::trivial::info;
    }
    else
    {
        r = logging::trivial::error;
    }
    return r;
}

#ifdef USE_BOOST_LOG
logging::trivial::severity_level map_log_level(int level)
{
    logging::trivial::severity_level r;
    switch(level)
    {
    case LOG_LEVEL_TRACE:
        r = logging::trivial::trace;
        break;
    case LOG_LEVEL_DEBUG:
        r = logging::trivial::debug;
        break;
    case LOG_LEVEL_INFO:
        r = logging::trivial::info;
        break;
    case LOG_LEVEL_WARNING:
        r = logging::trivial::warning;
        break;
    case LOG_LEVEL_ERROR:
        r = logging::trivial::error;
        break;
    case LOG_LEVEL_FATAL:
        r = logging::trivial::fatal;
        break;
    default:
        r = logging::trivial::error;
        break;
    }
    return r;
}
#endif // HAVE_FILE_LOG

#ifdef USE_BOOST_LOG
int boost_log_printf(int level, const char *fmt, ...)
#else
int boost_log_printf(FILE *level, const char *fmt, ...)
#endif
{
    int cnt;
    char buf[1024];
    va_list argptr;

    va_start(argptr, fmt);
    cnt = vsprintf(buf, fmt, argptr);
    va_end(argptr);
    for(int off = cnt > 0 ? cnt - 1 : -1;
        off > 0 && off < (int)sizeof(buf);
        off--)
    {
        if(buf[off] == '\r' || buf[off] == '\n')
        {
            buf[off] = '\0';
        }
        else
        {
            break;
        }
    }
    BOOST_LOG_SEV(g_logger, map_log_level(level)) << buf;
    return cnt;
}
