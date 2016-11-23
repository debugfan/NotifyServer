#include "reminders.h"
#include <libxml/encoding.h>
#include <libxml/xmlwriter.h>
#include <libxml/xmlreader.h>
#include <string.h>
#include "notify.h"
#include <windows.h>
#include "wild_time.h"

using namespace std;

std::list<reminder_t> g_reminder_list;

void process_reminder_list_node(xmlNodePtr node)
{
    reminder_t reminder;
    for(xmlNodePtr child = node->children; child != NULL; child = child->next)
    {
        if(child->type == XML_ELEMENT_NODE)
        {
            xmlChar *content = NULL;
            if(0 == strcasecmp((const char *)child->name, "time"))
            {
                content = xmlNodeGetContent(child);
                parse_wild_time_string(&reminder.time,
                                      (const char *)content);
                printf("time: %s\n", content);
            }
            else if(0 == strcasecmp((const char *)child->name, "subject"))
            {
                content = xmlNodeGetContent(child);
                reminder.subject = (const char *)content;
                printf("subject: %s\n", xmlNodeGetContent(child));
            }
            else if(0 == strcasecmp((const char *)child->name, "content"))
            {
                content = xmlNodeGetContent(child);
                reminder.content = (const char *)content;
                printf("content: %s\n", xmlNodeGetContent(child));
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
    reminder.last = 0;
    g_reminder_list.push_back(reminder);
}

static void process_reminders_config(xmlTextReaderPtr reader)
{
    xmlNodePtr node;
    int ret;
    ret = xmlTextReaderRead(reader);
    while (ret == 1) {
        if(XML_READER_TYPE_ELEMENT == xmlTextReaderNodeType(reader))
        {
            node = xmlTextReaderExpand(reader);
            if(0 == strcasecmp((const char *)node->name, "reminder"))
            {
                process_reminder_list_node(node);
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

void load_reminders_from_file(const char *filename)
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
        process_reminders_config(reader);
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

time_t get_next_remind_time(reminder_t *reminder)
{
    time_t start;
    time_t now;
    now = time(NULL);
    if(reminder->last == 0)
    {
        return wild_time_get_recent_time(&reminder->time, now);
    }
    else
    {
        if(reminder->last > now)
        {
            start = now;
        }
        else
        {
            start = reminder->last;
        }
        return wild_time_get_next_time(&reminder->time, start);
    }
}

void process_reminders()
{
    time_t wait = 0;
    while(true) {
        wait = 60*60;
        for(std::list<reminder_t>::iterator iter = g_reminder_list.begin();
            iter != g_reminder_list.end();
            iter++)
        {
            time_t next = get_next_remind_time(&(*iter));
            time_t now = time(NULL);
            if(next > iter->last)
            {
                if(next <= now)
                {
                    wait = 0;
                    notify(iter->subject.c_str(), iter->content.c_str());
                    iter->last = now;
                }
                else
                {
                    time_t tmp = difftime(next, now);
                    if(wait > tmp)
                    {
                        wait = tmp;
                    }
                }
            }
        }
        if(wait > 0)
        {
            Sleep(wait*1000);
        }
    }
}

void save_reminders_to_file(const char *filename)
{
    int rc;
    xmlTextWriterPtr writer;

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

    for(std::list<reminder_t>::iterator iter = g_reminder_list.begin();
        iter != g_reminder_list.end();
        iter++)
    {
        rc = xmlTextWriterStartElement(writer, BAD_CAST "reminder");
        if (rc < 0) {
            printf
                ("testXmlwriterFilename: Error at xmlTextWriterStartElement\n");
            return;
        }

        char time_buf[256];
        format_wild_time(&iter->time, time_buf);
        rc = xmlTextWriterWriteElement(writer, BAD_CAST "time",
                                       BAD_CAST time_buf);
        if (rc < 0) {
            printf
                ("testXmlwriterFilename: Error at xmlTextWriterWriteFormatElement\n");
            return;
        }

        rc = xmlTextWriterWriteElement(writer, BAD_CAST "subject",
                                       BAD_CAST iter->subject.c_str());
        if (rc < 0) {
            printf
                ("testXmlwriterFilename: Error at xmlTextWriterWriteFormatElement\n");
            return;
        }

        rc = xmlTextWriterWriteElement(writer, BAD_CAST "content",
                                       BAD_CAST iter->content.c_str());
        if (rc < 0) {
            printf
                ("testXmlwriterFilename: Error at xmlTextWriterWriteFormatElement\n");
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