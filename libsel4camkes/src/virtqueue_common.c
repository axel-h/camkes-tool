/*
 * Copyright 2019, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <virtqueue.h>

#include <camkes/io.h>

#include "virtqueue_common.h"

struct vq_buf_alloc *init_vq_allocator(void *mem_pool, unsigned len, size_t block_size)
{
    ps_malloc_ops_t malloc_ops;
    int error = camkes_ps_malloc_ops(&malloc_ops);
    if (error) {
        return NULL;
    }

    struct vq_buf_alloc *allocator = NULL;

    error = ps_calloc(&malloc_ops, 1, sizeof(*allocator), (void **)&allocator);
    if (error) {
        return NULL;
    }

    allocator->buffer = mem_pool;
    allocator->len = len;
    allocator->free_list_size = len / block_size;
    allocator->block_size = block_size;
    if (ps_calloc(&malloc_ops, 1, allocator->free_list_size * sizeof(*(allocator->free_list)),
                  (void **)(&allocator->free_list))) {
        ZF_LOGE("Failed to allocate free_list");
        ps_free(&malloc_ops, sizeof(*allocator), allocator);
        return NULL;
    }
    unsigned i;
    for (i = 0; i < allocator->free_list_size; i++) {
        allocator->free_list[i] = i + 1;
    }
    allocator->head = 0;

    return allocator;
}

typedef struct {
    size_t offs_avail_ring;
    size_t offs_used_ring;
    size_t offs_desc;
    size_t offs_end;
} virtqueue_offsets_t;

static virtqueue_offsets_t calulate_virtqueue_offsets(unsigned queue_len)
{
    virtqueue_offsets_t vqo;

    size_t offs = 0;
    vqo.offs_avail_ring = offs;
    offs += sizeof(vq_vring_avail_t) +
            sizeof(vq_vring_avail_elem_t) * queue_len;

    vqo.offs_used_ring = offs;
    offs += sizeof(vq_vring_used_t) +
            sizeof(vq_vring_used_elem_t) * queue_len;

    vqo.offs_desc = offs;
    offs += sizeof(vq_vring_desc_t) * queue_len;

    vqo.offs_end = offs;

    return vqo;
}

int camkes_virtqueue_driver_init_common(virtqueue_driver_t *driver,
                                        volatile void *buffer,
                                        unsigned queue_len,
                                        size_t buffer_size,
                                        void (*notify)(void),
                                        size_t block_size)
{
    /* Don't check for notify, as it can be NULL in some situations */
    if (!driver || !buffer) {
        return EINVAL;
    }

    /* Calculate the offsets for the management structures. */
    virtqueue_offsets_t vqo = calulate_virtqueue_offsets(queue_len);
    /* Check if the buffer is big enough. */
    if (vqo.offs_end >= buffer_size) {
        ZF_LOGE("virtqueue buffer too small, have %zu, need at least %zu",
                buffer_size, vqo.offs_end);
        return ENOMEM;
    }

    /* The remaining buffer is available for queue data. */
    size_t vq_buffer_size = buffer_size - vqo.offs_end;
    void *vq_buffer = (void *)((uintptr_t)buffer + vqo.offs_end);
    struct vq_buf_alloc *allocator = init_vq_allocator(vq_buffer,
                                                       vq_buffer_size,
                                                       block_size);
    if (!allocator) {
        ZF_LOGE("virtqueue allocator initialization failed");
        return ENOMEM;
    }

    void *avail_ring = (void *)((uintptr_t)buffer + vqo.offs_avail_ring);
    void *used_ring = (void *)((uintptr_t)buffer + vqo.offs_used_ring);
    void *desc = (void *)((uintptr_t)buffer + vqo.offs_desc);
    virtqueue_init_driver(driver, queue_len, avail_ring, used_ring, desc,
                          notify, allocator);

    return 0;
}

int camkes_virtqueue_device_init_common(virtqueue_device_t *device,
                                        volatile void *buffer,
                                        unsigned queue_len,
                                        void (*notify)(void))
{
    /* Don't check for notify, as it can be NULL in some situations */
    if (!device || !buffer) {
        return EINVAL;
    }

    /* Calculate the offsets for the management structures. */
    virtqueue_offsets_t vqo = calulate_virtqueue_offsets(queue_len);

    /* The buffer size is not passed, so we can only hope it is big enough for
     * what we need.
     */

    void *avail_ring = (void *)((uintptr_t)buffer + vqo.offs_avail_ring);
    void *used_ring = (void *)((uintptr_t)buffer + vqo.offs_used_ring);
    void *desc = (void *)((uintptr_t)buffer + vqo.offs_desc);
    void *cookie = (void *)((uintptr_t)buffer + vqo.offs_end);
    virtqueue_init_device(device, queue_len, avail_ring, used_ring, desc,
                          notify, cookie);

    return 0;
}
