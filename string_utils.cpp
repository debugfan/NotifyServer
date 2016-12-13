#include "string_utils.h"

void replace_all(std::string& str,
                 const std::string& from,
                 const std::string& to)
{
	if(from.empty())
    {
        return;
    }
	size_t start_pos = 0;
	while((start_pos = str.find(from, start_pos)) != std::string::npos)
    {
		str.replace(start_pos, from.length(), to);
		start_pos += to.length(); // In case 'to' contains 'from', like replacing 'x' with 'yx'
	}
}

void split_string(std::vector<std::string> &string_list,
                  const std::string& text,
                  const std::string& delims)
{
    std::size_t start;
    std::size_t end;

    start = text.find_first_not_of(delims);

    while((end = text.find_first_of(delims, start)) != std::string::npos)
    {
        string_list.push_back(text.substr(start, end - start));
        start = text.find_first_not_of(delims, end);
    }

    if(start != std::string::npos)
    {
        string_list.push_back(text.substr(start));
    }
}

char *skip_blank(const char *s)
{
    while(*s == ' ' || *s == '\t')
    {
        s++;
    }
    return (char *)s;
}

