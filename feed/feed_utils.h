#ifndef FEED_UTILS_H_INCLUDED
#define FEED_UTILS_H_INCLUDED

#include <string>
#include <vector>

std::string absolute_url(const std::string& url, const std::string& link);
void trim(std::string& str);
std::vector<std::string> tokenize(const std::string& str, std::string delimiters);

#endif // FEED_UTILS_H_INCLUDED
