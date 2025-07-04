/*************************************************
* Copyright 2025 NTT Corporation
* Licensed under the 3-Clause BSD License, see LICENSE for details.
* SPDX-License-Identifier: BSD-3-Clause
*************************************************/
/**
 * @file iddma.cpp
 * @brief iddma user interface implementation
 *
 */

#include <iddma.h>
#include <iddma_common.hpp>
#include <iddma_memory.hpp>
#ifdef __USE_CPU__
#include <iddma_cpu.hpp>
#endif
#ifdef __USE_GPU_NV__
#include <iddma_gpu.hpp>
#endif
#ifdef __USE_FPGA_XSE__
#include <iddma_fpga_xse.hpp>
#endif

#include <map>
#include <memory>

static std::map<iddma_object, std::unique_ptr<iddma_common> > s_instances_;

iddma_object iddma_create_object(iddma_device_type dev_type, const char** options) {
    try {
        iddma_common *obj = nullptr;
#ifdef __USE_CPU__
        if (dev_type == KIDDMA_DEVICE_TYPE_CPU) {
            obj = new iddma_cpu(dev_type);
        }
#endif
#ifdef __USE_GPU_NV__
        if (dev_type == KIDDMA_DEVICE_TYPE_GPU_NV) {
            obj = new iddma_gpu(dev_type);
        }
#endif
#ifdef __USE_FPGA_XSE__
        if (dev_type == KIDDMA_DEVICE_TYPE_FPGA_XSE) {
            obj = new iddma_fpga_xse(dev_type);
        }
#endif
        if (!obj) return obj;

        obj->initialize(options);
        s_instances_[obj].reset(obj);
        return (iddma_object) obj;
    } catch (...) {
        return nullptr;
    }
}

iddma_status iddma_destroy_object(iddma_object obj) {
    if (!obj)
        return KIDDMA_ERROR_INVALID_ARGUMENT;
    try {
        auto it = s_instances_.find(obj);
        if (it != s_instances_.end()) {
            s_instances_.erase(it);
        } else {
            return KIDDMA_ERROR_INVALID_ARGUMENT;
        }
        return KIDDMA_SUCCESS;
    } catch (...) {
        return KIDDMA_ERROR_UNKNOWN_EXCEPTION;
    }
}

iddma_status iddma_send(iddma_object obj, const void *addr, size_t size_byte) {
    return iddma_send_with_imm(obj, addr, size_byte, 0);
}

iddma_status iddma_send_with_imm(iddma_object obj, const void *addr, size_t size_byte, uint64_t imm) {
    if (!obj)
        return KIDDMA_ERROR_NOT_ALLOCATED;
    try {
        iddma_common *iddma_obj = reinterpret_cast<iddma_common*>(obj);
        return iddma_obj->send(addr, size_byte, imm);
    } catch (...) {
        return KIDDMA_ERROR_UNKNOWN_EXCEPTION;
    }
}

iddma_status iddma_recv(iddma_object obj, void *addr, size_t size_byte) {
    if (!obj)
        return KIDDMA_ERROR_NOT_ALLOCATED;
    try {
        iddma_common *iddma_obj = reinterpret_cast<iddma_common*>(obj);
        return iddma_obj->recv(addr, size_byte);
    } catch (...) {
        return KIDDMA_ERROR_UNKNOWN_EXCEPTION;
    }
}

iddma_status iddma_poll_send(iddma_object obj, uint64_t timeout_sec, iddma_queue_element *data) {
    if (!obj)
        return KIDDMA_ERROR_NOT_ALLOCATED;
    try {
        iddma_common *iddma_obj = reinterpret_cast<iddma_common*>(obj);
        return iddma_obj->poll_send(timeout_sec, data);
    } catch (...) {
        return KIDDMA_ERROR_UNKNOWN_EXCEPTION;
    }
}

iddma_status iddma_poll_recv(iddma_object obj, uint64_t timeout_sec, iddma_queue_element *data) {
    if (!obj)
        return KIDDMA_ERROR_NOT_ALLOCATED;
    try {
        iddma_common *iddma_obj = reinterpret_cast<iddma_common*>(obj);
        return iddma_obj->poll_recv(timeout_sec, data);
    } catch (...) {
        return KIDDMA_ERROR_UNKNOWN_EXCEPTION;
    }
}

iddma_device_object iddma_get_device_object(iddma_object obj) {
    if (!obj)
        return nullptr;
    try {
        iddma_common *iddma_obj = reinterpret_cast<iddma_common*>(obj);
        return iddma_obj->get_device_object();
    } catch (...) {
        return nullptr;
    }
}

iddma_status iddma_listen(iddma_object obj, int id, const char *directory) {
    if (!obj)
        return KIDDMA_ERROR_NOT_ALLOCATED;
    try {
        iddma_common *iddma_obj = reinterpret_cast<iddma_common*>(obj);
        if (directory == NULL)
            return iddma_obj->listen(id, "./");
        else
            return iddma_obj->listen(id, std::string(directory));
    } catch (...) {
        return KIDDMA_ERROR_UNKNOWN_EXCEPTION;
    }
}

iddma_status iddma_accept(iddma_object obj) {
    if (!obj)
        return KIDDMA_ERROR_NOT_ALLOCATED;
    try {
        iddma_common *iddma_obj = reinterpret_cast<iddma_common*>(obj);
        return iddma_obj->accept();
    } catch (...) {
        return KIDDMA_ERROR_UNKNOWN_EXCEPTION;
    }
}

iddma_status iddma_connect(iddma_object obj, int id, const char *directory) {
    if (!obj)
        return KIDDMA_ERROR_NOT_ALLOCATED;
    try {
        iddma_common *iddma_obj = reinterpret_cast<iddma_common*>(obj);
        if (directory == NULL)
            return iddma_obj->connect(id, "./");
        else
            return iddma_obj->connect(id, std::string(directory));
    } catch (...) {
        return KIDDMA_ERROR_UNKNOWN_EXCEPTION;
    }
}

iddma_status iddma_close(iddma_object obj) {
    if (!obj)
        return KIDDMA_ERROR_NOT_ALLOCATED;
    try {
        iddma_common *iddma_obj = reinterpret_cast<iddma_common*>(obj);
        return iddma_obj->close();
    } catch (...) {
        return KIDDMA_ERROR_UNKNOWN_EXCEPTION;
    }
}

iddma_status iddma_mmap_populate(iddma_object obj, void *addr, size_t size_byte) {
    if (!obj)
        return KIDDMA_ERROR_NOT_ALLOCATED;
    try {
        iddma_common *iddma_obj = reinterpret_cast<iddma_common*>(obj);
        return iddma_obj->mmap_populate(addr, size_byte);
    } catch (...) {
        return KIDDMA_ERROR_UNKNOWN_EXCEPTION;
    }
}

iddma_status iddma_mmap_share(iddma_object obj) {
    if (!obj)
        return KIDDMA_ERROR_NOT_ALLOCATED;
    try {
        iddma_common *iddma_obj = reinterpret_cast<iddma_common*>(obj);
        return iddma_obj->mmap_share();
    } catch (...) {
        return KIDDMA_ERROR_UNKNOWN_EXCEPTION;
    }
}

void *iddma_mem_allocate(size_t size, iddma_memory_type type, size_t align) {
    iddma_memory *instance = iddma_memory::get_instance();
    return instance->allocate_top(size, type, align);
}

void iddma_mem_free(void *addr) {
    iddma_memory *instance = iddma_memory::get_instance();
    instance->free_top(addr);
}
