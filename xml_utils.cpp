#include "xml_utils.h"

#include <vector>
#include <string.h>
#include "string_utils.h"

std::string dump_node_children(xmlNodePtr node)
{
    std::string r = "";
    for(xmlNodePtr child = node->children; child != NULL; child = child->next)
    {
        xmlBufferPtr pbuf =	xmlBufferCreate();
        if(pbuf != NULL)
        {
            xmlNodeDump(pbuf, NULL, child, 0, 0);
            const xmlChar *content = xmlBufferContent(pbuf);
            r += std::string((const char *)content);
            xmlBufferFree(pbuf);
        }
    }
    return r;
}

std::string parse_content_node(xmlNodePtr node)
{
    std::string r = "";
    for(xmlNodePtr child = node->children; child != NULL; child = child->next)
    {
        if(child->type == XML_ELEMENT_NODE)
        {
            if(0 == strcasecmp((const char *)child->name, "p"))
            {
                r += dump_node_children(child) + "\r\n";
            }
        }
    }
    return r;
}

void write_content_node(xmlTextWriterPtr writer,
                        const char *name,
                        const std::string &content)
{
    int rc;
    rc = xmlTextWriterStartElement(writer, BAD_CAST "content");
    if (rc < 0)
    {
        printf
        ("testXmlwriterFilename: Error at xmlTextWriterWriteFormatElement\n");
        return;
    }

    std::vector<std::string> string_list;
    split_string(string_list, content, "\n");
    for(std::vector<std::string>::const_iterator s_iter = string_list.begin();
            s_iter != string_list.end();
            s_iter++)
    {
        rc = xmlTextWriterWriteElement(writer, BAD_CAST "p",
                                       BAD_CAST s_iter->c_str());
        if (rc < 0)
        {
            printf
            ("testXmlwriterFilename: Error at xmlTextWriterWriteFormatElement\n");
            return;
        }
    }

    rc = xmlTextWriterEndElement(writer);
    if (rc < 0)
    {
        printf
        ("testXmlwriterFilename: Error at xmlTextWriterEndElement\n");
        return;
    }
}
