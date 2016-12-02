#ifndef XML_UTILS_H_INCLUDED
#define XML_UTILS_H_INCLUDED

#include <libxml/encoding.h>
#include <libxml/xmlwriter.h>
#include <libxml/xmlreader.h>

#include <string>

std::string parse_content_node(xmlNodePtr node);
void write_content_node(xmlTextWriterPtr writer,
                        const char *name,
                        const std::string &content);

#endif // XML_UTILS_H_INCLUDED
