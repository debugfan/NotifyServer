#include "text_template.h"
#include "string_utils.h"
#include <time.h>
#include "time_utils.h"

void template_replace_key(std::string &template_text,
                          const std::string &key,
                          const dict_value_t &value)
{
    if(value.vt == dict_value_t::vt_string)
    {
        std::string t_key = std::string("{{") + key + std::string("}}");
        replace_all(template_text,
                    t_key,
                    value.v_string);
    }
    else if(value.vt == dict_value_t::vt_list)
    {
        size_t n_start, n_end;
        std::string s_start, s_end;
        s_start = std::string("{{#") + key + std::string("}}");
        s_end = std::string("{{/") + key + std::string("}}");
        while(true)
        {
            n_start = template_text.find(s_start);
            if(n_start == std::string::npos)
            {
                break;
            }
            n_end = template_text.find(s_end,
                                                n_start + s_start.length());
            if(n_end == std::string::npos)
            {
                break;
            }

            std::string template_map;
            template_map = template_text.substr(n_start + s_start.length(),
                                                            n_end - (n_start + s_start.length()));
            std::string template_result = "";
            std::list<std::map<std::string, struct _dict_value>>::const_iterator l_it;
            for(l_it = value.v_list.begin();
                l_it != value.v_list.end();
                l_it++)
            {
                std::string tmp = template_map;
                std::map<std::string, struct _dict_value>::const_iterator it;
                for(it = l_it->begin();
                    it != l_it->end();
                    it++)
                {
                    template_replace_key(tmp,
                                         it->first,
                                         it->second);
                }
                template_result += tmp;
            }
            template_text.replace(n_start,
                                  n_end + s_end.length() - n_start,
                                  template_result);
        }
    }
}

void template_replace(std::string &template_text,
                      const dict_t &dict)
{
    std::map<std::string, dict_value_t>::const_iterator it;
    for(it = dict.begin(); it != dict.end(); it++)
    {
        template_replace_key(template_text, it->first, it->second);
    }
}

void template_dict_map_set_pair(dict_map_t &dict_map,
                           const char *key,
                           const char *value)
{
    dict_value_t v;
    v.vt = dict_value_t::vt_string;
    v.v_string = value;
    dict_map[key] = v;
}

void template_dict_set_time(dict_t &dict,
                            const time_t *time)
{
    char buf[16];
    struct tm tm_time = *(localtime(time));
    sprintf(buf, "%d", tm_time.tm_year + 1900);
    template_dict_map_set_pair(dict, "YEAR", buf);
    template_dict_map_set_pair(dict, "MONTH", MonthNames[tm_time.tm_mon]);
    sprintf(buf, "%d", tm_time.tm_mday);
    template_dict_map_set_pair(dict, "DAY", buf);
    template_dict_map_set_pair(dict, "DAY_OF_WEEK",
                           DayNames[tm_time.tm_wday]);
    sprintf(buf, "%d", tm_time.tm_hour);
    template_dict_map_set_pair(dict, "HOUR", buf);
    sprintf(buf, "%d", tm_time.tm_min);
    template_dict_map_set_pair(dict, "MINUTE", buf);
    sprintf(buf, "%d", tm_time.tm_sec);
    template_dict_map_set_pair(dict, "SECOND", buf);
}

void template_dict_set_list_pair(dict_map_t &dict_map,
                           const char *key,
                           std::list<std::map<std::string, dict_value_t>> value)
{
    dict_value_t v;
    v.vt = dict_value_t::vt_list;
    v.v_list = value;
    dict_map[key] = v;
}
