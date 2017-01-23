#include "wild_time.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "string_utils.h"
#include "minmax.h"

#define GUARANTEE_TYPE_NONE         0
#define GUARANTEE_TYPE_POSITIVE     1
#define GUARANTEE_TYPE_NEGATIVE     -1

#define MAX_TIME_YEAR   (2038 - 1900)

void wild_time_set_fixed_time(wild_time_t *time, time_t value)
{
    struct tm tm_local = *(localtime(&value));
    time->year.start = time->year.end = tm_local.tm_year;
    time->mon.start = time->mon.end = tm_local.tm_mon;
    time->mday.start = time->mday.end = tm_local.tm_mday;
    time->hour.start = time->hour.end = tm_local.tm_hour;
    time->min.start = time->min.end = tm_local.tm_min;
    time->sec.start = time->sec.end = tm_local.tm_sec;
}

bool is_blank(char c)
{
    if(c == ' ' || c == '\t')
    {
        return true;
    }
    else
    {
        return false;
    }
}

int get_closest_valid_value(int val, int v_min, int v_max)
{
    if(val <= v_min)
    {
        return v_min;
    }
    else if(val >= v_max)
    {
        return v_max;
    }
    else
    {
        return val;
    }
}

void parse_origin_time_range(time_range_t *range,
                             const char *start,
                             int len)
{
    char buf[64];
    const char *p;
    if(len >= 0)
    {
        memset(buf, 0, sizeof(buf));
        strncpy(buf, start, len);
        p = buf;
    }
    else
    {
        p = start;
    }

    p = skip_blank(p);
    if(*p == '\0')
    {
        return;
    }
    if(*p == '*')
    {
        range->start = 0;
        range->end = 0x7fffffff;
        return;
    }
    const char *e = strchr(p, '_');
    if(e == NULL)
    {
        range->start = range->end = atoi(p);
    }
    else
    {
        range->start = atoi_n(p, e - p);
        range->end = atoi(e + 1);
    }
}

void move_time_range(time_range_t *range,
                     int delta,
                     int range_min,
                     int range_max)
{
    range->start = range->start + delta;
    range->end = range->end + delta;

    if(range->start > range_max)
    {
        range->start = range_max;
    }
    if(range->start < range_min)
    {
        range->start = range_min;
    }

    if(range->end < range_min)
    {
        range->end = range_min;
    }
    if(range->end > range_max)
    {
        range->end = range_max;
    }
}

void parse_wild_day_time_string(wild_time_t *time, const char *s)
{
    const char *p;
    const char *e;
    int n;
    time_range_t range;

    p = skip_blank(s);
    e = strchr(p, ':');
    n = (e != NULL ? e - p : -1);
    parse_origin_time_range(&range, p, n);
    move_time_range(&range, 0, 0, 23);
    time->hour = range;
    if(e == NULL)
    {
        return;
    }
    p = e+1;

    p = skip_blank(p);
    e = strchr(p, ':');
    n = (e != NULL ? e - p : -1);
    parse_origin_time_range(&range, p, n);
    move_time_range(&range, 0, 0, 59);
    time->min = range;
    if(e == NULL)
    {
        return;
    }
    p = e+1;

    parse_origin_time_range(&range, p, -1);
    move_time_range(&range, 0, 0, 59);
    time->sec = range;
}

void parse_wild_time_string(wild_time_t *time, const char *s)
{
    const char *p;
    char *e;
    int n;
    time_range_t range;

    memset(time, 0, sizeof(wild_time_t));

    p = skip_blank(s);
    e = strchr(p, '-');
    n = (e != NULL ? e - p : -1);
    parse_origin_time_range(&range, p, n);
    move_time_range(&range, -1900, 0, MAX_TIME_YEAR);
    time->year = range;
    if(e == NULL)
    {
        return;
    }
    p = e+1;

    p = skip_blank(p);
    e = strchr(p, '-');
    n = (e != NULL ? e - p : -1);
    parse_origin_time_range(&range, p, n);
    move_time_range(&range, -1, 0, 11);
    time->mon = range;
    if(e == NULL)
    {
        return;
    }
    p = e+1;

    p = skip_blank(p);
    e = strchr(p, ' ');
    n = (e != NULL ? e - p : -1);
    parse_origin_time_range(&range, p, n);
    move_time_range(&range, 0, 1, 31);
    time->mday = range;
    if(e == NULL)
    {
        return;
    }
    p = e+1;

    parse_wild_day_time_string(time, p);
}

