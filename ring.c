#include "ring.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

struct ring {
    size_t head; // next pointer to read
    size_t tail; // next pointer to write
    size_t size;
    char data[];
};

void *open_shm(shm_key *key, shm_id *id, void *addr, size_t size)
{
    shm_key _key;
    shm_id _id;
    do {
        _key = rand();
    } while (_key != 0 && (_id = shmget(_key, size, IPC_CREAT | IPC_EXCL | 0660)) == -1);

    *key = _key;
    *id = _id;
    return shmat(_id, addr, 0);
}

struct ring *get_ring_from_rd(ring_descriptor *rd) {
    return (struct ring *)rd->addr;
}

void ring_buffer_reset(ring_descriptor *rd) {
    struct ring *r = (struct ring *)rd->addr;
    r->head = 0;
    r->tail = 0;
}

ring_descriptor *ring_buffer_create(size_t size) {
    ring_descriptor *rd = (ring_descriptor *)malloc(sizeof(ring_descriptor));
    rd->size = size;
    rd->addr = open_shm(&rd->key, &rd->id, 0, size + sizeof(size_t) * 3);
    get_ring_from_rd(rd)->size = size;
    ring_buffer_reset(rd);
    return rd;
}

ring_descriptor *ring_buffer_attach(shm_key key, size_t size) {
    ring_descriptor *rd = (ring_descriptor *)malloc(sizeof(ring_descriptor));
    rd->key = key;
    rd->id = shmget(key, size, 0660);
    rd->addr = shmat(rd->id, 0, 0);
    rd->size = get_ring_from_rd(rd)->size;
}

void ring_buffer_detach(ring_descriptor *rd) {
    shmdt(rd->addr);
}

void ring_buffer_destroy(ring_descriptor *rd) {
    ring_buffer_detach(rd);
    shmctl(rd->id, IPC_RMID, 0);
}

size_t ring_wheel_tail(ring *r, size_t p) {
    r->tail = (r->tail + p) % r->size;
    return r->tail;
}

size_t ring_wheel_head(ring *r, size_t p) {
    r->head = (r->head + p) % r->size;
    return r->head;
}

size_t ring_buffer_write(ring_descriptor *rd, void *buf, size_t size) {
    struct ring *r = (struct ring *)rd->addr;
    size_t tail = r->tail;
    size_t head = r->head;
    size_t writable = head > tail ? (head - tail - 1) : (r->size + head - tail - 1);
    writable = size > writable ? writable : size;
    if (writable > 0) {
        if (tail < head) { // tail - head
            memcpy(r->data + tail, buf, writable);
            ring_wheel_tail(r, writable);
        } else { // head - tail - end
            size_t to_end = r->size - tail;
            if (to_end >= writable) {
                memcpy(r->data + tail, buf, writable);
                ring_wheel_tail(r, writable);
            } else {
                memcpy(r->data + tail, buf, to_end);
                memcpy(r->data, buf + to_end, writable - to_end);
                ring_wheel_tail(r, writable);
            }
        }
    }
#ifdef __DEBUG_RING__
    if (writable != 0)
    printf("Write: %d %d\n", r->head, r->tail);
#endif // __DEBUG_RING__
    return writable;
}

size_t ring_buffer_read(ring_descriptor *rd, void *buf, size_t size) {
    struct ring *r = (struct ring *)rd->addr;
    size_t tail = r->tail;
    size_t head = r->head;
    size_t readable = tail >= head ? (tail - head) : (r->size - head + tail);
    readable = size > readable ? readable : size;
    if (readable > 0) {
        if (tail >= head) { // head - tail
            memcpy(buf, r->data + head, readable);
            ring_wheel_head(r, readable);
        } else { // tail - head - end
            size_t to_end = r->size - head;
            if (to_end >= readable) {
                memcpy(buf, r->data + head, readable);
                ring_wheel_head(r, readable);
            } else {
                memcpy(buf, r->data + head, to_end);
                memcpy(buf + to_end, r->data, readable - to_end);
                ring_wheel_head(r, readable);
            }
        }
    }
#ifdef __DEBUG_RING__
    if (readable != 0)
    printf("READ: %d %d\n", r->head, r->tail);
#endif //__DEBUG_RING__
    return readable;
}
