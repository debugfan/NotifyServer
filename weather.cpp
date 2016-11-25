#include "weather.h"
#include "wild_time.h"
#include <libxml/encoding.h>
#include <libxml/xmlwriter.h>
#include <libxml/xmlreader.h>
#include <string.h>
#include "work_state.h"

#include <list>
#include <string>

using namespace std;

typedef struct {
    int enable;
    string city;
    string url;
    list<wild_time_t> check_time_list;
} weather_warning_t;

list<weather_warning_t> g_weather_warnining_list;

void process_weather_list_node(xmlNodePtr node)
{
    weather_warning_t warning;
    for(xmlNodePtr child = node->children; child != NULL; child = child->next)
    {
        if(child->type == XML_ELEMENT_NODE)
        {
            xmlChar *content = NULL;
            if(0 == strcasecmp((const char *)child->name, "enable"))
            {
                content = xmlNodeGetContent(child);
                warning.enable = atoi((const char *)content);
                printf("time: %s\n", content);
            }
            else if(0 == strcasecmp((const char *)child->name, "city"))
            {
                content = xmlNodeGetContent(child);
                warning.city = (const char *)content;
                printf("time: %s\n", content);
            }
            else if(0 == strcasecmp((const char *)child->name, "time"))
            {
                wild_time_t wild_time;
                content = xmlNodeGetContent(child);
                parse_wild_time_string(&wild_time,
                                      (const char *)content);
                warning.check_time_list.push_back(wild_time);
                printf("time: %s\n", content);
            }
            else {
                content = xmlNodeGetContent(child);
                printf("unknown key: %s, value: %s\n",
                    child->name,
                    content);
            }

            if(content != NULL)
            {
                xmlFree(content);
            }
        }
    }

    g_weather_warnining_list.push_back(warning);
}

static void process_weather_config(xmlTextReaderPtr reader)
{
    xmlNodePtr node;
    int ret;
    ret = xmlTextReaderRead(reader);
    while (ret == 1) {
        if(XML_READER_TYPE_ELEMENT == xmlTextReaderNodeType(reader))
        {
            node = xmlTextReaderExpand(reader);
            if(0 == strcasecmp((const char *)node->name, "weather_warning"))
            {
                process_weather_list_node(node);
                ret = xmlTextReaderNext(reader);
            }
            else
            {
                ret = xmlTextReaderRead(reader);
            }
        }
        else
        {
            ret = xmlTextReaderRead(reader);
        }
    }
}

void load_weather_config_from_file(const char *filename)
{
    xmlTextReaderPtr reader;

    /*
     * this initialize the library and check potential ABI mismatches
     * between the version it was compiled for and the actual shared
     * library used.
     */
    LIBXML_TEST_VERSION

    reader = xmlReaderForFile(filename, NULL, 0);
    if (reader != NULL) {
        process_weather_config(reader);
        xmlFreeTextReader(reader);
    } else {
        fprintf(stderr, "Unable to open %s\n", filename);
    }

    /*
     * Cleanup function for the XML library.
     */
    xmlCleanupParser();
    /*
     * this is to debug memory for regression tests
     */
    xmlMemoryDump();
}

