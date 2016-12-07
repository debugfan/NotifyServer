#include "feed_utils.h"
#include <libxml/uri.h>

std::string absolute_url(const std::string& url, const std::string& link) {
	xmlChar * newurl = xmlBuildURI((const xmlChar *)link.c_str(), (const xmlChar *)url.c_str());
	std::string retval;
	if (newurl) {
		retval = (const char *)newurl;
		xmlFree(newurl);
	} else {
		retval = link;
	}
	return retval;
}

void trim_end(std::string& str) {
	std::string::size_type pos = str.length()-1;
	while (str.length()>0 && (str[pos] == '\n' || str[pos] == '\r')) {
		str.erase(pos);
		pos--;
	}
}

void trim(std::string& str) {
	while (str.length() > 0 && ::isspace(str[0])) {
		str.erase(0,1);
	}
	trim_end(str);
}

std::vector<std::string> tokenize(const std::string& str, std::string delimiters) {
	/*
	 * This function tokenizes a string by the delimiters. Plain and simple.
	 */
	std::vector<std::string> tokens;
	std::string::size_type last_pos = str.find_first_not_of(delimiters, 0);
	std::string::size_type pos = str.find_first_of(delimiters, last_pos);

	while (std::string::npos != pos || std::string::npos != last_pos) {
		tokens.push_back(str.substr(last_pos, pos - last_pos));
		last_pos = str.find_first_not_of(delimiters, pos);
		pos = str.find_first_of(delimiters, last_pos);
	}
	return tokens;
}