int get_max_mday(int year, int month)
{
    int r;
    switch(month%12)
    {
    case 0:
    case 2:
    case 4:
    case 6:
    case 7:
    case 9:
    case 11:
        r = 31;
        break;
    case 3:
    case 5:
    case 8:
    case 10:
        r = 30;
        break;
    case 1:
        r = 28;
        break;
    default:
        r = 28;
        break;
    }
    return r;
}

void tm_get_recent_time(struct tm *recent,
                        const struct tm *tm_adj,
                        unsigned int tm_flags,
                        time_t start)
{
    int guarantee = GUARANTEE_TYPE_NONE;
    struct tm tm_start = *(localtime(&start));

    if(!(tm_flags & WILD_TIME_YEAR_STAR))
    {
        recent->tm_year = tm_adj->tm_year;
        if(guarantee == GUARANTEE_TYPE_NONE)
        {
            if(recent->tm_year > tm_start.tm_year)
            {
                guarantee = GUARANTEE_TYPE_POSITIVE;
            }
            else if(recent->tm_year < tm_start.tm_year)
            {
                guarantee = GUARANTEE_TYPE_NEGATIVE;
            }
        }
    }
    else
    {
        recent->tm_year = tm_start.tm_year;
    }

    if(!(tm_flags & WILD_TIME_MONTH_STAR))
    {
        recent->tm_mon = tm_adj->tm_mon;
        if(guarantee == GUARANTEE_TYPE_NONE)
        {
            if(recent->tm_mon > tm_start.tm_mon)
            {
                guarantee = GUARANTEE_TYPE_POSITIVE;
            }
            else if(recent->tm_mon < tm_start.tm_mon)
            {
                guarantee = GUARANTEE_TYPE_NEGATIVE;
            }
        }
    }
    else
    {
        if(guarantee == GUARANTEE_TYPE_POSITIVE)
        {
            recent->tm_mon = 0;
        }
        else if(guarantee == GUARANTEE_TYPE_NEGATIVE)
        {
            recent->tm_mon = 11;
        }
        else
        {
            recent->tm_mon = tm_start.tm_mon;
        }
    }

    if(!(tm_flags & WILD_TIME_DAY_STAR))
    {
        recent->tm_mday = tm_adj->tm_mday;
        if(guarantee == GUARANTEE_TYPE_NONE)
        {
            if(recent->tm_mday > tm_start.tm_mday)
            {
                guarantee = GUARANTEE_TYPE_POSITIVE;
            }
            else if(recent->tm_mday < tm_start.tm_mday)
            {
                guarantee = GUARANTEE_TYPE_NEGATIVE;
            }
        }
    }
    else
    {
        if(guarantee == GUARANTEE_TYPE_POSITIVE)
        {
            recent->tm_mday = 1;
        }
        else if(guarantee == GUARANTEE_TYPE_NEGATIVE)
        {
            recent->tm_mday = get_max_mday(recent->tm_year,
                                           recent->tm_mon);
        }
        else
        {
            recent->tm_mday = tm_start.tm_mday;
        }
    }

    if(!(tm_flags & WILD_TIME_HOUR_STAR))
    {
        recent->tm_hour = tm_adj->tm_hour;
        if(guarantee == GUARANTEE_TYPE_NONE)
        {
            if(recent->tm_hour > tm_start.tm_hour)
            {
                guarantee = GUARANTEE_TYPE_POSITIVE;
            }
            else if(recent->tm_hour < tm_start.tm_hour)
            {
                guarantee = GUARANTEE_TYPE_NEGATIVE;
            }
        }
    }
    else
    {
        if(guarantee == GUARANTEE_TYPE_POSITIVE)
        {
            recent->tm_hour = 0;
        }
        else if(guarantee == GUARANTEE_TYPE_NEGATIVE)
        {
            recent->tm_hour = 23;
        }
        else
        {
            recent->tm_hour = tm_start.tm_hour;
        }
    }

    if(!(tm_flags & WILD_TIME_MINUTE_STAR))
    {
        recent->tm_min = tm_adj->tm_min;
        if(guarantee == GUARANTEE_TYPE_NONE)
        {
            if(recent->tm_min > tm_start.tm_min)
            {
                guarantee = GUARANTEE_TYPE_POSITIVE;
            }
            else if(recent->tm_min < tm_start.tm_min)
            {
                guarantee = GUARANTEE_TYPE_NEGATIVE;
            }
        }
    }
    else
    {
        if(guarantee == GUARANTEE_TYPE_POSITIVE)
        {
            recent->tm_min = 0;
        }
        else if(guarantee == GUARANTEE_TYPE_NEGATIVE)
        {
            recent->tm_min = 59;
        }
        else
        {
            recent->tm_min = tm_start.tm_min;
        }
    }

    if(!(tm_flags & WILD_TIME_SECOND_STAR))
    {
        recent->tm_sec = tm_adj->tm_sec;
    }
    else
    {
        if(guarantee == GUARANTEE_TYPE_POSITIVE)
        {
            recent->tm_sec = 0;
        }
        else if(guarantee == GUARANTEE_TYPE_NEGATIVE)
        {
            recent->tm_sec = 59;
        }
        else
        {
            recent->tm_sec = tm_start.tm_sec;
        }
    }
}

