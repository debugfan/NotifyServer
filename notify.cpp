#include "notify.h"
#include <string>
#include <list>
#include "accounts.h"
#include "send_mail.h"

void notify(const char *subject, const char *content)
{
    for(std::list<std::string>::iterator iter = g_receiver_list.begin();
        iter != g_receiver_list.end();
        iter++)
    {
        send_mail(g_smtp_server, 1, g_username, g_password,
            NULL, iter->c_str(), NULL,
            subject, content);
    }
}
