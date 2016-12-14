#ifndef SYS_UTILS_H_INCLUDED
#define SYS_UTILS_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

bool file_exists(const char *filename);
bool directory_file_exists(const char *dir,
                        const char *filename);
int create_dir_safely(const char *dir);
void get_default_config_directory(char *buf, int len);
void get_full_path(char *fullpath,
                   const char *dir,
                   const char *filename);

#ifdef __cplusplus
}
#endif

#endif // SYS_UTILS_H_INCLUDED
