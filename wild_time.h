#ifndef WILD_TIME_H_INCLUDED
#define WILD_TIME_H_INCLUDED

#include <time.h>
#include <list>

#define WILE_TIME_NOT_SET         0x0
#define WILD_TIME_SECOND_STAR     0x1
#define WILD_TIME_MINUTE_STAR     0x2
#define WILD_TIME_HOUR_STAR       0x4
#define WILD_TIME_DAY_STAR        0x8
#define WILD_TIME_MONTH_STAR      0x10
#define WILD_TIME_YEAR_STAR       0x20

#define MAX_TIME    0x7FFFFFFF

typedef struct
{
    int start;
    int end;
} time_range_t;

typedef struct {
    time_range_t year;
    time_range_t mon;
    time_range_t mday;
    time_range_t hour;
    time_range_t min;
    time_range_t sec;
} wild_time_t;

void wild_time_set_fixed_time(wild_time_t *time, time_t value);
void parse_wild_time_string(wild_time_t *time, const char *s);
time_t wild_time_get_recent_time(wild_time_t *wild_time, time_t current);
time_t wild_time_get_next_time(const wild_time_t *time, time_t start);
time_t wild_time_list_get_next_time(const std::list<wild_time_t> &time_list,
                                    time_t start);
time_t wild_time_list_get_approx_period(const std::list<wild_time_t> &time_list);
void format_wild_time(const wild_time_t *wild_time, char *buf);

#endif // WILD_TIME_H_INCLUDED
