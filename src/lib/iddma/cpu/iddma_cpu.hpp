/*************************************************
* Copyright 2025 NTT Corporation
* Licensed under the 3-Clause BSD License, see LICENSE for details.
* SPDX-License-Identifier: BSD-3-Clause
*************************************************/
/**
 * @file iddma_cpu.hpp
 * @brief CPU specific object of iddma.
 *
 */

#ifndef __IDDMA_CPU_H__

#define __IDDMA_CPU_H__

#include <iddma_common.hpp>

class iddma_cpu : public iddma_common {
public:
    explicit iddma_cpu(iddma_device_type device_type);
    ~iddma_cpu(void);

    void finalize(void);

    iddma_device_object get_device_object(void);
    iddma_status mmap_populate(void *addr, size_t size);
private:
    // virtual function's definition
    // api's sub function
    iddma_status post_mmap_share(void);
    // allocation
    iddma_status allocate_queue_set(iddma_queue_set **queue_set);
    void free_queue_set(iddma_queue_set *queue_set);
    // queue management
    iddma_status send_and_recv(void *addr, size_t size_byte, uint64_t imm, bool is_send);
    iddma_status poll_send_and_recv(uint64_t timeout_sec, iddma_queue_element *data, bool is_send);
    //
    void on_close(void);

    // cpu specific function (non virtual)
    /*
    void free_mmap_token(void)
    free allocated buffer in mmap_from_export
    (ex. call cudaIpcCloseMemHandle())
    */
    void free_mmap_token(void);

    /*
    add a new element {addr, transfer_size_byte, status} to where buffer.data points to.
    */
    iddma_status add_queue_element(iddma_queue *buffer, iddma_queue_element *qe);

    /*
    iddma_status check_completion_queue(bool is_send, iddma_queue_element *data)
    is_send: true = send, false = recv
    data: completed queue element's written to this pointer
    if there is a queue element in send/recv completion queue, return the element to data
    */
    iddma_status check_completion_queue(bool is_send, iddma_queue_element *data);

    iddma_status enable_dma_engine(void);

    /*
    void move_pointer_forward(int *ptr, int max_size)
    ptr: pointer to head or tail
    max_size: queue's max size
    move head/tail forward by one according to the max size
    */
    void move_pointer_forward(uint32_t *ptr);

    // variables
    /*
    in post_mmap_share(),
    use memory_map_import_ to create a map
    and store the map to memory_map_
    */
    iddma_memory_map memory_map_;

#ifdef __KTP_DEBUG__
    double memcpy_time_;
    double update_memcpy_time_;
    int update_memcpy_count_;
#endif // __KTP_DEBUG__
    bool counterpart_close_;
};

#endif // __IDDMA_CPU_H__
