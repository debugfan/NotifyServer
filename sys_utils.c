#include "sys_utils.h"
#include <stdio.h>

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
