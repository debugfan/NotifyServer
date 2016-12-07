#ifndef STRTPTIME_H
#define STRTPTIME_H

#ifdef __cplusplus
extern "C" {
#endif

char *
strptime (const char *buf,
          const char *format,
          struct tm *tm);

#ifdef __cplusplus
}
#endif

#endif // STRTPTIME_H
