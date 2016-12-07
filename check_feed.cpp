#include "check_feed.h"
#include "wild_time.h"
#include <libxml/encoding.h>
#include <libxml/xmlwriter.h>
#include <libxml/xmlreader.h>
#include <string.h>
#include "work_state.h"
#include "http_client.h"
#include <jansson.h>
#include <float.h>
#include "notify.h"
#include "time_utils.h"
#include "log.h"
#include "xml_utils.h"
#include "feed/rsspp.h"

#include <list>
#include <string>

using namespace std;

typedef struct
{
    int enable;
    string url;
    list<wild_time_t> check_time_list;
    time_t last;
    string subject;
    string content;
} feed_outline_t;

list<feed_outline_t> g_feed_outline_list;

void process_feed_list_node(xmlNodePtr node)
{
    feed_outline_t feed;
    feed.last = 0;
    feed.enable = 0;

    for(xmlNodePtr child = node->children; child != NULL; child = child->next)
    {
        if(child->type == XML_ELEMENT_NODE)
        {
            xmlChar *content = NULL;
            if(0 == strcasecmp((const char *)child->name, "enable"))
            {
                content = xmlNodeGetContent(child);
                feed.enable = atoi((const char *)content);
                printf("enable: %s\n", content);
            }
            else if(0 == strcasecmp((const char *)child->name, "url"))
            {
                content = xmlNodeGetContent(child);
                feed.url = (const char *)content;
                printf("url: %s\n", content);
            }
            else if(0 == strcasecmp((const char *)child->name, "time"))
            {
                wild_time_t wild_time;
                content = xmlNodeGetContent(child);
                parse_wild_time_string(&wild_time,
                                       (const char *)content);
                feed.check_time_list.push_back(wild_time);
                printf("time: %s\n", content);
            }
            else if(0 == strcasecmp((const char *)child->name, "subject"))
            {
                content = xmlNodeGetContent(child);
                feed.subject = (const char *)content;
                printf("subject: %s\n", content);
            }
            else if(0 == strcasecmp((const char *)child->name, "content"))
            {
                string s = parse_content_node(child);
                feed.content = s;
                printf("content: %s\n", s.c_str());
            }
            else
            {
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

    g_feed_outline_list.push_back(feed);
}

static void process_feed_config(xmlTextReaderPtr reader)
{
    xmlNodePtr node;
    int ret;
    ret = xmlTextReaderRead(reader);
    while (ret == 1)
    {
        if(XML_READER_TYPE_ELEMENT == xmlTextReaderNodeType(reader))
        {
            node = xmlTextReaderExpand(reader);
            if(0 == strcasecmp((const char *)node->name, "feed"))
            {
                process_feed_list_node(node);
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

void load_feed_config_from_file(const char *filename)
{
    xmlTextReaderPtr reader;

    /*
     * this initialize the library and check potential ABI mismatches
     * between the version it was compiled for and the actual shared
     * library used.
     */
    LIBXML_TEST_VERSION

    reader = xmlReaderForFile(filename, NULL, 0);
    if (reader != NULL)
    {
        process_feed_config(reader);
        xmlFreeTextReader(reader);
    }
    else
    {
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

void save_feed_config_to_file(const char *filename)
{
    int rc;
    xmlTextWriterPtr writer;
    char tmp_buf[256];

    /* Create a new XmlWriter for filename, with no compression. */
    writer = xmlNewTextWriterFilename(filename, 0);
    if (writer == NULL)
    {
        printf("testXmlwriterFilename: Error creating the xml writer\n");
        return;
    }

    // Set indent
    xmlTextWriterSetIndent(writer, 1);
    xmlTextWriterSetIndentString(writer, BAD_CAST "    ");

    // Start
    rc = xmlTextWriterStartDocument(writer, NULL, NULL, NULL);
    if (rc < 0)
    {
        printf
        ("testXmlwriterFilename: Error at xmlTextWriterStartDocument\n");
        return;
    }

    /* Start an element named "config". */
    rc = xmlTextWriterStartElement(writer, BAD_CAST "config");
    if (rc < 0)
    {
        printf
        ("testXmlwriterFilename: Error at xmlTextWriterStartElement\n");
        return;
    }

    /* Add an attribute with name "version" and value "1.0" to config. */
    rc = xmlTextWriterWriteAttribute(writer, BAD_CAST "version",
                                     BAD_CAST "1.0");
    if (rc < 0)
    {
        printf
        ("testXmlwriterFilename: Error at xmlTextWriterWriteAttribute\n");
        return;
    }

    std::list<feed_outline_t>::iterator iter;
    for(iter = g_feed_outline_list.begin();
            iter != g_feed_outline_list.end();
            iter++)
    {
        rc = xmlTextWriterStartElement(writer, BAD_CAST "feed");
        if (rc < 0)
        {
            printf
            ("testXmlwriterFilename: Error at xmlTextWriterStartElement\n");
            return;
        }

        sprintf(tmp_buf, "%s", iter->enable == 0 ? "0" : "1");
        rc = xmlTextWriterWriteElement(writer, BAD_CAST "enable",
                                       BAD_CAST tmp_buf);
        if (rc < 0)
        {
            printf
            ("testXmlwriterFilename: Error at xmlTextWriterWriteFormatElement\n");
            return;
        }

        rc = xmlTextWriterWriteElement(writer, BAD_CAST "url",
                                       BAD_CAST iter->url.c_str());
        if (rc < 0)
        {
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
            if (rc < 0)
            {
                printf
                ("testXmlwriterFilename: Error at xmlTextWriterWriteFormatElement\n");
                return;
            }
        }

        rc = xmlTextWriterWriteElement(writer, BAD_CAST "subject",
                                       BAD_CAST iter->subject.c_str());
        if (rc < 0) {
            printf
                ("testXmlwriterFilename: Error at xmlTextWriterWriteFormatElement\n");
            return;
        }

        write_content_node(writer, "content", iter->content);

        rc = xmlTextWriterEndElement(writer);
        if (rc < 0)
        {
            printf
            ("testXmlwriterFilename: Error at xmlTextWriterEndElement\n");
            return;
        }

        rc = xmlTextWriterEndElement(writer);
        if (rc < 0)
        {
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
    if (rc < 0)
    {
        printf
        ("testXmlwriterFilename: Error at xmlTextWriterEndDocument\n");
        return;
    }

    xmlFreeTextWriter(writer);
}

void create_feed_config_file(const char *filename)
{
    feed_outline_t feed;
    time_t next = time(NULL) + 30;
    wild_time_t wild_time;
    set_wild_time(&wild_time, next, 0);
    feed.check_time_list.push_back(wild_time);
    feed.enable = 1;
    feed.url = "http://www.eduro.com/feed/";
    g_feed_outline_list.push_back(feed);
    save_feed_config_to_file(filename);
}

int execute_check_feed(feed_outline_t *feed,
                                     time_t last_check,
                                     time_t current_time)
{
    rsspp::parser p(0, "Rsspp");

    rsspp::feed f = p.parse_url(feed->url.c_str(),
                    feed->last,
                    "",
                    NULL,
                    "",
                    0);

    for(int i = 0; i < f.items.size(); i++)
    {
        string subject = f.items[i].title;
        string content = f.items[i].description;

        notify(subject.c_str(), content.c_str());
    }
}

time_t process_feed_item(feed_outline_t *feed,
                                     time_t last_check,
                                     time_t current_time)
{
    time_t start;

    start = feed->last;
    if(start == 0)
    {
        if(last_check > 0)
        {
            start = last_check;
        }
        else
        {
            time_t period = wild_time_list_get_approx_period(feed->check_time_list);
            if(period >= 0x7fffffff)
            {
                period = 60 * 60 * 24 * 183;
            }
            start = current_time - period/2;
        }
    }

    if(start > current_time)
    {
        start = current_time;
    }

    time_t next = wild_time_list_get_next_time(feed->check_time_list,
                                               start);
    if(next > start)
    {
        if(next <= current_time)
        {
            execute_check_feed(feed, last_check, current_time);
            feed->last = current_time;

            return wild_time_list_get_next_time(feed->check_time_list,
                                        current_time);
        }
        else
        {
            return next;
        }
    }
    else
    {
        return 0;
    }
}

time_t process_feed_list(time_t current)
{
    time_t last_check;
    time_t next = current + 60*60;
    get_work_state_time("last_check_feed", &last_check);
    std::list<feed_outline_t>::iterator iter;
    for(iter = g_feed_outline_list.begin();
            iter != g_feed_outline_list.end();
            iter++)
    {
        if(iter->enable != 0)
        {
            time_t tmp_next = process_feed_item(&(*iter),
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
    set_work_state_time("last_check_feed", &current);
    return next;
}
