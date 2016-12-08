#ifndef TEXT_TEMPLATE_H_INCLUDED
#define TEXT_TEMPLATE_H_INCLUDED

#include <string>
#include <map>
#include <list>

struct _dict_value;

typedef struct _dict_value dict_value_t;
typedef std::map<std::string, dict_value_t> dict_map_t;

struct _dict_value {
    enum variant_type {vt_string, vt_list} vt;
    std::string v_string;
    std::list<dict_map_t> v_list;
};

typedef dict_map_t dict_t;

void template_dict_set_time(dict_t &dict,
                            const time_t *time);

void template_dict_map_set_pair(dict_map_t &dict_map,
                           const char *key,
                           const char *value);

void template_replace(std::string &template_text,
                      const dict_t &dict);

#define template_dict_set_pair template_dict_map_set_pair

void template_dict_set_list_pair(dict_map_t &dict_map,
                           const char *key,
                           std::list<std::map<std::string, dict_value_t>> value);

#endif // TEXT_TEMPLATE_H_INCLUDED