void save_weather_config_to_file(const char *filename)
{
    int rc;
    xmlTextWriterPtr writer;
    char tmp_buf[256];

    /* Create a new XmlWriter for filename, with no compression. */
    writer = xmlNewTextWriterFilename(filename, 0);
    if (writer == NULL) {
        printf("testXmlwriterFilename: Error creating the xml writer\n");
        return;
    }

    // Set indent
    xmlTextWriterSetIndent(writer, 1);
    xmlTextWriterSetIndentString(writer, BAD_CAST "    ");

    // Start
    rc = xmlTextWriterStartDocument(writer, NULL, NULL, NULL);
    if (rc < 0) {
        printf
            ("testXmlwriterFilename: Error at xmlTextWriterStartDocument\n");
        return;
    }

    /* Start an element named "config". */
    rc = xmlTextWriterStartElement(writer, BAD_CAST "config");
    if (rc < 0) {
        printf
            ("testXmlwriterFilename: Error at xmlTextWriterStartElement\n");
        return;
    }

    /* Add an attribute with name "version" and value "1.0" to config. */
    rc = xmlTextWriterWriteAttribute(writer, BAD_CAST "version",
                                     BAD_CAST "1.0");
    if (rc < 0) {
        printf
            ("testXmlwriterFilename: Error at xmlTextWriterWriteAttribute\n");
        return;
    }

    std::list<weather_warning_t>::iterator iter;
    for(iter = g_weather_warnining_list.begin();
        iter != g_weather_warnining_list.end();
        iter++)
    {
        rc = xmlTextWriterStartElement(writer, BAD_CAST "weather_warning");
        if (rc < 0) {
            printf
                ("testXmlwriterFilename: Error at xmlTextWriterStartElement\n");
            return;
        }

        sprintf(tmp_buf, "%s", iter->enable == 0 ? "0" : "1");
        rc = xmlTextWriterWriteElement(writer, BAD_CAST "enable",
                                       BAD_CAST tmp_buf);
        if (rc < 0) {
            printf
                ("testXmlwriterFilename: Error at xmlTextWriterWriteFormatElement\n");
            return;
        }

        rc = xmlTextWriterWriteElement(writer, BAD_CAST "city",
                                       BAD_CAST iter->city.c_str());
        if (rc < 0) {
            printf
                ("testXmlwriterFilename: Error at xmlTextWriterWriteFormatElement\n");
            return;
        }

        std::list<wild_time_t>::iterator s_iter;
        for(s_iter = iter->check_time_list.begin();
            s_iter != iter->check_time_list.end();
            s_iter++)
        {
            format_wild_time(&(*s_iter), tmp_buf);
            rc = xmlTextWriterWriteElement(writer, BAD_CAST "time",
                                           BAD_CAST tmp_buf);
            if (rc < 0) {
                printf
                    ("testXmlwriterFilename: Error at xmlTextWriterWriteFormatElement\n");
                return;
            }
        }

        rc = xmlTextWriterEndElement(writer);
        if (rc < 0) {
            printf
                ("testXmlwriterFilename: Error at xmlTextWriterEndElement\n");
            return;
        }

        rc = xmlTextWriterEndElement(writer);
        if (rc < 0) {
            printf
                ("testXmlwriterFilename: Error at xmlTextWriterEndElement\n");
            return;
        }
    }

    /* Here we could close the element config using the
     * function xmlTextWriterEndElement, but since we do not want to
     * write any other elements, we simply call xmlTextWriterEndDocument,
     * which will do all the work. */
    rc = xmlTextWriterEndDocument(writer);
    if (rc < 0) {
        printf
            ("testXmlwriterFilename: Error at xmlTextWriterEndDocument\n");
        return;
    }

    xmlFreeTextWriter(writer);
}

time_t process_weather_warning(weather_warning_t *warning,
                               time_t last_check,
                            time_t current_time)
{
    return 60*60;
}

time_t process_weather_warning_list(time_t current)
{
    time_t last_check;
    time_t next = current + 60*60;
    get_work_state_time("last_check_weather", &last_check);
    while(true) {
        std::list<weather_warning_t>::iterator iter;
        for(iter = g_weather_warnining_list.begin();
            iter != g_weather_warnining_list.end();
            iter++)
        {
            time_t tmp_next = process_weather_warning(&(*iter),
                                                      last_check,
                                                      current);
            if(tmp_next != 0 && tmp_next >= current)
            {
                if(next > tmp_next)
                {
                    next = tmp_next;
                }
            }
        }
    }
    set_work_state_time("last_check_weather", &last_check);
    return next;
}