bool tm_day_is_valid(struct tm *time, int delta)
{
    struct tm tm_tmp;
    time_t tt_new;
    struct tm tm_new;

    if((time->tm_mon != 1
       && time->tm_mday + delta <= 30)
       || (time->tm_mday == 1
           && time->tm_mday + delta <= 28))
    {
        return true;
    }

    tm_tmp = *time;
    tm_tmp.tm_mday += delta;
    tt_new = mktime(&tm_tmp);
    tm_new = *(localtime(&tt_new));
    if(tm_new.tm_mday == time->tm_mday + delta)
    {
        return true;
    }
    else
    {
        return false;
    }
}

bool tm_get_next_time(struct tm *next,
                      const struct tm *start,
                      unsigned int time_flags)
{
    *next = *start;
    do
    {
        if(time_flags & WILD_TIME_SECOND_STAR)
        {
            if(next->tm_sec < 59)
            {
                next->tm_sec++;
                break;
            }
            else
            {
                next->tm_sec = 0;
            }
        }

        if(time_flags & WILD_TIME_MINUTE_STAR)
        {
            if(next->tm_min < 59)
            {
                next->tm_min++;
                break;
            }
            else
            {
                next->tm_min = 0;
            }
        }

        if(time_flags & WILD_TIME_HOUR_STAR)
        {
            if(next->tm_hour < 23)
            {
                next->tm_hour++;
                break;
            }
            else
            {
                next->tm_hour = 0;
            }
        }

        if(time_flags & WILD_TIME_DAY_STAR)
        {
            if(next->tm_mday < 28)
            {
                next->tm_mday++;
                break;
            }
            else if(next->tm_mday >= 31)
            {
                next->tm_mday = 1;
            }
            else
            {
                if(tm_day_is_valid(next, 1))
                {
                    next->tm_mday++;
                    break;
                }
                else
                {
                    next->tm_mday = 1;
                }
            }
        }

        if(time_flags & WILD_TIME_MONTH_STAR)
        {
            bool done = false;
            for(int i = 0; i < 2; i++)
            {
                if(next->tm_mon < 11)
                {
                    next->tm_mon++;
                    if(tm_day_is_valid(next, 0))
                    {
                        done = true;
                        break;
                    }
                }
                else
                {
                    next->tm_mon = 0;
                    break;
                }
            }
            if(done)
            {
                break;
            }
        }

        if(time_flags & WILD_TIME_YEAR_STAR)
        {
            next->tm_year++;
            if(!tm_day_is_valid(next, 0))
            {
                next->tm_year++;
            }
            break;
        }

        *next = *start;
        return false;
    }
    while(false);
    return true;
}

time_t tm_get_period(const struct tm *start, unsigned int time_flags)
{
    time_t period = 0;

    do
    {
        if(time_flags & WILD_TIME_SECOND_STAR)
        {
            period = 1;
            break;
        }

        if(time_flags & WILD_TIME_MINUTE_STAR)
        {
            period = 60;
            break;
        }

        if(time_flags & WILD_TIME_HOUR_STAR)
        {
            period = 60 * 60;
            break;
        }

        if(time_flags & WILD_TIME_DAY_STAR)
        {
            period = 60 * 60 * 24;
            break;
        }

        if(time_flags & WILD_TIME_MONTH_STAR)
        {
            period = 60 * 60 * 24 * 31;
            break;
        }

        if(time_flags & WILD_TIME_YEAR_STAR)
        {
            period = 60 * 60 * 24 * 366;
            break;
        }

        period = 0x7fffffff;
    }
    while(false);

    return period;
}

