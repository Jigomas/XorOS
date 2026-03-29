#ifndef PIPE_H
#define PIPE_H

#include <stdint.h>

#define PIPE_SIZE 16  // ring buffer capacity, bytes

typedef struct {
    uint8_t  buf[PIPE_SIZE];
    uint32_t read_pos;
    uint32_t write_pos;
    uint32_t count;
} pipe_t;

void pipe_init(pipe_t* p);

// returns 0 on success, -1 if full (non-blocking)
int pipe_write(pipe_t* p, uint8_t byte);

// returns 0 on success and stores byte in *out, -1 if empty (non-blocking)
int pipe_read(pipe_t* p, uint8_t* out);

#endif /* PIPE_H */
