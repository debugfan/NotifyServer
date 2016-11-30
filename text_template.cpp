#include "text_template.h"
#include "string_utils.h"
#include <time.h>
#include "time_utils.h"

void template_replace(std::string &template_text,
                      const std::map<std::string, std::string> &dict)
{
    std::map<std::string, std::string>::const_iterator it;
    for(it = dict.begin(); it != dict.end(); it++)
    {
        replace_all(template_text, it->first, it->second);
    }
}

void template_dict_set_time(std::map<std::string, std::string> &dict,
                            const time_t *time)
{
    char buf[16];
    struct tm tm_time = *(localtime(time));
    sprintf(buf, "%d", tm_time.tm_year + 1900);
    dict["{{YEAR}}"] = buf;
    dict["{{MONTH}}"] = MonthNames[tm_time.tm_mon];
    sprintf(buf, "%d", tm_time.tm_mday);
    dict["{{DAY}}"] = buf;
    dict["{{DAY_OF_WEEK}}"] = DayNames[tm_time.tm_wday];
    sprintf(buf, "%d", tm_time.tm_hour);
    dict["{{HOUR}}"] = buf;
    sprintf(buf, "%d", tm_time.tm_min);
    dict["{{MINUTE}}"] = buf;
    sprintf(buf, "%d", tm_time.tm_sec);
    dict["{{SECOND}}"] = buf;
}
