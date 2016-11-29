#ifndef WEATHER_H_INCLUDED
#define WEATHER_H_INCLUDED

#include <time.h>

void load_weather_config_from_file(const char *filename);
void save_weather_config_to_file(const char *filename);
void create_weather_config_file(const char *filename);

time_t process_weather_forecast(time_t current);

#endif // WEATHER_H_INCLUDED
