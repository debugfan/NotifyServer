#include <iostream>
#include "send_mail.h"
#include <stdio.h>
#include "accounts.h"
#include "sys_utils.h"
#include <string.h>

using namespace std;

int main()
{
    if(file_exists("accounts.xml")) {
        load_accounts_from_file("accounts.xml");
        for(std::list<std::string>::iterator iter = g_receiver_list.begin();
            iter != g_receiver_list.end();
            iter++)
        {
            send_mail(g_smtp_server, 1, g_username, g_password,
                NULL, iter->c_str(), NULL,
                "test subject", "test message");
        }
    }
    else {
        strcpy(g_smtp_server, "smtp.example.com");
        strcpy(g_username, "user@example.com");
        strcpy(g_password, "pass");
        g_receiver_list.push_back("alice@example.com");
        save_accounts_from_file("accounts.xml");
    }

    return 0;
}
