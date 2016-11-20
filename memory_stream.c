#include "memory_stream.h"
#include <stdlib.h>
#include <memory.h>
#include <stdio.h>

#ifndef MIN
#define MIN(X, Y) (((X) < (Y)) ? (X) : (Y))
#endif

void memory_stream_init(memory_stream_t *stream)
{
    memset(stream, 0, sizeof(memory_stream_t));
}

size_t memory_stream_write(const void *ptr, size_t size, size_t nmemb, memory_stream_t *stream)
{
    size_t write_len;

    if(ptr == 0 || size == 0 || nmemb == 0) {
        return 0;
    }

    write_len = size * nmemb;
    if(write_len <= 0) {
        return 0;
    }

    if(stream->offset + write_len > stream->buf_len)
    {
        size_t new_len = stream->buf_len;

        while(new_len < stream->offset + write_len) {
            if(new_len <= 0) {
                new_len = 256;
            }
            else {
                new_len = new_len*2;
            }
        }

        if(stream->buf == NULL || stream->buf_len <= 0)
        {
            stream->buf = malloc(new_len);
            if(stream->buf != NULL) {
                stream->buf_len = new_len;
            }
        }
        else {
            char *new_p = realloc(stream->buf, new_len);
            if(new_p != NULL) {
                stream->buf_len = new_len;
                stream->buf = new_p;
            }
        }
    }

    if(stream->offset + write_len <= stream->buf_len) {
        memcpy(stream->buf + stream->offset, ptr, write_len);
        stream->offset += write_len;
        stream->len = stream->offset;
    }

    return write_len;
}

size_t memory_stream_read(void *ptr, size_t size, size_t nmemb, memory_stream_t *stream)
{
    size_t n;
    size_t rest;

    if(ptr == 0 || size == 0 || nmemb == 0) {
        return 0;
    }

    rest = stream->len - stream->offset;
    n = MIN(size*nmemb, rest);
    if(n > 0) {
        memcpy(ptr, stream->buf + stream->offset, n);
        stream->offset += n;
    }

    return n;
}

void memory_stream_rewind(memory_stream_t *stream)
{
    stream->offset = 0;
}

void memory_stream_cleanup(memory_stream_t *stream)
{
    if(stream->buf != NULL) {
        free(stream->buf);
        stream->buf = NULL;
    }
    stream->buf_len = 0;
    stream->offset = 0;
    stream->len = 0;
}

