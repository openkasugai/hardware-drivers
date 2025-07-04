/*************************************************
* Copyright 2025 NTT Corporation
* Licensed under the 3-Clause BSD License, see LICENSE for details.
* SPDX-License-Identifier: BSD-3-Clause
*************************************************/
/**
 * @file iddma_gpu.cu
 * @brief Implementation of iddma_gpu class.
 *
 */

#include <iddma_gpu.hpp>
#include <iddma_gpu.cuh>
#include <iddma_nv_dma.hpp>
#include <ftok_util.hpp>
#include <string.h>
#include <chrono>

static __global__ void iddma_gclose_global(iddma_device_object obj) {
    iddma_gclose(obj);
}

iddma_gpu::iddma_gpu(iddma_device_type device_type)
    : iddma_common(device_type),
      gobj_(nullptr),
      need_close_(nullptr),
      closed_(false),
      cpu_managed_(false),
      is_host_gpu_queue_(false),
      buffer_type_(KIDDMA_MEMTYPE_UNKNOWN) {
}

iddma_gpu::~iddma_gpu(void) {
    finalize();
}

void iddma_gpu::finalize(void) {
    iddma_common::finalize();

    if (gobj_) {
        cudaFree(gobj_);
        gobj_ = nullptr;
    }
    if (need_close_) {
        cudaFreeHost(need_close_);
        need_close_ = nullptr;
    }
    free_mmap_token();
}

void iddma_gpu::check_gpu_object_init(void) {
    if (!gobj_) {
        size_t gpu_object_size(sizeof(gpu_object));
        gpu_object_size = (gpu_object_size + 65535) & ~65535;
        if (cudaMalloc(&gobj_, gpu_object_size) != cudaSuccess) return;
        if (cudaMemcpy(&gobj_->queue_set, &queue_set_, sizeof(iddma_queue_set*),
                       cudaMemcpyHostToDevice) != cudaSuccess) {
            cudaFree(gobj_);
            gobj_ = nullptr;
            return;
        }
        if (cudaMemcpy(&gobj_->need_close, &need_close_, sizeof(bool*),
                       cudaMemcpyHostToDevice) != cudaSuccess) {
            cudaFree(gobj_);
            gobj_ = nullptr;
            return;
        }
        if (cudaMemcpy(&gobj_->transfer_mode, &transfer_mode_, sizeof(iddma_transfer_mode),
                       cudaMemcpyHostToDevice) != cudaSuccess) {
            cudaFree(gobj_);
            gobj_ = nullptr;
            return;
        }
    }
}

iddma_device_object iddma_gpu::get_device_object(void) {
    if (!check_initialization())
        return nullptr;
    if (cpu_managed_)
        return nullptr;
    check_gpu_object_init();
    if (!gobj_)
        return nullptr;
    return reinterpret_cast<iddma_device_object>(gobj_);
}

iddma_status iddma_gpu::create_memory_map(mmap_share_data& data, void* ptr) {
    iddma_mmap_memory_type type = data.mem_type;
    if (type == KIDDMA_MEMTYPE_HOST) {
        cudaPointerAttributes attr;
        cudaError_t cu_status;
        cu_status = cudaPointerGetAttributes(&attr, ptr);
        if (cu_status == cudaSuccess && attr.type == cudaMemoryTypeHost) {
            type = KIDDMA_MEMTYPE_HOST_PINNED;
        } else {
            auto err = cudaHostRegister(ptr, data.size, cudaHostRegisterPortable);
            if (err) {
                cudaGetLastError();
                return KIDDMA_ERROR_CUDA_HOST_REGISTER_FAILED;
            }
        }
    }
    if (memory_map_.add_map((void*)data.addr, ptr, data.size, (int)type) != KIDDMA_SUCCESS)
        return KIDDMA_ERROR_BUFFER_ADD_MAP_FAILED;
    return KIDDMA_SUCCESS;
}

iddma_status iddma_gpu::destroy_memory_map(void* ptr) {
    if (memory_map_.get_mem_type(ptr) == KIDDMA_MEMTYPE_HOST) {
        void* map_addr = memory_map_.translate_addr(ptr);
        auto err = cudaHostUnregister(map_addr);
        if (err) cudaGetLastError();
    }
    memory_map_.clear(ptr);
    return KIDDMA_SUCCESS;
}