time_t wild_time_get_period(const wild_time_t *wild_time)
{
    time_t period = 0;

    do
    {
        if(wild_time->sec.end > wild_time->sec.start)
        {
            period = 1;
            break;
        }

        if(wild_time->min.end > wild_time->min.start)
        {
            period = 60;
            break;
        }

        if(wild_time->hour.end > wild_time->hour.start)
        {
            period = 60 * 60;
            break;
        }

        if(wild_time->mday.end > wild_time->mday.start)
        {
            period = 60 * 60 * 24;
            break;
        }

        if(wild_time->mon.end > wild_time->mon.start)
        {
            period = 60 * 60 * 24 * 31;
            break;
        }

        if(wild_time->year.end > wild_time->year.start)
        {
            period = 60 * 60 * 24 * 366;
            break;
        }

        period = 0x7fffffff;
    }
    while(false);

    return period;
}

bool wild_time_get_next_tm_time(struct tm *next,
                      const struct tm *start,
                      const wild_time_t *wild_time)
{
    *next = *start;
    do
    {
        if(next->tm_sec < wild_time->sec.end)
        {
            next->tm_sec++;
            break;
        }
        else
        {
            next->tm_sec = wild_time->sec.start;
        }

        if(next->tm_min < wild_time->min.end)
        {
            next->tm_min++;
            break;
        }
        else
        {
            next->tm_min = wild_time->min.start;
        }

        if(next->tm_hour < wild_time->hour.end)
        {
            next->tm_hour++;
            break;
        }
        else
        {
            next->tm_hour = wild_time->hour.start;
        }

        if(next->tm_mday < wild_time->mday.end)
        {
            if(next->tm_mday < 28)
            {
                next->tm_mday++;
                break;
            }
            else if(next->tm_mday >= 31)
            {
                next->tm_mday = wild_time->mday.start;
            }
            else
            {
                if(tm_day_is_valid(next, 1))
                {
                    next->tm_mday++;
                    break;
                }
                else
                {
                    next->tm_mday = wild_time->mday.start;
                }
            }
        }
        else
        {
            next->tm_mday = wild_time->mday.start;
        }

        bool done = false;
        for(int i = 0; i < 2; i++)
        {
            if(next->tm_mon < wild_time->mon.end)
            {
                next->tm_mon++;
                if(tm_day_is_valid(next, 0))
                {
                    done = true;
                    break;
                }
            }
            else
            {
                next->tm_mon = wild_time->mon.start;
                break;
            }
        }
        if(done)
        {
            break;
        }

        if(next->tm_year < wild_time->year.end)
        {
            next->tm_year++;
            if(!tm_day_is_valid(next, 0))
            {
                if(next->tm_year < wild_time->year.end)
                {
                    next->tm_year++;
                    break;
                }
            }
            else
            {
                break;
            }
        }

        *next = *start;
        return false;
    }
    while(false);
    return true;
}

