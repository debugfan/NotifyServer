#include <iostream>
#include "send_mail.h"
#include <stdio.h>
#include "accounts.h"
#include "sys_utils.h"
#include <string.h>
#include "reminders.h"

using namespace std;

int main()
{
    if(file_exists("accounts.xml")) {
        load_accounts_from_file("accounts.xml");
    }
    else {
        strcpy(g_smtp_server, "smtp.example.com");
        strcpy(g_username, "user@example.com");
        strcpy(g_password, "pass");
        g_receiver_list.push_back("alice@example.com");
        save_accounts_to_file("accounts.xml");
    }

    if(file_exists("reminders.xml")) {
        load_reminders_from_file("reminders.xml");
    }
    else
    {
        time_t next = time(NULL) + 30;
        reminder_t reminder;
        set_wild_time(&reminder.time, next, 0);
        reminder.subject = "test";
        reminder.content = "test";
        g_reminder_list.push_back(reminder);
        save_reminders_to_file("reminders.xml");
    }

    process_reminders();

    return 0;
}
