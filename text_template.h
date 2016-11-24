#ifndef TEXT_TEMPLATE_H_INCLUDED
#define TEXT_TEMPLATE_H_INCLUDED

#include <string>
#include <map>

void template_dict_set_time(std::map<std::string, std::string> &dict,
                            const time_t *time);

void template_replace(std::string &template_text,
                      const std::map<std::string, std::string> &dict);

#endif // TEXT_TEMPLATE_H_INCLUDED
