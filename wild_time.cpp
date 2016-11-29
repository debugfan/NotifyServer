#include "wild_time.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

void set_wild_time(wild_time_t *time, time_t value, unsigned int flags)
{
    time->time = *(localtime(&value));
    time->flags = flags;
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

char *skip_blank(const char *p)
{
    while(*p != '\0' && is_blank(*p))
    {
        p++;
    }
    return (char *)p;
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

void parse_wild_day_time_string(wild_time_t *time, const char *s)
{
    char tmp[10];
    const char *p;
    const char *e;
    p = skip_blank(s);
    e = strchr(p, ':');
    if(*p == '*')
    {
        time->flags |= WILD_TIME_HOUR_STAR;
        time->time.tm_hour = 0;
    }
    else
    {
        if(e != NULL)
        {
            memset(tmp, 0, sizeof(tmp));
            strncpy(tmp, p, e - p);
            time->time.tm_hour = get_closest_valid_value(atoi(tmp), 0, 23);
        }
        else
        {
            return;
        }
    }
    p = e+1;

    e = strchr(p, ':');
    if(*p == '*')
    {
        time->flags |= WILD_TIME_MINUTE_STAR;
        time->time.tm_min = 0;
    }
    else
    {
        if(e != NULL)
        {
            memset(tmp, 0, sizeof(tmp));
            strncpy(tmp, p, e - p);
            time->time.tm_min = get_closest_valid_value(atoi(tmp), 0, 59);

        }
        else
        {
            return;
        }
    }
    p = e+1;

    if(*p == '*')
    {
        time->flags |= WILD_TIME_SECOND_STAR;
        time->time.tm_sec = 0;
    }
    else
    {
        time->time.tm_sec = get_closest_valid_value(atoi(p), 0, 59);
    }
}

void parse_wild_time_string(wild_time_t *time, const char *s)
{
    char tmp[10];
    const char *p;
    char *e;

    memset(time, 0, sizeof(wild_time_t));
    p = skip_blank(s);
    e = strchr(p, '-');
    if(*p == '*')
    {
        time->flags |= WILD_TIME_YEAR_STAR;
        time->time.tm_year = 0;
    }
    else
    {
        if(e != NULL)
        {
            memset(tmp, 0, sizeof(tmp));
            strncpy(tmp, p, e - p);
            time->time.tm_year = atoi(tmp) - 1900;
            p = e+1;
        }
        else
        {
            return;
        }
    }
    p = e+1;

    e = strchr(p, '-');
    if(*p == '*')
    {
        time->flags |= WILD_TIME_MONTH_STAR;
        time->time.tm_mon = 0;
    }
    else
    {
        if(e != NULL)
        {
            memset(tmp, 0, sizeof(tmp));
            strncpy(tmp, p, e - p);
            time->time.tm_mon = get_closest_valid_value(atoi(tmp) - 1, 0, 11);
        }
    }
    p = e+1;

    e = strchr(p, ' ');
    if(*p == '*')
    {
        time->flags |= WILD_TIME_DAY_STAR;
        time->time.tm_mday = 1;
    }
    else
    {
        if(e != NULL)
        {
            memset(tmp, 0, sizeof(tmp));
            strncpy(tmp, p, e - p);
            time->time.tm_mday = get_closest_valid_value(atoi(tmp), 0, 31);
        }
    }
    p = e+1;

    parse_wild_day_time_string(time, p);
}

void tm_get_recent_time(struct tm *recent, const struct tm *tm_adj, unsigned int tm_flags, time_t start)
{
    *recent = *(localtime(&start));

    if(!(tm_flags & WILD_TIME_SECOND_STAR))
    {
        recent->tm_sec = tm_adj->tm_sec;
    }

    if(!(tm_flags & WILD_TIME_MINUTE_STAR))
    {
        recent->tm_min = tm_adj->tm_min;
    }

    if(!(tm_flags & WILD_TIME_HOUR_STAR))
    {
        recent->tm_hour = tm_adj->tm_hour;
    }

    if(!(tm_flags & WILD_TIME_DAY_STAR))
    {
        recent->tm_mday = tm_adj->tm_mday;
    }

    if(!(tm_flags & WILD_TIME_MONTH_STAR))
    {
        recent->tm_mon = tm_adj->tm_mon;
    }

    if(!(tm_flags & WILD_TIME_YEAR_STAR))
    {
        recent->tm_year = tm_adj->tm_year;
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

bool tm_get_next_time(struct tm *next, struct tm *start, unsigned int time_flags)
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

time_t wild_time_get_recent_time(wild_time_t *wild_time, time_t current)
{
    struct tm tm_recent;
    struct tm tm_next;

    tm_get_recent_time(&tm_recent, &wild_time->time, wild_time->flags, current);

    if(tm_day_is_valid(&tm_recent, 0))
    {
        return mktime(&tm_recent);
    }
    else
    {
        tm_get_next_time(&tm_next, &tm_recent, wild_time->flags);
        return mktime(&tm_next);
    }
}

time_t wild_time_get_next_time(const wild_time_t *wild_time, time_t start)
{
    time_t next;
    struct tm tm_recent;
    struct tm tm_next;

    tm_get_recent_time(&tm_recent, &wild_time->time, wild_time->flags, start);
    next = mktime(&tm_recent);
    if(!tm_day_is_valid(&tm_recent, 0) || next <= start)
    {
        tm_get_next_time(&tm_next, &tm_recent, wild_time->flags);
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
        if(tmp_next < next)
        {
            next = tmp_next;
        }
    }
    return next;
}

void format_wild_time(const wild_time_t *wild_time, char *buf)
{
    char tmp[10];

    const struct tm *time = &wild_time->time;
    unsigned int star_flags = wild_time->flags;

    if(star_flags & WILD_TIME_YEAR_STAR)
    {
        strcpy(buf, "*");
    }
    else
    {
        sprintf(tmp, "%04d", time->tm_year + 1900);
        strcpy(buf, tmp);
    }

    if(star_flags & WILD_TIME_MONTH_STAR)
    {
        strcat(buf, "-*");
    }
    else
    {
        sprintf(tmp, "-%02d", time->tm_mon + 1);
        strcat(buf, tmp);
    }

    if(star_flags & WILD_TIME_DAY_STAR)
    {
        strcat(buf, "-*");
    }
    else
    {
        sprintf(tmp, "-%02d", time->tm_mday);
        strcat(buf, tmp);
    }

    strcat(buf, " ");

    if(star_flags & WILD_TIME_HOUR_STAR)
    {
        strcat(buf, "*");
    }
    else
    {
        sprintf(tmp, "%02d", time->tm_hour);
        strcat(buf, tmp);
    }

    if(star_flags & WILD_TIME_MINUTE_STAR)
    {
        strcat(buf, ":*");
    }
    else
    {
        sprintf(tmp, ":%02d", time->tm_min);
        strcat(buf, tmp);
    }

    if(star_flags & WILD_TIME_SECOND_STAR)
    {
        strcat(buf, ":*");
    }
    else
    {
        sprintf(tmp, ":%02d", time->tm_sec);
        strcat(buf, tmp);
    }
}