iddma_status iddma_gpu::mmap_populate(void *addr, size_t size) {
    if (!check_connection())
        return KIDDMA_ERROR_NOT_CONNECTED;

    void* token;
    size_t token_size;
    iddma_mmap_memory_type mem_type = KIDDMA_MEMTYPE_DEVICE;

    iddma_status ret = cuda_util_->register_memory(addr, size, &token, &token_size, &mem_type);
    if (ret != KIDDMA_SUCCESS) return ret;

    if (buffer_type_ == KIDDMA_MEMTYPE_UNKNOWN && addr != reinterpret_cast<void *>(queue_set_)) {
        buffer_type_ = mem_type;
    }
    if (buffer_type_ != KIDDMA_MEMTYPE_UNKNOWN && mem_type != buffer_type_) {
        cuda_util_->unregister_memory(addr);
        return KIDDMA_ERROR_BUFFER_DIFFERENT_TYPE_SPECIFIED;
    }

    memory_map_export_.push_back(mmap_share_data({(uintptr_t)addr, size, (uintptr_t)token, token_size, mem_type}));
    return create_memory_map(memory_map_export_.back(), addr);
}

void iddma_gpu::free_mmap_token(void) {
    for (auto itr = memory_map_import_.begin(); itr != memory_map_import_.end(); itr++) {
        destroy_memory_map((void*)itr->addr);
        cuda_util_->release_addrs((void*)itr->token_ptr, itr->token_size);
    }
    for (auto itr = memory_map_export_.begin(); itr != memory_map_export_.end(); itr++) {
        destroy_memory_map((void*)itr->addr);
        cuda_util_->release_addrs((void*)itr->token_ptr, itr->token_size);
    }
    memory_map_.clear();
}

iddma_status iddma_gpu::post_mmap_share(void) {
    for (auto itr = memory_map_import_.begin(); itr != memory_map_import_.end(); itr++) {
        void *map_addr = nullptr;
        printf_debug("addr: 0x%lx, size: 0x%lx, token_ptr: 0x%lx, token_size: %ld\n",
                     itr->addr, itr->size, itr->token_ptr, itr->token_size);

        if (reinterpret_cast<void *>(itr->addr) != reinterpret_cast<void *>(counterpart_queue_set_)) {
            cp_buffer_type_ = itr->mem_type;
        }

        iddma_status ret;
        ret = cuda_util_->get_virt_addr((void*)itr->token_ptr, itr->token_size, itr->size, &map_addr);
        if (ret != KIDDMA_SUCCESS) {
            return ret;
        }
        ret = create_memory_map(*itr, map_addr);
        if (ret) return ret;
    }
    return KIDDMA_SUCCESS;
}

iddma_status iddma_gpu::enable_dma_engine(void) {
    if (dma_engine_.get()) return KIDDMA_SUCCESS;

    iddma_mmap_memory_type cp_queue_set_memtype =
        static_cast<iddma_mmap_memory_type>(memory_map_.get_mem_type(counterpart_queue_set_));

    if (tcp_info_.protocol == KIDDMA_DMA_PROTOCOL_DEFAULT &&
        (counterpart_device_type_ == KIDDMA_DEVICE_TYPE_CPU) ||
        (counterpart_device_type_ == KIDDMA_DEVICE_TYPE_GPU_NV &&
         buffer_type_ == KIDDMA_MEMTYPE_DEVICE &&
         cp_buffer_type_ != KIDDMA_MEMTYPE_DEVICE &&
         cp_queue_set_memtype != KIDDMA_MEMTYPE_DEVICE )) { // gpu - gpu: host managed communication
        bool send_enable = transfer_mode_ == KIDDMA_TRANSFER_MODE_SEND ||
            transfer_mode_ == KIDDMA_TRANSFER_MODE_BOTH;
        bool recv_enable = transfer_mode_ == KIDDMA_TRANSFER_MODE_RECV ||
            transfer_mode_ == KIDDMA_TRANSFER_MODE_BOTH;

        // TODO: select engine type
        try {
            iddma_engine* engine = new iddma_nv_dma(queue_set_, counterpart_queue_set_,
                                                    memory_map_, closed_, memory_map_export_, memory_map_import_,
                                                    send_enable, recv_enable, poll_interval_ns_);
            dma_engine_.reset(engine);
        } catch (...) {
            return KIDDMA_ERROR_DMA_ENGINE_CREATION_FAILED;
        }
    } else if (counterpart_device_type_ >= KIDDMA_DEVICE_TYPE_FPGA &&
               counterpart_device_type_ < KIDDMA_DEVICE_TYPE_FPGA_MAX) {
        // nop: fpga is dma engine
    } else if (counterpart_device_type_ == KIDDMA_DEVICE_TYPE_GPU_NV &&
               is_host_gpu_queue_ &&
               buffer_type_ != KIDDMA_MEMTYPE_DEVICE &&
               cp_buffer_type_ == KIDDMA_MEMTYPE_DEVICE) {
        // nop: cp's gpu is dma engine
    } else {
        // currently gpu (dev mem) - gpu (dev mem) falls here.
        // TODO: gpu-gpu only device-side communication :
        return KIDDMA_ERROR_UNSUPPORTED;
    }
    return iddma_common::enable_dma_engine();
}

