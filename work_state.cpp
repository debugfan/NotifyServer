#include "work_state.h"
#include <windows.h>
#include <stdio.h>
#include "sys_utils.h"

char g_config_file[MAX_PATH] = "575E45AEA2314B598E19E2FCCCF741A3.ini";
char g_section_name[256] = "work_state";

void set_work_state_file(const char *filename,
                         const char *section_name)
{
    strcpy(g_config_file, filename);
    strcpy(g_section_name, section_name);
}

void get_work_state(const char *key, char *buf, int len)
{
    if(!file_exists(g_config_file))
    {
        return;
    }
    if(!GetPrivateProfileString(g_section_name,
                            key,
                            "",
                            buf,
                            len,
                            g_config_file))
    {
        printf("Read profile failed: %d.\n", (int)GetLastError());
    }
}

void set_work_state(const char *key, const char *value)
{
    if(!WritePrivateProfileString(g_section_name,
                              key,
                              value,
                              g_config_file))
    {
        printf("Write profile failed: %d.\n", (int)GetLastError());
    }
}

void get_work_state_time(const char *key, time_t *time)
{
    char tmp_buf[32] = {0};
    get_work_state(key, tmp_buf, sizeof(tmp_buf));
    *time = atoi(tmp_buf);
}

void set_work_state_time(const char *key, const time_t *time)
{
    char tmp_buf[32];
    sprintf(tmp_buf, "%ld", *time);
    set_work_state(key, tmp_buf);
}
