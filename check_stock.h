#ifndef CHECK_STOCK_H_INCLUDED
#define CHECK_STOCK_H_INCLUDED

#include <time.h>

void load_stock_config_from_file(const char *filename);
void save_stock_config_to_file(const char *filename);
void create_stock_config_file(const char *filename);

time_t process_stock_list(time_t current);

#endif // CHECK_FEED_H_INCLUDED
