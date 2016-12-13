#ifndef STRING_UTILS_H_INCLUDED
#define STRING_UTILS_H_INCLUDED

#include <vector>
#include <string>
#include <map>

char *skip_blank(const char *s);

void replace_all(std::string& str,
                 const std::string& from,
                 const std::string& to);

void split_string(std::vector<std::string> &string_list,
                  const std::string& text,
                  const std::string& delims);


#endif // STRING_UTILS_H_INCLUDED
