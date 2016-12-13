#ifndef REMINDERS_H_INCLUDED
#define REMINDERS_H_INCLUDED

#include <time.h>

void load_reminders_from_file(const char *filename);
void save_reminders_to_file(const char *filename);
void create_reminders_config_file(const char *filename);

time_t process_reminders(time_t check_time);

#endif // REMINDERS_H_INCLUDED
