#include "check_stock.h"
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
#include "text_template.h"
#include "http_client.h"
#include "string_utils.h"

#include <list>
#include <string>
#include <map>

using namespace std;

typedef struct
{
    int id;
    const char *name;
} data_field_symbol_t;

#define DATA_TYPE_FIELD_CURRENT                     0
#define DATA_TYPE_FIELD_INCREASE                    1
#define DATA_TYPE_FIELD_INCREASE_RATIO              2
#define DATA_TYPE_FIELD_INCREASE_RATIO_FROM_NADIR   3
#define DATA_TYPE_FIELD_INCREASE_RATIO_FROM_ZENITH  4

data_field_symbol_t symbol_table[] = {
    {DATA_TYPE_FIELD_CURRENT, "{{CURRENT}}"},
    {DATA_TYPE_FIELD_INCREASE, "{{INCREASE}}"},
    {DATA_TYPE_FIELD_INCREASE_RATIO, "{{INCREASE_RATIO}}"},
    {DATA_TYPE_FIELD_INCREASE_RATIO_FROM_NADIR, "{{INCREASE_RATIO_FROM_NADIR}}"},
    {DATA_TYPE_FIELD_INCREASE_RATIO_FROM_ZENITH, "{{INCREASE_RATIO_FROM_ZENITH}}"},
};

typedef struct
{
// type
#define DATA_TYPE_INT       0
#define DATA_TYPE_DOUBLE    1
#define DATA_TYPE_FIELD     2
// field type
    int type;
    union {
        int i_data;
        double f_data;
    } u;
} data_item_t;

typedef struct
{
#define OPERATE_TYPE_NONE       0
#define OPERATE_TYPE_SMALL      1
#define OPERATE_TYPE_NOT_SMALL  2
#define OPERATE_TYPE_LARGE      3
#define OPERATE_TYPE_NOT_LARGE  4
    int op_type;
    data_item_t data1;
    data_item_t data2;
} basic_condition_t;

typedef struct {
    double open;
    double last_close;
    double current;
} basic_context_t;

typedef struct {
    basic_context_t basic;
    double nadir;
    double zenith;
} condition_context_t;

typedef struct
{
#define WARNING_ACTIVE          0
#define WARNING_NOT_ACTIVE      1
    int active_state;
    basic_condition_t warning;
    basic_condition_t reactive;
    std::string expression;
} stock_warning_t;

typedef struct
{
    int enable;
    string url;
    list<wild_time_t> check_time_list;
    list<stock_warning_t> warning_list;
    time_t last;
    string subject;
    string content;
} stock_outline_t;

list<stock_outline_t> g_stock_outline_list;
map<string, condition_context_t> g_stock_context_map;

void parse_basic_condition(basic_condition_t *condi, const char *content)
{
    memset(condi, 0, sizeof(basic_condition_t));

    const char *p;

    p = skip_blank(content);
    if(*p == '\0')
    {
        return;
    }

    unsigned int idx = 0;
    for(idx = 0; idx < sizeof(symbol_table)/sizeof(data_field_symbol_t); idx++)
    {
        const char *name = symbol_table[idx].name;
        if(0 == strncmp(p,
                        name,
                        strlen(name)))
        {
            condi->data1.type = DATA_TYPE_FIELD;
            condi->data1.u.i_data = symbol_table[idx].id;
            p += strlen(name);
            break;
        }
    }

    if(idx >= sizeof(symbol_table)/sizeof(data_field_symbol_t))
    {
        return;
    }

    p = skip_blank(p);
    if(*p == '\0')
    {
        return;
    }

    if(0 == strncmp(p, ">=", strlen(">=")))
    {
        condi->op_type = OPERATE_TYPE_NOT_SMALL;
        p += strlen(">=");
    }
    else if(0 == strncmp(p, ">", strlen(">")))
    {
        condi->op_type = OPERATE_TYPE_LARGE;
        p += strlen(">");
    }
    else if(0 == strncmp(p, "<=", strlen("<=")))
    {
        condi->op_type = OPERATE_TYPE_NOT_LARGE;
        p += strlen("<=");
    }
    else if(0 == strncmp(p, "<", strlen("<")))
    {
        condi->op_type = OPERATE_TYPE_SMALL;
        p += strlen("<");
    }
    else
    {
        condi->op_type = OPERATE_TYPE_NONE;
        return;
    }

    p = skip_blank(p);
    if(*p == '\0')
    {
        return;
    }

    if(NULL != strchr(p, '.'))
    {
        condi->data2.type = DATA_TYPE_DOUBLE;
        condi->data2.u.f_data = atof(p);
    }
    else
    {
        condi->data2.type = DATA_TYPE_INT;
        condi->data2.u.i_data = atoi(p);
    }
}

