#include "weather.h"
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

#include <list>
#include <string>

using namespace std;

typedef struct
{
    double temp;
    double temp_min;
    double temp_max;
    double pressure;
    double humidity;
    double wind_speed;
    double wind_deg;
    string weather;
    string weather_desc;
} weather_t;

typedef struct
{
    int enable;
    string city;
    string url;
    list<wild_time_t> check_time_list;
    time_t last;
} weather_forecast_t;

list<weather_forecast_t> g_weather_forecast_list;

void process_weather_list_node(xmlNodePtr node)
{
    weather_forecast_t forecast;
    forecast.last = 0;
    forecast.enable = 0;

    for(xmlNodePtr child = node->children; child != NULL; child = child->next)
    {
        if(child->type == XML_ELEMENT_NODE)
        {
            xmlChar *content = NULL;
            if(0 == strcasecmp((const char *)child->name, "enable"))
            {
                content = xmlNodeGetContent(child);
                forecast.enable = atoi((const char *)content);
                printf("enable: %s\n", content);
            }
            else if(0 == strcasecmp((const char *)child->name, "city"))
            {
                content = xmlNodeGetContent(child);
                forecast.city = (const char *)content;
                printf("city: %s\n", content);
            }
            else if(0 == strcasecmp((const char *)child->name, "url"))
            {
                content = xmlNodeGetContent(child);
                forecast.url = (const char *)content;
                printf("url: %s\n", content);
            }
            else if(0 == strcasecmp((const char *)child->name, "time"))
            {
                wild_time_t wild_time;
                content = xmlNodeGetContent(child);
                parse_wild_time_string(&wild_time,
                                       (const char *)content);
                forecast.check_time_list.push_back(wild_time);
                printf("time: %s\n", content);
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

    g_weather_forecast_list.push_back(forecast);
}

static void process_weather_config(xmlTextReaderPtr reader)
{
    xmlNodePtr node;
    int ret;
    ret = xmlTextReaderRead(reader);
    while (ret == 1)
    {
        if(XML_READER_TYPE_ELEMENT == xmlTextReaderNodeType(reader))
        {
            node = xmlTextReaderExpand(reader);
            if(0 == strcasecmp((const char *)node->name, "weather_forecast"))
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
    if (reader != NULL)
    {
        process_weather_config(reader);
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

void save_weather_config_to_file(const char *filename)
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

    std::list<weather_forecast_t>::iterator iter;
    for(iter = g_weather_forecast_list.begin();
            iter != g_weather_forecast_list.end();
            iter++)
    {
        rc = xmlTextWriterStartElement(writer, BAD_CAST "weather_forecast");
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

        rc = xmlTextWriterWriteElement(writer, BAD_CAST "city",
                                       BAD_CAST iter->city.c_str());
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

void create_weather_config_file(const char *filename)
{
    weather_forecast_t warning;
    time_t next = time(NULL) + 30;
    wild_time_t wild_time;
    set_wild_time(&wild_time, next, 0);
    warning.check_time_list.push_back(wild_time);
    warning.city = "abc";
    warning.enable = 1;
    warning.url = "http://www.example.com/request.php?a=1&b=2";
    g_weather_forecast_list.push_back(warning);
    save_weather_config_to_file(filename);
}

int parse_weather_object(weather_t *weather, json_t *entry)
{
    json_t *main = NULL;
    json_t *param = NULL;
    weather->weather = "";
    weather->weather_desc = "";

    main = json_object_get(entry, "main");
    if(main != NULL && json_is_object(main))
    {
        json_t *tmp;
        tmp = json_object_get(main, "temp");
        if(tmp != NULL && json_is_real(tmp))
        {
            weather->temp =  json_real_value(tmp);
        }
        tmp = json_object_get(main, "temp_min");
        if(tmp != NULL && json_is_real(tmp))
        {
            weather->temp_min = json_real_value(tmp);
        }
        tmp = json_object_get(main, "temp_max");
        if(tmp != NULL && json_is_real(tmp))
        {
            weather->temp_min = json_real_value(tmp);
        }
        tmp = json_object_get(main, "pressure");
        if(tmp != NULL && json_is_real(tmp))
        {
            weather->pressure = json_real_value(tmp);
        }
    }

    json_t *param_array = json_object_get(entry, "weather");
    if(param_array != NULL && json_is_array(param_array)
       && json_array_size(param_array) > 0)
    {
        param = json_array_get(param_array, 0);
        if(param != NULL && json_is_object(param))
        {
            json_t *tmp;
            tmp = json_object_get(param, "main");
            if(tmp != NULL && json_is_string(tmp))
            {
                weather->weather = json_string_value(tmp);
            }
            tmp = json_object_get(param, "description");
            if(tmp != NULL && json_is_string(tmp))
            {
                weather->weather_desc = json_string_value(tmp);
            }
        }
    }
    return 0;
}

int parse_weather_json_list(list<weather_t> &weather_list, json_t *root, time_t current_time)
{
    if(!json_is_object(root))
    {
        json_decref(root);
        return 1;
    }

    json_t *object_list = json_object_get(root, "list");

    if(!json_is_array(object_list))
    {
        fprintf(stderr, "error: root is not an array\n");
        json_decref(root);
        return 1;
    }

    int n = json_array_size(object_list);
    for(int i = 0; i < n; i++)
    {
        json_t *data, *dt;
        weather_t weather;
        time_t v_dt;

        data = json_array_get(object_list, i);
        if(!json_is_object(data))
        {
            json_decref(root);
            return 1;
        }

        dt = json_object_get(data, "dt");
        if(json_is_integer(dt))
        {
            const char *tmp = json_string_value(dt);
            v_dt = atoi(tmp);
        }

        if(v_dt > current_time + 60*60*24)
        {
            break;
        }

        parse_weather_object(&weather, data);
        weather_list.push_back(weather);
    }
    return 0;
}


double weather_list_get_max_temp(const list<weather_t> &weather_list)
{
    double v_max = -DBL_MAX;
    list<weather_t>::iterator iter;
    for(iter == weather_list.begin();
            iter != weather_list.end();
            iter++)
    {
        if(iter->temp_max > v_max)
        {
            v_max = iter->temp_max;
        }
    }
    return v_max;
}

double weather_list_get_min_temp(const list<weather_t> &weather_list)
{
    double v_min = DBL_MAX;
    list<weather_t>::iterator iter;
    for(iter == weather_list.begin();
            iter != weather_list.end();
            iter++)
    {
        if(iter->temp_min < v_min)
        {
            v_min = iter->temp_min;
        }
    }
    return v_min;
}

bool weather_list_has_rain(const list<weather_t> &weather_list)
{
    bool has_rain = false;
    list<weather_t>::const_iterator iter;
    for(iter = weather_list.begin();
            iter != weather_list.end();
            iter++)
    {
        if(0 == strcasecmp(iter->weather.c_str(), "rain"))
        {
            has_rain = true;
        }
    }
    return has_rain;
}

bool weather_list_has_snow(const list<weather_t> &weather_list)
{
    bool has_snow = false;
    list<weather_t>::const_iterator iter;
    for(iter = weather_list.begin();
            iter != weather_list.end();
            iter++)
    {
        if(0 == strcasecmp(iter->weather.c_str(), "snow"))
        {
            has_snow = true;
        }
    }
    return has_snow;
}

int execute_weather_forcast(weather_forecast_t *forecast,
                                     time_t last_check,
                                     time_t current_time)
{
    json_t *root;
    json_error_t error;
    const char *text;
    list<weather_t> weather_list;

    text = http_request(forecast->url.c_str());
    if(text != NULL)
    {
        root = json_loads(text, 0, &error);
        free((void *)text);
    }
    else
    {
        return -1;
    }

    if(root == NULL)
    {
        fprintf(stderr, "error: on line %d: %s\n", error.line,
                error.text);
        return -1;
    }

    parse_weather_json_list(weather_list, root, current_time);
    json_decref(root);

    string subject = "";
    string content = "";
    if(weather_list_has_rain(weather_list))
    {
        content = "There will be rain in 24 hours";
    }
    else if(weather_list_has_rain(weather_list))
    {
        content = "There will be snow in 24 hours";
    }

    if(content.length() > 0)
    {
        subject = "weather forecast";
        notify(subject.c_str(), content.c_str());
    }

    return 0;
}

time_t process_weather_forecast_item(weather_forecast_t *forecast,
                                     time_t last_check,
                                     time_t current_time)
{
    time_t start;

    start = forecast->last;
    if(start == 0)
    {
        if(last_check > 0)
        {
            start = last_check;
        }
        else
        {
            start = current_time - 60*60*8;
        }
    }

    if(start > current_time)
    {
        start = current_time;
    }

    time_t next = wild_time_list_get_next_time(forecast->check_time_list,
                                               start);
    if(next > start)
    {
        if(next <= current_time)
        {
            execute_weather_forcast(forecast, last_check, current_time);
            forecast->last = current_time;

            return wild_time_list_get_next_time(forecast->check_time_list,
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

time_t process_weather_forecast(time_t current)
{
    time_t last_check;
    time_t next = current + 60*60;
    get_work_state_time("last_check_weather", &last_check);
    std::list<weather_forecast_t>::iterator iter;
    for(iter = g_weather_forecast_list.begin();
            iter != g_weather_forecast_list.end();
            iter++)
    {
        time_t tmp_next = process_weather_forecast_item(&(*iter),
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
    set_work_state_time("last_check_weather", &current);
    return next;
}
