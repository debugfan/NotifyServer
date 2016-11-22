#include "wild_time.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

time_t make_time_from_local(struct tm *value)
{
    time_t tt_now;
    time_t diff;
    struct tm tm_now_local;
    struct tm tm_now_gm;
    tt_now = time(NULL);
    tm_now_local = *(localtime(&tt_now));
    tm_now_gm = *(gmtime(&tt_now));
    diff = mktime(&tm_now_local) - mktime(&tm_now_gm);
    return mktime(value) - diff;
}

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
    }
    else
    {
        if(e != NULL)
        {
            memset(tmp, 0, sizeof(tmp));
            strncpy(tmp, p, e - p);
            time->time.tm_hour = atoi(tmp);
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
    }
    else
    {
        if(e != NULL)
        {
            memset(tmp, 0, sizeof(tmp));
            strncpy(tmp, p, e - p);
            time->time.tm_min = atoi(tmp);

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
    }
    else
    {
        time->time.tm_sec = atoi(p);
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
    }
    else
    {
        if(e != NULL)
        {
            memset(tmp, 0, sizeof(tmp));
            strncpy(tmp, p, e - p);
            time->time.tm_mon = atoi(tmp) - 1;
        }
    }
    p = e+1;

    e = strchr(p, ' ');
    if(*p == '*')
    {
        time->flags |= WILD_TIME_DAY_STAR;
    }
    else
    {
        if(e != NULL)
        {
            memset(tmp, 0, sizeof(tmp));
            strncpy(tmp, p, e - p);
            time->time.tm_mday = atoi(tmp);
        }
    }
    p = e+1;

    parse_wild_day_time_string(time, p);
}

void wild_time_get_recent_time(struct tm *recent, wild_time_t *wild_time, time_t start)
{
    struct tm tm_start;
    struct tm *tm_adj;

    tm_start = *(localtime(&start));
    tm_adj = &wild_time->time;

    if(!(wild_time->flags & WILD_TIME_SECOND_STAR))
    {
        recent->tm_sec = tm_adj->tm_sec;
    }

    if(!(wild_time->flags & WILD_TIME_MINUTE_STAR))
    {
        recent->tm_min = tm_adj->tm_min;
    }

    if(!(wild_time->flags & WILD_TIME_HOUR_STAR))
    {
        recent->tm_hour = tm_adj->tm_hour;
    }

    if(!(wild_time->flags & WILD_TIME_DAY_STAR))
    {
        recent->tm_hour = tm_adj->tm_mday;
    }

    if(!(wild_time->flags & WILD_TIME_MONTH_STAR))
    {
        recent->tm_mon = tm_adj->tm_mon;
    }

    if(!(wild_time->flags & WILD_TIME_YEAR_STAR))
    {
        recent->tm_year = tm_adj->tm_year;
    }
}

time_t wild_time_get_next_time(wild_time_t *wild_time, time_t start)
{
    time_t next;
    struct tm tm_start;
    struct tm tm_next;
    struct tm *tm_adj;
    unsigned int period_flag = 0;

    tm_start = *(localtime(&start));
    tm_next = *(localtime(&start));
    tm_adj = &wild_time->time;

    if(!(wild_time->flags & WILD_TIME_SECOND_STAR))
    {
        tm_next.tm_sec = tm_adj->tm_sec;
    }
    else
    {
        if(period_flag == 0)
        {
            period_flag = WILD_TIME_SECOND_STAR;
        }
    }

    if(!(wild_time->flags & WILD_TIME_MINUTE_STAR))
    {
        tm_next.tm_min = tm_adj->tm_min;
    }
    else
    {
        if(period_flag == 0)
        {
            period_flag = WILD_TIME_MINUTE_STAR;
        }
    }
    if(!(wild_time->flags & WILD_TIME_HOUR_STAR))
    {
        tm_next.tm_hour = tm_adj->tm_hour;
    }
    else
    {
        if(period_flag == 0)
        {
            period_flag = WILD_TIME_HOUR_STAR;
        }
    }
    if(!(wild_time->flags & WILD_TIME_DAY_STAR))
    {
        tm_next.tm_hour = tm_adj->tm_mday;
    }
    else
    {
        if(period_flag == 0)
        {
            period_flag = WILD_TIME_DAY_STAR;
        }
    }

    if(!(wild_time->flags & WILD_TIME_MONTH_STAR))
    {
        tm_next.tm_mon = tm_adj->tm_mon;
    }
    else
    {
        if(period_flag == 0)
        {
            period_flag = WILD_TIME_MONTH_STAR;
        }
    }

    if(!(wild_time->flags & WILD_TIME_YEAR_STAR))
    {
        tm_next.tm_year = tm_adj->tm_year;
    }
    else
    {
        if(period_flag == 0)
        {
            period_flag = WILD_TIME_YEAR_STAR;
        }
    }

    next = make_time_from_local(&tm_next);
    if(next <= start)
    {
        switch(period_flag)
        {
        case WILD_TIME_YEAR_STAR:
            tm_next.tm_year++;
            next = make_time_from_local(&tm_next);
            break;
        case WILD_TIME_MONTH_STAR:
            tm_next.tm_mon++;
            if(tm_next.tm_mon >= 12)
            {
                tm_next.tm_mon = tm_next.tm_mon%12;
                tm_next.tm_year = tm_next.tm_mon/12;
            }
            next = make_time_from_local(&tm_next);
        case WILD_TIME_DAY_STAR:
            next += 60*60*24;
            break;
        case WILD_TIME_HOUR_STAR:
            next += 60*60;
            break;
        case WILD_TIME_MINUTE_STAR:
            next += 60;
        case WILD_TIME_SECOND_STAR:
            next += 1;
            break;
        }
    }
    return next;
}

void format_wild_time(wild_time_t *wild_time, char *buf)
{
    char tmp[10];

    struct tm *time = &wild_time->time;
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
        sprintf(tmp, ":%02d", time->tm_min + 1);
        strcat(buf, tmp);
    }

    if(star_flags & WILD_TIME_SECOND_STAR)
    {
        strcat(buf, ":*");
    }
    else
    {
        sprintf(tmp, ":%02d", time->tm_min + 1);
        strcat(buf, tmp);
    }
}