void parse_basic_warning_item(stock_warning_t *stock_warning,
                              const char *content)
{
    stock_warning->expression = content;
    stock_warning->active_state = 0;
    parse_basic_condition(&stock_warning->warning, content);
    stock_warning->reactive.op_type = OPERATE_TYPE_NONE;
}

void process_stock_list_node(xmlNodePtr node)
{
    stock_outline_t stock;
    stock.last = 0;
    stock.enable = 0;

    for(xmlNodePtr child = node->children; child != NULL; child = child->next)
    {
        if(child->type == XML_ELEMENT_NODE)
        {
            xmlChar *content = NULL;
            if(0 == strcasecmp((const char *)child->name, "enable"))
            {
                content = xmlNodeGetContent(child);
                stock.enable = atoi((const char *)content);
                printf("enable: %s\n", content);
            }
            else if(0 == strcasecmp((const char *)child->name, "url"))
            {
                content = xmlNodeGetContent(child);
                stock.url = (const char *)content;
                printf("last url: %s\n", content);
            }
            else if(0 == strcasecmp((const char *)child->name, "time"))
            {
                wild_time_t wild_time;
                content = xmlNodeGetContent(child);
                parse_wild_time_string(&wild_time,
                                       (const char *)content);
                stock.check_time_list.push_back(wild_time);
                printf("time: %s\n", content);
            }
            else if(0 == strcasecmp((const char *)child->name, "warning"))
            {
                stock_warning_t warning;
                content = xmlNodeGetContent(child);
                parse_basic_warning_item(&warning,
                                       (const char *)content);
                stock.warning_list.push_back(warning);
                printf("warning: %s\n", content);
            }
            else if(0 == strcasecmp((const char *)child->name, "subject"))
            {
                content = xmlNodeGetContent(child);
                stock.subject = (const char *)content;
                printf("subject: %s\n", content);
            }
            else if(0 == strcasecmp((const char *)child->name, "content"))
            {
                string s = parse_content_node(child);
                stock.content = s;
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

    g_stock_outline_list.push_back(stock);
}

static void process_stock_config(xmlTextReaderPtr reader)
{
    xmlNodePtr node;
    int ret;
    ret = xmlTextReaderRead(reader);
    while (ret == 1)
    {
        if(XML_READER_TYPE_ELEMENT == xmlTextReaderNodeType(reader))
        {
            node = xmlTextReaderExpand(reader);
            if(0 == strcasecmp((const char *)node->name, "stock"))
            {
                process_stock_list_node(node);
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

void load_stock_config_from_file(const char *filename)
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
        process_stock_config(reader);
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

void save_stock_config_to_file(const char *filename)
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

    std::list<stock_outline_t>::iterator iter;
    for(iter = g_stock_outline_list.begin();
            iter != g_stock_outline_list.end();
            iter++)
    {
        rc = xmlTextWriterStartElement(writer, BAD_CAST "stock");
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

        std::list<stock_warning_t>::iterator w_iter;
        for(w_iter = iter->warning_list.begin();
                w_iter != iter->warning_list.end();
                w_iter++)
        {
            rc = xmlTextWriterWriteElement(writer, BAD_CAST "warning",
                                           BAD_CAST w_iter->expression.c_str());
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

void create_stock_config_file(const char *filename)
{
    stock_outline_t stock;
    time_t next = time(NULL) + 30;
    wild_time_t wild_time;
    wild_time_set_fixed_time(&wild_time, next);
    stock.check_time_list.push_back(wild_time);
    stock_warning_t warning;
    warning.expression = "{{INCREASE_RATIO}} >= 0.01";
    stock.warning_list.push_back(warning);
    warning.expression = "{{INCREASE_RATIO}} <= -0.01";
    stock.warning_list.push_back(warning);
    stock.subject = "Stock Warning";
    stock.content = "Current price: {{CURRENT_PRICE}}";
    stock.url = "http://hq.sinajs.cn/list=sh000001";
    stock.enable = 1;
    g_stock_outline_list.push_back(stock);
    save_stock_config_to_file(filename);
}

double get_data_value(const data_item_t *item,
                           const condition_context_t *ctx)
{
    double r;
    if(item->type == DATA_TYPE_INT)
    {
        r = item->u.i_data;
    }
    else if(item->type == DATA_TYPE_DOUBLE)
    {
        r = item->u.f_data;
    }
    else if(item->type == DATA_TYPE_FIELD)
    {
        if(item->u.i_data == DATA_TYPE_FIELD_CURRENT)
        {
            r = ctx->basic.current;
        }
        else if(item->u.i_data == DATA_TYPE_FIELD_INCREASE)
        {
            r = ctx->basic.current - ctx->basic.last_close;
        }
        else if(item->u.i_data == DATA_TYPE_FIELD_INCREASE_RATIO)
        {
            double divider = 1;
            if(ctx->basic.last_close != 0)
            {
                divider = ctx->basic.last_close;
            }
            else
            {
                divider = ctx->basic.open;
            }

            r = (ctx->basic.current - divider)/divider;
        }
        else if(item->u.i_data == DATA_TYPE_FIELD_INCREASE_RATIO_FROM_NADIR)
        {
            r = (ctx->basic.current - ctx->nadir)/ctx->nadir;
        }
        else if(item->u.i_data == DATA_TYPE_FIELD_INCREASE_RATIO_FROM_ZENITH)
        {
            r = (ctx->basic.current - ctx->zenith)/ctx->zenith;
        }
        else
        {
            r = 0;
        }
    }
    else
    {
        r = 0;
    }

    return r;
}

bool check_basic_condition(const basic_condition_t *condi,
                           const condition_context_t *ctx)
{
    bool r;
    if(condi->op_type == OPERATE_TYPE_NONE)
    {
        r = false;
    }
    else if(condi->op_type == OPERATE_TYPE_SMALL)
    {
        r = get_data_value(&condi->data1, ctx) < get_data_value(&condi->data2, ctx);
    }
    else if(condi->op_type == OPERATE_TYPE_NOT_SMALL)
    {
        r = get_data_value(&condi->data1, ctx) >= get_data_value(&condi->data2, ctx);
    }
    else if(condi->op_type == OPERATE_TYPE_LARGE)
    {
        r = get_data_value(&condi->data1, ctx) > get_data_value(&condi->data2, ctx);
    }
    else if(condi->op_type == OPERATE_TYPE_NOT_LARGE)
    {
        r = get_data_value(&condi->data1, ctx) <= get_data_value(&condi->data2, ctx);
    }
    else
    {
        r = false;
    }
    return r;
}

bool check_stock_warning(stock_warning_t *warning, const condition_context_t *ctx)
{
    if(warning->active_state != WARNING_ACTIVE)
    {
        if(check_basic_condition(&warning->reactive, ctx))
        {
            warning->active_state = WARNING_ACTIVE;
        }
    }

    if(warning->active_state == WARNING_ACTIVE)
    {
        if(check_basic_condition(&warning->warning, ctx))
        {
            return true;
        }
        else
        {
            return false;
        }
    }
    else
    {
        return false;
    }
}

const char *match_next(const char *s, const char *pattern)
{
    s = skip_blank(s);
    if(*s == '\0')
    {
        return NULL;
    }
    if(0 == strncmp(s, pattern, strlen(pattern)))
    {
        return s;
    }
    else
    {
        return NULL;
    }
}

bool get_stock_basic_context(basic_context_t *ctx,
                   const char *url)
{
    bool r = false;
    memset(ctx, 0, sizeof(basic_context_t));
    char *text;
    const char *p;
    const char *end;
    do
    {
        text = http_request(url);

        p = strchr(text, '=');
        if(p == NULL)
        {
            r = false;
            break;
        }
        p += strlen("=");

        p = match_next(p, "\"");
        if(p == NULL)
        {
            r = false;
            break;
        }
        p += 1;

        p = strchr(p, ',');
        if(p == NULL)
        {
            r = false;
            break;
        }
        p += 1;

        end = strchr(p, ',');
        if(end != NULL)
        {
            ctx->open = atof_n(p, end - p);
        }
        p = end + 1;


        end = strchr(p, ',');
        if(end != NULL)
        {
            ctx->last_close = atof_n(p, end - p);
        }
        p = end + 1;

        end = strchr(p, ',');
        if(end != NULL)
        {
            ctx->current = atof_n(p, end - p);
        }

        r = true;
    }
    while(false);

    if(text != NULL)
    {
        free(text);
    }

    return r;
}

bool execute_check_stock(stock_outline_t *stock,
                                     time_t last_check,
                                     time_t current_time)
{
    condition_context_t ctx;
    basic_context_t basic_ctx;
    string url = stock->url;
    if(!get_stock_basic_context(&basic_ctx, url.c_str()))
    {
        return false;
    }
    if(basic_ctx.open == 0
       || basic_ctx.current == 0)
    {
        return false;
    }
    map<string, condition_context_t>::iterator condi_iter;
    condi_iter = g_stock_context_map.find(url);
    if(condi_iter != g_stock_context_map.end())
    {
        if(condi_iter->second.basic.open != basic_ctx.open
           || condi_iter->second.basic.last_close != basic_ctx.last_close)
        {
            condi_iter->second.basic = basic_ctx;
            condi_iter->second.nadir = basic_ctx.current;
            condi_iter->second.zenith = basic_ctx.current;
        }
        else
        {
            if(condi_iter->second.nadir > basic_ctx.current)
            {
                condi_iter->second.nadir = basic_ctx.current;
            }

            if(condi_iter->second.zenith < basic_ctx.current)
            {
                condi_iter->second.zenith = basic_ctx.current;
            }
        }
        ctx = condi_iter->second;
    }
    else
    {
        ctx.basic = basic_ctx;
        ctx.nadir = basic_ctx.current;
        ctx.zenith = basic_ctx.current;
        g_stock_context_map[url] = ctx;
    }

    std::list<stock_warning_t>::iterator iter;
    for(iter = stock->warning_list.begin();
            iter != stock->warning_list.end();
            iter++)
    {
        if(check_stock_warning(&(*iter), &ctx))
        {
            char buf[64];
            string subject = stock->subject;
            string content = stock->content;
            dict_t dict;
            sprintf(buf, "%.3f", ctx.basic.last_close);
            template_dict_set_pair(dict,
                                    "LAST_CLOSE_PRICE",
                                    buf);
            sprintf(buf, "%.3f", ctx.basic.open);
            template_replace(subject, dict);
                        template_dict_set_pair(dict,
                                    "OPEN_PRICE",
                                    buf);
            sprintf(buf, "%.3f", ctx.zenith);
            template_replace(subject, dict);
                        template_dict_set_pair(dict,
                                    "HIGH_PRICE",
                                    buf);
            sprintf(buf, "%.3f", ctx.nadir);
            template_replace(subject, dict);
                        template_dict_set_pair(dict,
                                    "LOW_PRICE",
                                    buf);
            sprintf(buf, "%.3f", ctx.basic.current);
            template_dict_set_pair(dict,
                                    "CURRENT_PRICE",
                                    buf);
            template_replace(subject, dict);
            template_replace(content, dict);
            notify(subject.c_str(),
                   content.c_str());
            iter->active_state = WARNING_NOT_ACTIVE;
        }
    }

    return true;
}

time_t process_stock_item(stock_outline_t *stock,
                                     time_t last_check,
                                     time_t current_time)
{
    time_t start;

    start = stock->last;
    if(start == 0)
    {
        if(last_check > 0)
        {
            start = last_check;
        }
        else
        {
            time_t period = wild_time_list_get_approx_period(stock->check_time_list);
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

    time_t next = wild_time_list_get_next_time(stock->check_time_list,
                                               start);
    if(next > start)
    {
        if(next <= current_time)
        {
            if(execute_check_stock(stock, start, current_time))
            {
                stock->last = current_time;

                return wild_time_list_get_next_time(stock->check_time_list,
                                            current_time);
            }
            else
            {
                // retry
                return current_time + 5;
            }
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

time_t process_stock_list(time_t current)
{
    time_t last_check;
    time_t next = current + 60*60;
    get_work_state_time("last_check_stock", &last_check);
    std::list<stock_outline_t>::iterator iter;
    for(iter = g_stock_outline_list.begin();
            iter != g_stock_outline_list.end();
            iter++)
    {
        if(iter->enable != 0)
        {
            time_t tmp_next = process_stock_item(&(*iter),
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
    set_work_state_time("last_check_stock", &current);
    return next;
}
