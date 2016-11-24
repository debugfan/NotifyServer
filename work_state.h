#ifndef WORK_STATE_H_INCLUDED
#define WORK_STATE_H_INCLUDED

#include <time.h>

void set_work_state_file(const char *filename,
                         const char *section_name);
void get_work_state(const char *key, char *buf, int len);
void set_work_state(const char *key, const char *value);

void get_work_state_time(const char *key, time_t *time);
void set_work_state_time(const char *key, const time_t *time);

#endif // WORK_STATE_H_INCLUDED
