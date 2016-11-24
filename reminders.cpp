#include "reminders.h"
#include <libxml/encoding.h>
#include <libxml/xmlwriter.h>
#include <libxml/xmlreader.h>
#include <string.h>
#include "notify.h"
#include <windows.h>
#include "wild_time.h"
#include "work_state.h"
#include "string_utils.h"
#include "text_template.h"

using namespace std;

std::list<reminder_t> g_reminder_list;

string process_content_node(xmlNodePtr node)
{
    string r = "";
    for(xmlNodePtr child = node->children; child != NULL; child = child->next)
    {
        xmlChar *content = NULL;
        if(child->type == XML_ELEMENT_NODE)
        {
            if(0 == strcasecmp((const char *)child->name, "p"))
            {
                content = xmlNodeGetContent(child);
                r += string((const char *)content) + "\r\n";
            }
        }
        if(content != NULL)
        {
            xmlFree(content);
        }
    }
    return r;
}

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
                printf("subject: %s\n", content);
            }
            else if(0 == strcasecmp((const char *)child->name, "content"))
            {
                string s = process_content_node(child);
                reminder.content = s;
                printf("content: %s\n", s.c_str());
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

time_t get_recent_remind_time(reminder_t *reminder, time_t current)
{
    return wild_time_get_recent_time(&reminder->time, current);
}

time_t get_next_remind_time(reminder_t *reminder, time_t start)
{
    return wild_time_get_next_time(&reminder->time, start);
}

void process_reminders()
{
    int wait = 0;
    time_t last_check = 0;
    get_work_state_time("last_check_reminders", &last_check);
    while(true) {
        wait = 60*60;
        time_t start;
        time_t now = time(NULL);
        for(std::list<reminder_t>::iterator iter = g_reminder_list.begin();
            iter != g_reminder_list.end();
            iter++)
        {
            start = iter->last;
            if(start == 0)
            {
                if(last_check > 0)
                {
                    start = last_check;
                }
                else
                {
                    start = now - 60*60*24*180;
                }
            }
            if(start > now)
            {
                start = now;
            }
            time_t next = get_next_remind_time(&(*iter), start);
            if(next > start)
            {
                if(next <= now)
                {
                    string subject = iter->subject;
                    string content = iter->content;
                    std::map<std::string, std::string> dict;
                    template_dict_set_time(dict, &next);
                    template_replace(subject, dict);
                    template_replace(content, dict);
                    notify(subject.c_str(), content.c_str());
                    iter->last = now;
                    wait = 0;
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
            set_work_state_time("last_check_reminders", &now);
            printf("Sleep for (seconds): %d\n", wait);
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

        rc = xmlTextWriterStartElement(writer, BAD_CAST "content");
        if (rc < 0) {
            printf
                ("testXmlwriterFilename: Error at xmlTextWriterWriteFormatElement\n");
            return;
        }

        std::vector<std::string> string_list;
        split_string(string_list, iter->content, "\n");
        for(std::vector<std::string>::iterator s_iter = string_list.begin();
            s_iter != string_list.end();
            s_iter++)
        {
            rc = xmlTextWriterWriteElement(writer, BAD_CAST "p",
                                           BAD_CAST s_iter->c_str());
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
