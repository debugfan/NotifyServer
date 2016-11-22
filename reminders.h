#ifndef REMINDERS_H_INCLUDED
#define REMINDERS_H_INCLUDED

#include <list>
#include <string>
#include <time.h>
#include "wild_time.h"

typedef struct {
    wild_time_t time;
    time_t last;
    std::string subject;
    std::string content;
} reminder_t;

extern std::list<reminder_t> g_reminder_list;

void load_reminders_from_file(const char *filename);
void save_reminders_to_file(const char *filename);
void process_reminders();

#endif // REMINDERS_H_INCLUDED
