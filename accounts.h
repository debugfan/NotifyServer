#ifndef ACCOUNTS_H_INCLUDED
#define ACCOUNTS_H_INCLUDED

#include <string>
#include <list>

extern char g_smtp_server[512];
extern char g_username[512];
extern char g_password[512];
extern std::list<std::string> g_receiver_list;

void load_accounts_from_file(const char *filename);
void save_accounts_from_file(const char *filename);

#endif // ACCOUNTS_H_INCLUDED