void iddma_gpu::on_close(void) {
    *need_close_ = true;
    if (!(*(need_close_+1))) {
        if (cpu_managed_) {
            on_close_host();
        } else {
            check_gpu_object_init();
            cudaStream_t stream;
            cudaStreamCreateWithFlags(&stream, cudaStreamNonBlocking);
            iddma_gclose_global<<<1,1,0,stream>>>(gobj_);
            cudaStreamSynchronize(stream);
            cudaStreamDestroy(stream);
        }
    } else {
        closed_ = true;
    }
}

void iddma_gpu::on_close_host(void) {
    iddma_queue_element qe({0, 0, KIDDMA_QUEUE_STATUS_CLOSE_REQUEST, 0});
    const uint64_t poll_timing = poll_interval_ns_;
    uint64_t timeout_maxcount = 100000;
    uint64_t time_count = 0;
    struct timespec ts = {.tv_sec = poll_timing / 1000000000, .tv_nsec = poll_timing % 1000000000};
    if (!*(need_close_ + 1)) {
        iddma_queue_set *queue_set = queue_set_;
        bool srq_ready = false;
        bool rrq_ready = false;
        while (time_count < timeout_maxcount) {
            srq_ready = ((queue_set->srq.head + 1) & (MAX_QUEUE_SIZE - 1)) != queue_set->srq.tail;
            rrq_ready = ((queue_set->rrq.head + 1) & (MAX_QUEUE_SIZE - 1)) != queue_set->rrq.tail;
            if (srq_ready || rrq_ready) break;
            nanosleep(&ts, NULL);
            ++time_count;
        }
        if (srq_ready) iddma_add_queue_element(&queue_set->srq, &qe);
        if (rrq_ready) iddma_add_queue_element(&queue_set->rrq, &qe);
        *(need_close_ + 1) = true;
    }
}

iddma_status iddma_gpu::allocate_queue_set(iddma_queue_set **queue_set) {
    if (!(*queue_set)) {
        size_t queue_size(sizeof(iddma_queue_set));
        queue_size = (queue_size + 65535) & ~65535;
        if (!(cpu_managed_ || is_host_gpu_queue_)) {
            if (cudaMalloc(queue_set, queue_size) != cudaSuccess)
                return KIDDMA_ERROR_MALLOC_FAILED;
            if (cudaMemset(*queue_set, 0, sizeof(iddma_queue_set)) != cudaSuccess)
                return KIDDMA_ERROR_BUFFER_CUDAMEMSET_FAILED;
        } else {
            *queue_set = (iddma_queue_set *)aligned_alloc(4096, queue_size);
            if (!*queue_set)
                return KIDDMA_ERROR_MALLOC_FAILED;
            memset(*queue_set, 0, queue_size);
        }
    }

    if (!need_close_) {
        if (cudaMallocHost(&need_close_, sizeof(bool)*2) != cudaSuccess) {
            need_close_ = nullptr;
        } else {
            *need_close_ = false;
            *(need_close_+1) = false;
        }
    }

    return KIDDMA_SUCCESS;
}

void iddma_gpu::free_queue_set(iddma_queue_set *queue_set) {
    if (queue_set) {
        if (cpu_managed_ || is_host_gpu_queue_) {
            free(queue_set);
        }
        else
            cudaFree(queue_set);
    }
}

