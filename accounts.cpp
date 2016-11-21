#include "accounts.h"
#include <libxml/encoding.h>
#include <libxml/xmlwriter.h>
#include <libxml/xmlreader.h>
#include <string>
#include <stdio.h>
#include <string.h>

using namespace std;

char g_smtp_server[512];
char g_username[512];
char g_password[512];
std::list<std::string> g_receiver_list;

void process_sender_node(xmlNodePtr node)
{
    for(xmlNodePtr child = node->children; child != NULL; child = child->next)
    {
        if(child->type == XML_ELEMENT_NODE)
        {
            xmlChar *content = NULL;
            if(0 == strcasecmp((const char *)child->name, "smtp_server"))
            {
                content = xmlNodeGetContent(child);
                strcpy(g_smtp_server, (const char *)content);
                printf("sender smtp server: %s\n", content);
            }
            else if(0 == strcasecmp((const char *)child->name, "username"))
            {
                content = xmlNodeGetContent(child);
                strcpy(g_username, (const char *)content);
                printf("sender username: %s\n", content);
            }
            else if(0 == strcasecmp((const char *)child->name, "password"))
            {
                content = xmlNodeGetContent(child);
                strcpy(g_password, (const char *)content);
                printf("sender password: %s\n", xmlNodeGetContent(child));
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
}

void process_receiver_list_node(xmlNodePtr node)
{
    for(xmlNodePtr child = node->children; child != NULL; child = child->next)
    {
        if(child->type == XML_ELEMENT_NODE)
        {
            xmlChar *content = NULL;
            if(0 == strcasecmp((const char *)child->name, "account"))
            {
                content = xmlNodeGetContent(child);
                g_receiver_list.push_back((const char *)content);
                printf("receiver account: %s\n", content);
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
}

static void process_accounts_config(xmlTextReaderPtr reader)
{
    xmlNodePtr node;
    int ret;
    ret = xmlTextReaderRead(reader);
    while (ret == 1) {
        if(XML_READER_TYPE_ELEMENT == xmlTextReaderNodeType(reader))
        {
            node = xmlTextReaderExpand(reader);
            if(0 == strcasecmp((const char *)node->name, "sender"))
            {
                process_sender_node(node);
                ret = xmlTextReaderNext(reader);
            }
            else if(0 == strcasecmp((const char *)node->name, "to"))
            {
                process_receiver_list_node(node);
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

void load_accounts_from_file(const char *filename)
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
        process_accounts_config(reader);
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

void save_accounts_from_file(const char *filename)
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

    /* Start an element named "sender". */
    rc = xmlTextWriterStartElement(writer, BAD_CAST "sender");
    if (rc < 0) {
        printf
            ("testXmlwriterFilename: Error at xmlTextWriterStartElement\n");
        return;
    }

    /* Write an element named "smtp_server" as child of sender. */
    rc = xmlTextWriterWriteElement(writer, BAD_CAST "smtp_server",
                                   BAD_CAST g_smtp_server);
    if (rc < 0) {
        printf
            ("testXmlwriterFilename: Error at xmlTextWriterWriteFormatElement\n");
        return;
    }

    /* Write an element named "username" as child of sender. */
    rc = xmlTextWriterWriteElement(writer, BAD_CAST "username", BAD_CAST g_username);
    if (rc < 0) {
        printf
            ("testXmlwriterFilename: Error at xmlTextWriterWriteFormatElement\n");
        return;
    }

    /* Write an element named "password" as child of sender. */
    rc = xmlTextWriterWriteElement(writer, BAD_CAST "password", BAD_CAST g_password);
    if (rc < 0) {
        printf
            ("testXmlwriterFilename: Error at xmlTextWriterWriteFormatElement\n");
        return;
    }

    /* Close the element named sender. */
    rc = xmlTextWriterEndElement(writer);
    if (rc < 0) {
        printf
            ("testXmlwriterFilename: Error at xmlTextWriterEndElement\n");
        return;
    }

    /* Start an element named "to". */
    rc = xmlTextWriterStartElement(writer, BAD_CAST "to");
    if (rc < 0) {
        printf("testXmlwriterFilename: Error at xmlTextWriterStartElement\n");
        return;
    }

    for(std::list<std::string>::iterator iter = g_receiver_list.begin();
        iter != g_receiver_list.end();
        iter++)
    {
        /* Write an element named "account". */
        rc = xmlTextWriterWriteElement(writer, BAD_CAST "account",
                                       BAD_CAST iter->c_str());
        if (rc < 0) {
            printf("testXmlwriterFilename: Error at xmlTextWriterStartElement\n");
            break;
        }
    }

    /* Close the element named "to". */
    rc = xmlTextWriterEndElement(writer);
    if (rc < 0) {
        printf
            ("testXmlwriterFilename: Error at xmlTextWriterEndElement\n");
        return;
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
