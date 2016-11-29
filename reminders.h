#ifndef REMINDERS_H_INCLUDED
#define REMINDERS_H_INCLUDED

#include <list>
#include <string>
#include <time.h>
#include "wild_time.h"

typedef struct {
    std::list<wild_time_t> time_list;
    time_t last;
    std::string subject;
    std::string content;
} reminder_t;

extern std::list<reminder_t> g_reminder_list;

void load_reminders_from_file(const char *filename);
void save_reminders_to_file(const char *filename);
time_t process_reminders(time_t check_time);

#endif // REMINDERS_H_INCLUDED