void wild_time_get_recent_tm_time(struct tm *tm_recent,
                                    const wild_time_t *wild_time,
                                    time_t current)
{
    struct tm tm_start;
    int guarantee;

    guarantee = GUARANTEE_TYPE_NONE;
    tm_start = *(localtime(&current));

    if(tm_start.tm_year < wild_time->year.start)
    {
        tm_recent->tm_year = wild_time->year.start;
        guarantee = GUARANTEE_TYPE_POSITIVE;
    }
    else if(tm_start.tm_year > wild_time->year.end)
    {
        tm_recent->tm_year = wild_time->year.end;
        guarantee = GUARANTEE_TYPE_NEGATIVE;
    }
    else
    {
        tm_recent->tm_year = tm_start.tm_year;
    }

    if(guarantee == GUARANTEE_TYPE_POSITIVE)
    {
        tm_recent->tm_mon = wild_time->mon.start;
    }
    else if(guarantee == GUARANTEE_TYPE_NEGATIVE)
    {
        tm_recent->tm_mon = wild_time->mon.end;
    }
    else
    {
        if(tm_start.tm_mon < wild_time->mon.start)
        {
            tm_recent->tm_mon = wild_time->mon.start;
            guarantee = GUARANTEE_TYPE_POSITIVE;
        }
        else if(tm_start.tm_mon > wild_time->mon.end)
        {
            tm_recent->tm_mon = wild_time->mon.end;
            guarantee = GUARANTEE_TYPE_NEGATIVE;
        }
        else
        {
            tm_recent->tm_mon = tm_start.tm_mon;
        }
    }

    int max_mday = get_max_mday(tm_recent->tm_year,
                                tm_recent->tm_mon);
    if(guarantee == GUARANTEE_TYPE_POSITIVE)
    {
        tm_recent->tm_mday = wild_time->mday.start;
    }
    else if(guarantee == GUARANTEE_TYPE_NEGATIVE)
    {
        tm_recent->tm_mday = MIN(max_mday,
                                 wild_time->mday.end);
    }
    else
    {
        if(tm_start.tm_mday < wild_time->mday.start)
        {
            tm_recent->tm_mday = wild_time->mday.start;
            guarantee = GUARANTEE_TYPE_POSITIVE;
        }
        else if(tm_start.tm_mday > wild_time->mday.end)
        {
            tm_recent->tm_mday = MIN(max_mday,
                                     wild_time->mday.end);
            guarantee = GUARANTEE_TYPE_NEGATIVE;
        }
        else
        {
            if(tm_start.tm_mday > max_mday)
            {
                tm_recent->tm_mday = max_mday;
                guarantee = GUARANTEE_TYPE_NEGATIVE;
            }
            else
            {
                tm_recent->tm_mday = tm_start.tm_mday;
            }
        }
    }

    if(guarantee == GUARANTEE_TYPE_POSITIVE)
    {
        tm_recent->tm_hour = wild_time->hour.start;
    }
    else if(guarantee == GUARANTEE_TYPE_NEGATIVE)
    {
        tm_recent->tm_hour = wild_time->hour.end;
    }
    else
    {
        if(tm_start.tm_hour < wild_time->hour.start)
        {
            tm_recent->tm_hour = wild_time->hour.start;
            guarantee = GUARANTEE_TYPE_POSITIVE;
        }
        else if(tm_start.tm_hour > wild_time->hour.end)
        {
            tm_recent->tm_hour = wild_time->hour.end;
            guarantee = GUARANTEE_TYPE_NEGATIVE;
        }
        else
        {
            tm_recent->tm_hour = tm_start.tm_hour;
        }
    }

    if(guarantee == GUARANTEE_TYPE_POSITIVE)
    {
        tm_recent->tm_min = wild_time->min.start;
    }
    else if(guarantee == GUARANTEE_TYPE_NEGATIVE)
    {
        tm_recent->tm_min = wild_time->min.end;
    }
    else
    {
        if(tm_start.tm_min < wild_time->min.start)
        {
            tm_recent->tm_min = wild_time->min.start;
            guarantee = GUARANTEE_TYPE_POSITIVE;
        }
        else if(tm_start.tm_min > wild_time->min.end)
        {
            tm_recent->tm_min = wild_time->min.end;
            guarantee = GUARANTEE_TYPE_NEGATIVE;
        }
        else
        {
            tm_recent->tm_min = tm_start.tm_min;
        }
    }

    if(guarantee == GUARANTEE_TYPE_POSITIVE)
    {
        tm_recent->tm_sec = wild_time->sec.start;
    }
    else if(guarantee == GUARANTEE_TYPE_NEGATIVE)
    {
        tm_recent->tm_sec = wild_time->sec.end;
    }
    else
    {
        if(tm_start.tm_sec < wild_time->sec.start)
        {
            tm_recent->tm_sec = wild_time->sec.start;
            guarantee = GUARANTEE_TYPE_POSITIVE;
        }
        else if(tm_start.tm_sec > wild_time->sec.end)
        {
            tm_recent->tm_sec = wild_time->sec.end;
            guarantee = GUARANTEE_TYPE_NEGATIVE;
        }
        else
        {
            tm_recent->tm_sec = tm_start.tm_sec;
        }
    }
}

time_t wild_time_get_recent_time(wild_time_t *wild_time, time_t current)
{
    struct tm tm_recent;
    struct tm tm_next;

    wild_time_get_recent_tm_time(&tm_recent, wild_time, current);
    if(tm_day_is_valid(&tm_recent, 0))
    {
        return mktime(&tm_recent);
    }
    else
    {
        wild_time_get_next_tm_time(&tm_next,
            &tm_recent,
            wild_time);
        return mktime(&tm_next);
    }
}

