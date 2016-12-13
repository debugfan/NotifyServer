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
#include "log.h"
#include "xml_utils.h"

using namespace std;

typedef struct {
    int enable;
    std::list<wild_time_t> time_list;
    time_t last;
    std::string subject;
    std::string content;
} reminder_t;

std::list<reminder_t> g_reminder_list;

void process_reminder_list_node(xmlNodePtr node)
{
    reminder_t reminder;
    reminder.last = 0;

    for(xmlNodePtr child = node->children; child != NULL; child = child->next)
    {
        if(child->type == XML_ELEMENT_NODE)
        {
            xmlChar *content = NULL;
            if(0 == strcasecmp((const char *)child->name, "enable"))
            {
                content = xmlNodeGetContent(child);
                reminder.enable = atoi((const char *)content);
                printf("enable: %s\n", content);
            }
            else if(0 == strcasecmp((const char *)child->name, "time"))
            {
                wild_time_t time;
                content = xmlNodeGetContent(child);
                parse_wild_time_string(&time,
                                      (const char *)content);
                reminder.time_list.push_back(time);
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
                string s = parse_content_node(child);
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

void save_reminders_to_file(const char *filename)
{
    int rc;
    xmlTextWriterPtr writer;
    char tmp_buf[512];

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

        sprintf(tmp_buf, "%s", iter->enable == 0 ? "0" : "1");
        rc = xmlTextWriterWriteElement(writer, BAD_CAST "enable",
                                       BAD_CAST tmp_buf);
        if (rc < 0)
        {
            printf
            ("testXmlwriterFilename: Error at xmlTextWriterWriteFormatElement\n");
            return;
        }

        list<wild_time_t>::iterator time_iter;
        for(time_iter = iter->time_list.begin();
            time_iter != iter->time_list.end();
            time_iter++)
        {
            format_wild_time(&(*time_iter), tmp_buf);
            rc = xmlTextWriterWriteElement(writer, BAD_CAST "time",
                                           BAD_CAST tmp_buf);
            if (rc < 0) {
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

void execute_reminder(reminder_t *reminder, time_t when)
{
    string subject = reminder->subject;
    string content = reminder->content;
    dict_t dict;
    template_dict_set_time(dict, &when);
    template_replace(subject, dict);
    template_replace(content, dict);
    log_printf(LOG_LEVEL_INFO,
           "[Notify] subject: %s.",
           subject.c_str());
    notify(subject.c_str(), content.c_str());
}

time_t process_reminder_item(reminder_t *reminder,
                             time_t last_check,
                             time_t current_time)
{
    time_t start;

    start = reminder->last;
    if(start == 0)
    {
        if(last_check > 0)
        {
            start = last_check;
        }
        else
        {
            time_t period = wild_time_list_get_approx_period(reminder->time_list);
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

    time_t next = wild_time_list_get_next_time(reminder->time_list, start);
    if(next > start)
    {
        if(next <= current_time)
        {
            execute_reminder(reminder, next);
            reminder->last = current_time;

            return wild_time_list_get_next_time(reminder->time_list,
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

time_t process_reminders(time_t check_time)
{
    time_t last_check;
    time_t next = check_time + 60*60;
    get_work_state_time("last_check_reminders",
                        &last_check);
    for(std::list<reminder_t>::iterator iter = g_reminder_list.begin();
        iter != g_reminder_list.end();
        iter++)
    {
        if(iter->enable != 0)
        {
            time_t tmp_next = process_reminder_item(&(*iter),
                                                    last_check,
                                                    check_time);
            if(tmp_next != 0 && tmp_next >= check_time)
            {
                if(next > tmp_next)
                {
                    next = tmp_next;
                }
            }
        }
    }
    set_work_state_time("last_check_reminders",
                    &check_time);
    return next;
}

void create_reminders_config_file(const char *filename)
{
    time_t next = time(NULL) + 30;
    reminder_t reminder;
    wild_time_t wild_time;
    set_wild_time(&wild_time, next, 0);
    reminder.time_list.push_back(wild_time);
    reminder.subject = "test";
    reminder.content = "test line 1\n"
        "test line 2\n"
        "test line 3\n";
    g_reminder_list.push_back(reminder);
    save_reminders_to_file("reminders.xml");
}