iddma_status iddma_gpu::send_and_recv(void *addr, size_t size_byte, uint64_t imm, bool is_send) {
    if (cpu_managed_) {
        if (iddma_check_connection() == KIDDMA_ERROR_NOT_CONNECTED)
            return KIDDMA_ERROR_NOT_CONNECTED;

        if (is_send && transfer_mode_ == KIDDMA_TRANSFER_MODE_RECV)
            return KIDDMA_ERROR_NOT_SUPPORTED_TRANSFER_DIRECTION_BY_TRANSFER_MODE;
        if (!is_send && transfer_mode_ == KIDDMA_TRANSFER_MODE_SEND)
            return KIDDMA_ERROR_NOT_SUPPORTED_TRANSFER_DIRECTION_BY_TRANSFER_MODE;

        iddma_queue_set *queue_set = queue_set_;

        iddma_status status = KIDDMA_SUCCESS;
        bool is_ready;
        if (is_send)
            is_ready = (((queue_set->srq.head + 1) & (MAX_QUEUE_SIZE - 1)) != queue_set->srq.tail);
        else
            is_ready = (((queue_set->rrq.head + 1) & (MAX_QUEUE_SIZE - 1)) != queue_set->rrq.tail);
        if (!is_ready)
            return KIDDMA_ERROR_QUEUE_IS_FULL;
        iddma_queue_element qe;
        qe.addr = (uint64_t)addr;
        qe.size = size_byte;
        qe.status = KIDDMA_QUEUE_STATUS_VALID;
        qe.imm = imm;
        if (is_send)
            status = iddma_add_queue_element(&queue_set->srq, &qe);
        else
            status = iddma_add_queue_element(&queue_set->rrq, &qe);

        dma_engine_->wakeup_workers();
        return status;
    }
    return KIDDMA_ERROR_QUEUE_MANAGED_ON_DEVICE;
}
iddma_status iddma_gpu::poll_send_and_recv(uint64_t timeout_sec, iddma_queue_element *data, bool is_send) {
    if (cpu_managed_) {
        if (iddma_check_connection() == KIDDMA_ERROR_NOT_CONNECTED)
            return KIDDMA_ERROR_NOT_CONNECTED;

        const uint64_t poll_timing = GPOLL_TIMING;
        uint64_t timeout_maxcount = timeout_sec * (1000000000 / poll_timing); // convert sec to how many count comes
        uint64_t time_count = 0;
        struct timespec ts = {.tv_sec = poll_timing / 1000000000, .tv_nsec = poll_timing % 1000000000};
        iddma_status status;
        if (timeout_maxcount == 0)
            timeout_maxcount = 1;

        dma_engine_->wakeup_workers();
        std::chrono::system_clock::time_point start_time = std::chrono::system_clock::now();

        while (time_count < timeout_maxcount) {
            status = iddma_check_completion_queue(queue_set_, is_send, data);
            if (status == KIDDMA_SUCCESS) break;

            std::chrono::system_clock::time_point temp_time = std::chrono::system_clock::now();
            long time_elapsed = std::chrono::duration_cast<std::chrono::seconds>(temp_time - start_time).count();
            if (time_elapsed >= timeout_sec) break;

            if (timeout_sec == 0) break;
            nanosleep(&ts, NULL);
            time_count++;
        }
        if (status == KIDDMA_ERROR_POLL_NO_VALID_ELEMENT)
            status = KIDDMA_ERROR_POLL_TIMEOUT;
        if (status == KIDDMA_SUCCESS && data->status == KIDDMA_QUEUE_STATUS_CLOSE_REQUEST) {
            *(need_close_+1) = true;
            return KIDDMA_ERROR_NOT_CONNECTED;
        }
        return status;
    }
    return KIDDMA_ERROR_QUEUE_MANAGED_ON_DEVICE;
}

bool iddma_gpu::check_option(const std::string& key, const std::string& value) {
    if (key == "cpu_manage" && value == "true") {
        cpu_managed_ = true;
    } else if (key == "host_queue" && value == "true") {
        is_host_gpu_queue_ = true;
    } else {
        return iddma_common::check_option(key, value);
    }
    return true;
}

// sub function implementation
iddma_status iddma_gpu::iddma_add_queue_element(iddma_queue *buffer, const iddma_queue_element *qe) {
    buffer->queue_elements[buffer->head] = *qe;
    iddma_move_pointer_forward(&buffer->head);
    return KIDDMA_SUCCESS;
}

iddma_status iddma_gpu::iddma_check_completion_queue(iddma_queue_set *queue_set, bool is_send, iddma_queue_element *c_data) {
    if (is_send) {
        bool is_ready = (queue_set->scq.head != queue_set->scq.tail);
        if (!is_ready)
            return KIDDMA_ERROR_POLL_NO_VALID_ELEMENT;
        *c_data = queue_set->scq.queue_elements[queue_set->scq.tail];
        iddma_move_pointer_forward(&queue_set->scq.tail);
    } else {
        bool is_ready = (queue_set->rcq.head != queue_set->rcq.tail);
        if (!is_ready)
            return KIDDMA_ERROR_POLL_NO_VALID_ELEMENT;
        *c_data = queue_set->rcq.queue_elements[queue_set->rcq.tail];
        iddma_move_pointer_forward(&queue_set->rcq.tail);
    }
    return KIDDMA_SUCCESS;
}

void iddma_gpu::iddma_move_pointer_forward(uint32_t *ptr) {
    *ptr = (*ptr + 1) & (MAX_QUEUE_SIZE - 1);
}

iddma_status iddma_gpu::iddma_check_connection(void) {
    if (*need_close_) {
        if (cpu_managed_) {
            on_close_host();
        } else {
            iddma_gclose_global<<<1,1>>>(gobj_);
            cudaDeviceSynchronize();
        }
        return KIDDMA_ERROR_NOT_CONNECTED;
    }
    return KIDDMA_SUCCESS;
}
