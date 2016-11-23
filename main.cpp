#include <iostream>
#include "send_mail.h"
#include <stdio.h>
#include "accounts.h"
#include "sys_utils.h"
#include <string.h>
#include "reminders.h"
#include <getopt.h>
#include <windows.h>

using namespace std;

#define PACKAGE_NAME    "NoAssign"
#define PROGRAM_NAME    "NotifyServer"
#define PACKAGE_VERSION "1.0"

typedef struct _options_t {
    bool daemon;
} options_t;

void print_version(const char *program_name)
{
    fprintf(stdout, "%s %s %s\n",
        PACKAGE_NAME,
        program_name,
        PACKAGE_VERSION);
}

void print_help(const char *program_name)
{
    print_version(program_name);
    fprintf(stdout, " -d, --daemon      run as daemon\n");
    fprintf(stdout, " -v, --version     print version infomation\n");
    fprintf(stdout, " -h, --help        print this help\n");
}

int parse_options(int argc, char **argv, options_t *opts)
{
    int c;

    while (1)
    {
        static struct option long_options[] =
        {
            { "daemon", no_argument, 0, 'd' },
            { "help", no_argument, 0, 'h' },
            { "version", no_argument, 0, 'v' },
            { 0, 0, 0, 0 }
        };
        /* getopt_long stores the option index here. */
        int option_index = 0;

        c = getopt_long(argc, argv, "dhv",
            long_options, &option_index);

        /* Detect the end of the options. */
        if (c == -1) {
            break;
        }

        switch (c)
        {
        case 0:
            /* If this option set a flag, do nothing else now. */
            if (long_options[option_index].flag != 0)
                break;
            printf("option %s", long_options[option_index].name);
            if (optarg)
                printf(" with arg %s", optarg);
            printf("\n");
            break;
        case 'd':
            opts->daemon = true;
            break;
        case 'h':
            print_help(PROGRAM_NAME);
            exit(0);
            break;
        case 'v':
            print_version(PROGRAM_NAME);
            exit(0);
            break;
        case '?':
            /* getopt_long already printed an error message. */
            break;

        default:
            printf("Argument %c is not supported.\n", c);
            abort();
        }
    }

    /* Print any remaining command line arguments (not options). */
    if (optind < argc)
    {
        printf("non-option ARGV-elements: ");
        while (optind < argc)
            printf("%s ", argv[optind++]);
        putchar('\n');
    }

    return 0;
}

int win_daemon(char *command_line)
{
    STARTUPINFO si;
    PROCESS_INFORMATION pi;

    ZeroMemory( &si, sizeof(si) );
    si.cb = sizeof(si);
    ZeroMemory( &pi, sizeof(pi) );

    // Start the child process.
    if( !CreateProcess( NULL,   // No module name (use command line)
        command_line,        // Command line
        NULL,           // Process handle not inheritable
        NULL,           // Thread handle not inheritable
        FALSE,          // Set handle inheritance to FALSE
        CREATE_NO_WINDOW,
        NULL,           // Use parent's environment block
        NULL,           // Use parent's starting directory
        &si,            // Pointer to STARTUPINFO structure
        &pi )           // Pointer to PROCESS_INFORMATION structure
    )
    {
        printf( "CreateProcess failed (%d).\n", (int)GetLastError() );
        return -1;
    }

    // Close process and thread handles.
    CloseHandle( pi.hProcess );
    CloseHandle( pi.hThread );

    exit(0);
}

int main(int argc, char *argv[])
{
    options_t opts;
	memset(&opts, 0, sizeof(opts));
	parse_options(argc, argv, &opts);
	if(opts.daemon != false)
    {
        char cl[MAX_PATH];
        for(int i = 0; i < argc; i++)
        {
            if(0 != strcmp(argv[i], "-d")
               && 0 != strcmp(argv[i], "--daemon"))
            {
                if(i == 0)
                {
                    strcpy(cl, argv[i]);
                }
                else
                {
                    strcat(cl, " ");
                    strcat(cl, argv[i]);
                }
            }
        }
        printf("Daemon command line: %s\n", cl);
        if(win_daemon(cl) < 0)
        {
            printf("Create daemon failed.\n");
        }
    }

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
