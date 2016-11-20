#ifndef MEMORY_STREAM_H_INCLUDED
#define MEMORY_STREAM_H_INCLUDED

#include <stddef.h>

typedef struct _memomry_stream_t {
    size_t len;
    size_t offset;
    size_t buf_len;
    char *buf;
} memory_stream_t;

void memory_stream_init(memory_stream_t *stream);
size_t memory_stream_read(void *ptr, size_t size, size_t nmemb, memory_stream_t *stream);
size_t memory_stream_write(const void *ptr, size_t size, size_t nmemb, memory_stream_t *stream);
void memory_stream_rewind(memory_stream_t *stream);
void memory_stream_cleanup(memory_stream_t *stream);

#endif // MEMORY_STREAM_H_INCLUDED
