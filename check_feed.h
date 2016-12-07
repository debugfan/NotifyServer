#ifndef CHECK_FEED_H_INCLUDED
#define CHECK_FEED_H_INCLUDED

#include <time.h>

void load_feed_config_from_file(const char *filename);
void save_feed_config_to_file(const char *filename);
void create_feed_config_file(const char *filename);

time_t process_feed_list(time_t current);

#endif // CHECK_FEED_H_INCLUDED
