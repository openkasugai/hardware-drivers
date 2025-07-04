/*************************************************
* Copyright 2025 NTT Corporation
* Licensed under the 3-Clause BSD License, see LICENSE for details.
* SPDX-License-Identifier: BSD-3-Clause
*************************************************/
/**
 * @file iddma_gpu.hpp
 * @brief GPU specific object of iddma.
 *
 */

#ifndef __IDDMA_GPU_H__

#define __IDDMA_GPU_H__

#include <iddma_common.hpp>
#include <vector>
#include <cuda_runtime.h>
#include <iddma_cuda_util.hpp>

class iddma_gpu : public iddma_common {
public:
    explicit iddma_gpu(iddma_device_type device_type);
    ~iddma_gpu(void);

    void finalize(void);

    iddma_device_object get_device_object(void);
    iddma_status mmap_populate(void *addr, size_t size);

private:
    struct gpu_object {
        iddma_queue_set *queue_set;
        bool *need_close;
        iddma_transfer_mode transfer_mode;
    };

    // virtual functions definition
    iddma_status post_mmap_share(void);
    iddma_status enable_dma_engine(void);

    // api's sub function
    void on_close(void);
    void on_close_host(void);
    // allocation
    iddma_status allocate_queue_set(iddma_queue_set **queue_set);
    void free_queue_set(iddma_queue_set *queue_set);
    // queue management
    iddma_status send_and_recv(void *addr, size_t size_byte, uint64_t imm, bool is_send);
    iddma_status poll_send_and_recv(uint64_t timeout_sec, iddma_queue_element *data, bool is_send);

    void free_mmap_token(void);
    iddma_status create_memory_map(mmap_share_data& data, void* ptr);
    iddma_status destroy_memory_map(void* ptr);

    // gpu specific function
    void check_gpu_object_init(void);
    bool check_option(const std::string& key, const std::string& value);
    iddma_status iddma_add_queue_element(iddma_queue *buffer, const iddma_queue_element *qe);
    iddma_status iddma_check_completion_queue(iddma_queue_set *queue_set, bool is_send, iddma_queue_element *c_data);
    void iddma_move_pointer_forward(uint32_t *ptr);
    iddma_status iddma_check_connection(void);

    // variables
    iddma_memory_map memory_map_;
    std::map<void *, std::unique_ptr<cudaIpcMemHandle_t> > ipc_mem_handles_;
    struct gpu_object *gobj_;
    bool *need_close_;
    bool closed_;

    bool cpu_managed_;
    bool is_host_gpu_queue_;
    iddma_mmap_memory_type buffer_type_;
    iddma_mmap_memory_type cp_buffer_type_;
    // iddma_status *cpu_managed_status_;
    // iddma_queue_element *cpu_managed_qe_;
};

#endif
