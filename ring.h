#ifndef __IPC__
#define __IPC__
typedef int sema_key;
typedef int sema_id;
typedef int shm_key;
typedef int shm_id;

typedef struct ringDesc {
    shm_key key;
    shm_id id;
    void *addr;
    size_t size;
} ring_descriptor;

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>

struct ring *get_ring_from_rd(ring_descriptor *rd);
void ring_buffer_reset(ring_descriptor *rd);
ring_descriptor *ring_buffer_create(size_t size);
ring_descriptor *ring_buffer_attach(shm_key key, size_t size);
void ring_buffer_detach(ring_descriptor *rd);
void ring_buffer_destroy(ring_descriptor *rd);
size_t ring_buffer_write(ring_descriptor *rd, void *buf, size_t size);
size_t ring_buffer_read(ring_descriptor *rd, void *buf, size_t size);

#endif
