#include "pipe.h"

void pipe_init(pipe_t* p) {
    p->read_pos  = 0;
    p->write_pos = 0;
    p->count     = 0;
}

int pipe_write(pipe_t* p, uint8_t byte) {
    if (p->count == PIPE_SIZE)
        return -1;
    p->buf[p->write_pos] = byte;
    p->write_pos         = (p->write_pos + 1) % PIPE_SIZE;
    p->count++;
    return 0;
}

int pipe_read(pipe_t* p, uint8_t* out) {
    if (p->count == 0)
        return -1;
    *out        = p->buf[p->read_pos];
    p->read_pos = (p->read_pos + 1) % PIPE_SIZE;
    p->count--;
    return 0;
}