time_t wild_time_get_next_time(const wild_time_t *wild_time, time_t start)
{
    time_t next;
    struct tm tm_recent;
    struct tm tm_next;

    wild_time_get_recent_tm_time(&tm_recent, wild_time, start);
    next = mktime(&tm_recent);
    if(!tm_day_is_valid(&tm_recent, 0) || next <= start)
    {
        wild_time_get_next_tm_time(&tm_next,
                                         &tm_recent,
                                         wild_time);
        next = mktime(&tm_next);
    }
    return next;
}

time_t wild_time_list_get_next_time(const std::list<wild_time_t> &time_list,
                                    time_t start)
{
    time_t next = MAX_TIME;
    std::list<wild_time_t>::const_iterator iter;
    for(iter = time_list.begin();
        iter != time_list.end();
        iter++)
    {
        time_t tmp_next = wild_time_get_next_time(&(*iter), start);
        if(tmp_next > start && tmp_next < MAX_TIME
           && tmp_next < next)
        {
            next = tmp_next;
        }
    }
    return next;
}

time_t wild_time_list_get_approx_period(const std::list<wild_time_t> &time_list)
{
    time_t period = 0x7fffffff;
    std::list<wild_time_t>::const_iterator iter;
    for(iter = time_list.begin();
        iter != time_list.end();
        iter++)
    {
        time_t tmp = wild_time_get_period(&(*iter));
        if(period > tmp)
        {
            period = tmp;
        }
    }
    return period;
}

void format_wild_time(const wild_time_t *wild_time, char *buf)
{
    char tmp[16];

    if(wild_time->year.end > wild_time->year.start)
    {
        if(wild_time->year.start <= 0
           && wild_time->year.end >= MAX_TIME_YEAR)
        {
            strcpy(tmp, "*");
        }
        else
        {
            sprintf(tmp, "%04d_%04d",
                    wild_time->year.start + 1900,
                    wild_time->year.end + 1900);
        }
    }
    else
    {
        sprintf(tmp, "%04d", wild_time->year.start + 1900);
    }
    strcpy(buf, tmp);

    if(wild_time->mon.end > wild_time->mon.start)
    {
        if(wild_time->mon.start <= 0
           && wild_time->mon.end >= 11)
        {
            strcpy(tmp, "-*");
        }
        else
        {
            sprintf(tmp, "-%02d_%02d",
                    wild_time->mon.start + 1,
                    wild_time->mon.end + 1);
        }
    }
    else
    {
        sprintf(tmp, "-%02d", wild_time->mon.start + 1);
    }
    strcat(buf, tmp);

    if(wild_time->mday.end > wild_time->mday.start)
    {
        if(wild_time->mday.start <= 1
           && wild_time->mday.end >= 31)
        {
            strcpy(tmp, "-*");
        }
        else
        {
            sprintf(tmp, "-%02d_%02d",
                    wild_time->mday.start,
                    wild_time->mday.end);
        }
    }
    else
    {
        sprintf(tmp, "-%02d", wild_time->mday.start);
    }
    strcat(buf, tmp);

    strcat(buf, " ");
    if(wild_time->hour.end > wild_time->hour.start)
    {
        if(wild_time->hour.start <= 0
           && wild_time->hour.end >= 59)
        {
            strcpy(tmp, "*");
        }
        else
        {
        sprintf(tmp, "%02d_%02d",
                wild_time->hour.start,
                wild_time->hour.end);
        }
    }
    else
    {
        sprintf(tmp, "%02d", wild_time->hour.start);
    }
    strcat(buf, tmp);

    if(wild_time->min.end > wild_time->min.start)
    {
        if(wild_time->min.start <= 0
           && wild_time->min.end >= 59)
        {
            strcpy(tmp, ":*");
        }
        else
        {
        sprintf(tmp, ":%02d_%02d",
                wild_time->min.start,
                wild_time->min.end);
        }
    }
    else
    {
        sprintf(tmp, ":%02d", wild_time->min.start);
    }
    strcat(buf, tmp);

    if(wild_time->sec.end > wild_time->sec.start)
    {
        if(wild_time->sec.start <= 0
           && wild_time->sec.end >= 59)
        {
            strcpy(tmp, ":*");
        }
        else
        {
            sprintf(tmp, ":%02d_%02d",
                    wild_time->sec.start,
                    wild_time->sec.end);
        }
    }
    else
    {
        sprintf(tmp, ":%02d", wild_time->sec.start);
    }
    strcat(buf, tmp);
}
