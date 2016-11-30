/*
 *          Copyright Andrey Semashev 2007 - 2015.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
#include "log.h"
#include <boost/log/core.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/sinks/text_file_backend.hpp>
#include <boost/log/utility/setup/file.hpp>
#include <boost/log/utility/setup/console.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/sources/severity_logger.hpp>
#include <boost/log/sources/record_ostream.hpp>

namespace logging = boost::log;
namespace src = boost::log::sources;
namespace sinks = boost::log::sinks;
namespace keywords = boost::log::keywords;

src::severity_logger<logging::trivial::severity_level> g_logger;

void init_log()
{
#define LOG_FORMAT_STRING "[%TimeStamp%]: %Message%"

    logging::add_console_log(std::cout,
                             keywords::format = LOG_FORMAT_STRING);
    logging::add_file_log
    (
        /*< file name pattern >*/
        keywords::file_name = "NotifyServer_%N.log",
        /*< rotate files every 10 MiB... >*/
        keywords::rotation_size = 10 * 1024 * 1024,
        /*< ...or at midnight >*/
        keywords::time_based_rotation = sinks::file::rotation_at_time_point(0, 0, 0),
        /*< log record format >*/
        keywords::format = LOG_FORMAT_STRING,
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

int log_printf(int level, const char *fmt, ...)
{
    int cnt;
    char buf[1024];
    va_list argptr;

    va_start(argptr, fmt);
    cnt = vsprintf(buf, fmt, argptr);
    va_end(argptr);
    BOOST_LOG_SEV(g_logger, map_log_level(level)) << buf;
    return cnt;
}

