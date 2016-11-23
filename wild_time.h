#ifndef WILD_TIME_H_INCLUDED
#define WILD_TIME_H_INCLUDED

#define WILE_TIME_NOT_SET         0x0
#define WILD_TIME_SECOND_STAR     0x1
#define WILD_TIME_MINUTE_STAR     0x2
#define WILD_TIME_HOUR_STAR       0x4
#define WILD_TIME_DAY_STAR        0x8
#define WILD_TIME_MONTH_STAR      0x10
#define WILD_TIME_YEAR_STAR       0x20

#include <time.h>

typedef struct {
    struct tm time;
    unsigned int flags;
} wild_time_t;

void set_wild_time(wild_time_t *time, time_t value, unsigned int flags);
void parse_wild_time_string(wild_time_t *time, const char *s);
time_t wild_time_get_recent_time(wild_time_t *wild_time, time_t start);
time_t wild_time_get_next_time(wild_time_t *time, time_t start);
void format_wild_time(wild_time_t *wild_time, char *buf);

#endif // WILD_TIME_H_INCLUDED
