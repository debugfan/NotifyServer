#include "sys_utils.h"
#include <stdio.h>
#include <direct.h>
#include <windows.h>

bool file_exists(const char *filename)
{
    FILE *fp = fopen(filename, "r");
    if(fp != NULL)
    {
        fclose(fp);
        return true;
    }
    else
    {
        return false;
    }
}

void get_full_path(char *fullpath,
                   const char *dir,
                   const char *filename)
{
    if(dir != NULL && strlen(dir) > 0)
    {
        sprintf(fullpath, "%s\\", dir);
    }
    strcat(fullpath, filename);
}

bool directory_file_exists(const char *dir,
                        const char *filename)
{
    char fullpath[MAX_PATH];
    get_full_path(fullpath, dir, filename);
    if(file_exists(fullpath))
    {
        return true;
    }
    else
    {
        return false;
    }
}

int create_dir_safely(const char *dir)
{
    char tmp[MAX_PATH];
    int off;

    for (off = 0; off < (sizeof(tmp) - 1) && dir[off] != '\0'; off++)
    {
        if (dir[off] == '\\' || dir[off] == '/')
        {
            tmp[off] = '\\';
            tmp[off + 1] = '\0';

            if (-1 == _access(tmp, 0))
            {
                if (-1 == _mkdir(tmp))
                {
                    return -1;
                }
            }
        }
        else
        {
            tmp[off] = dir[off];
        }
    }

    tmp[off] = '\0';

    if (off > 0 && -1 == _access(tmp, 0))
    {
        if (-1 == _mkdir(tmp))
        {
            return -1;
        }
    }

    return off;
}

void get_default_config_directory(char *buf, int len)
{
    strcpy(buf, getenv("HOMEDRIVE"));
    strcat(buf, getenv("HOMEPATH"));
    strcat(buf, "\\.NofityServer");
}
