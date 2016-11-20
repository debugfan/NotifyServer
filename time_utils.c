#include "time_utils.h"
#include <time.h>
#include <stdio.h>

const char *DayStr[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
const char *MthStr[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun",
                        "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

void build_mail_date_string(char *buf, int len)
{
    struct  tm *lt;
    struct  tm *gmt;
    time_t  tt;
    time_t  tt_local;
    time_t  tt_gmt;
    double  tzonediffFloat;
    int     tzonediffWord;
    char    tzoneBuf[16];

    /* Calculating time diff between GMT and localtime */
    tt       = time(0);
    lt       = localtime(&tt);
    tt_local = mktime(lt);
    gmt      = gmtime(&tt);
    tt_gmt   = mktime(gmt);
    tzonediffFloat = difftime(tt_local, tt_gmt);
    tzonediffWord  = (int)(tzonediffFloat/3600.0);

    if((double)(tzonediffWord * 3600) == tzonediffFloat)
      snprintf(tzoneBuf, 15, "%+03d00", tzonediffWord);
    else
      snprintf(tzoneBuf, 15, "%+03d30", tzonediffWord);

    lt       = localtime(&tt);
    // example: "Date: Mon, 29 Nov 2010 21:54:29 +1100\r\n"
    snprintf(buf, len, "%s %02d %s %04d %02d:%02d:%02d %s",
             DayStr[lt->tm_wday],
             lt->tm_mday, MthStr[lt->tm_mon], lt->tm_year + 1900,
             lt->tm_hour, lt->tm_min, lt->tm_sec,
             tzoneBuf);
}
