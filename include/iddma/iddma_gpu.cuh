/*************************************************
* Copyright 2025 NTT Corporation
* Licensed under the 3-Clause BSD License, see LICENSE for details.
* SPDX-License-Identifier: BSD-3-Clause
*************************************************/
/**
 * @file iddma_gpu.cuh
 * @brief GPU specific kernel functions.
 *
 */

#ifndef __IDDMA_GPU_CUH__
#define __IDDMA_GPU_CUH__

#ifdef __cplusplus
extern "C" {
#endif

#include <iddma_def.h>
#include <cuda.h>
#include <cuda_runtime.h>

#define GPOLL_TIMING 10000

// user interface
/**
 * @brief GPU kernel side iddma_send() function.
 *
 * @param [in] obj GPU side object to manipulate.
 * @param [in] addr Specify address to copy from.
 * @param [in] size_byte Buffer size in byte unit.
 * @return iddma_status
 *
 * @details
 * Posts a new send request specified by "addr" and "size_byte."
 * In the GPU kernel, users cannot call iddma_send(), so use iddma_gsend().
 */
__device__ iddma_status iddma_gsend(volatile iddma_device_object obj, const void *addr, size_t size_byte);

/**
 * @brief GPU kernel side iddma_send_with_imm() function.
 *
 * @param [in] obj GPU side object to manipulate.
 * @param [in] addr Specify address to copy from.
 * @param [in] size_byte Buffer size in byte unit.
 * @param [in] imm Specify 64bit immediate value.
 * @return iddma_status
 *
 * @details
 * Posts a new send request specified by "addr" and "size_byte."
 * In the GPU kernel, users cannot call iddma_send(), so use iddma_gsend().
 */
__device__ iddma_status iddma_gsend_with_imm(volatile iddma_device_object obj, const void *addr, size_t size_byte, uint64_t imm);

/**
 * @brief GPU kernel side iddma_recv() function.
 *
 * @param [in] obj GPU side object to manipulate.
 * @param [in] addr Specify address to copy to.
 * @param [in] size_byte Buffer size in byte unit.
 * @return iddma_status
 *
 * @details
 * Posts a new receive request specified by "addr" and "size_byte."
 * In the GPU kernel, users cannot call iddma_recv(), so use iddma_grecv().
 */
__device__ iddma_status iddma_grecv(volatile iddma_device_object obj, void *addr, size_t size_byte);

/**
 * @brief Do polling for a send request completion.
 *
 * @param [in] obj GPU side object to manipulate.
 * @param [in] timeout_sec Time in seconds for polling to time out.
 * @param [out] data A pointer to iddma_queue_element.
 * @return iddma_status
 *
 * @details
 * Polls for a send request completion.
 * After "timeout_sec" seconds, the polling times out.
 * If there is a completion, returns a completed request data.
 * In the GPU kernel, users cannot call iddma_poll_send(), so use iddma_gpoll_send().
 */
__device__ iddma_status iddma_gpoll_send(volatile iddma_device_object obj, uint64_t timeout_sec, iddma_queue_element *data);

/**
 * @brief Do polling for a receive request completion.
 *
 * @param [in] obj GPU side object to manipulate.
 * @param [in] timeout_sec Time in seconds for polling to time out.
 * @param [out] data A pointer to iddma_queue_element.
 * @return iddma_status
 *
 * @details
 * Polls for a receive request completion.
 * After "timeout_sec" seconds, the polling times out.
 * If there is a completion, returns a completed request data.
 * In the GPU kernel, users cannot call iddma_poll_recv(), so use iddma_gpoll_recv().
 */
__device__ iddma_status iddma_gpoll_recv(volatile iddma_device_object obj, uint64_t timeout_sec, iddma_queue_element *data);

// sub function
__device__ iddma_status iddma_gclose(volatile iddma_device_object obj);
__device__ iddma_status iddma_gadd_queue_element(volatile iddma_queue *buffer, const iddma_queue_element *qe);
__device__ iddma_status iddma_gcheck_completion_queue(volatile iddma_queue_set *queue_set, bool is_send, iddma_queue_element *data);
__device__ void iddma_gmove_pointer_forward(volatile uint32_t *ptr);
__device__ iddma_status iddma_gcheck_connection(iddma_device_object obj);

// user interface implementation
/** @struct iddma_gpu_object
 * GPU specific device object. Users can obtain this by iddma_get_device_object().
*/
typedef struct {
    //! Pointer to device-side iddma_queue_set.
    iddma_queue_set *queue_set;
    //! Pointer to the bool that indicates whether connection should be closed or not.
    bool *need_close;
    uint64_t transfer_mode;
} iddma_gpu_object;

__device__ iddma_status iddma_gsend(iddma_device_object obj, const void *addr, size_t size_byte) {
    return iddma_gsend_with_imm(obj, addr, size_byte, 0);
}

__device__ iddma_status iddma_gsend_with_imm(iddma_device_object obj, const void *addr, size_t size_byte, uint64_t imm) {
    if (!obj)
        return KIDDMA_ERROR_NOT_ALLOCATED;
    if (iddma_gcheck_connection(obj) == KIDDMA_ERROR_NOT_CONNECTED)
        return KIDDMA_ERROR_NOT_CONNECTED;
    volatile iddma_gpu_object *gobj = (iddma_gpu_object *)obj;
    volatile iddma_queue_set *queue_set = gobj->queue_set;
    if (gobj->transfer_mode == 2) // If recv mode, return error
        return KIDDMA_ERROR_NOT_SUPPORTED_TRANSFER_DIRECTION_BY_TRANSFER_MODE;

    iddma_status status = KIDDMA_SUCCESS;
    bool is_ready = (((queue_set->srq.head + 1) & (MAX_QUEUE_SIZE - 1)) != queue_set->srq.tail);
    if (!is_ready)
        return KIDDMA_ERROR_QUEUE_IS_FULL;
    iddma_queue_element qe;
    qe.addr = (uint64_t)addr;
    qe.size = size_byte;
    qe.status = KIDDMA_QUEUE_STATUS_VALID;
    qe.imm = imm;
    status = iddma_gadd_queue_element(&queue_set->srq, &qe);

    return status;
}

__device__ iddma_status iddma_grecv(iddma_device_object obj, void *addr, size_t size_byte) {
    if (!obj)
        return KIDDMA_ERROR_NOT_ALLOCATED;
    if (iddma_gcheck_connection(obj) == KIDDMA_ERROR_NOT_CONNECTED)
        return KIDDMA_ERROR_NOT_CONNECTED;
    volatile iddma_gpu_object *gobj = (iddma_gpu_object *)obj;
    volatile iddma_queue_set *queue_set = gobj->queue_set;
    if (gobj->transfer_mode == 1) // If send mode, return error
        return KIDDMA_ERROR_NOT_SUPPORTED_TRANSFER_DIRECTION_BY_TRANSFER_MODE;

    iddma_status status = KIDDMA_SUCCESS;
    bool is_ready = (((queue_set->rrq.head + 1) & (MAX_QUEUE_SIZE - 1)) != queue_set->rrq.tail);
    if (!is_ready)
        return KIDDMA_ERROR_QUEUE_IS_FULL;
    iddma_queue_element qe = {(uint64_t)addr, size_byte, KIDDMA_QUEUE_STATUS_VALID};
    iddma_gadd_queue_element(&queue_set->rrq, &qe);

    return status;
}

__device__ iddma_status iddma_gpoll_send_recv(iddma_device_object obj, uint64_t timeout_sec, iddma_queue_element *data, bool is_send) {
    if (!obj)
        return KIDDMA_ERROR_NOT_ALLOCATED;
    if (iddma_gcheck_connection(obj) == KIDDMA_ERROR_NOT_CONNECTED)
        return KIDDMA_ERROR_NOT_CONNECTED;
    volatile iddma_gpu_object *gobj = (iddma_gpu_object *)obj;
    volatile iddma_queue_set *queue_set = gobj->queue_set;

    const uint64_t poll_timing = GPOLL_TIMING;
    uint64_t timeout_maxcount = timeout_sec * (1000000000 / poll_timing); // convert sec to how many count comes
    uint64_t time_count = 0;
    if (timeout_maxcount == 0)
        timeout_maxcount = 1;
    iddma_status status;
    while (time_count < timeout_maxcount) {
        long poll_remain = poll_timing;
        status = iddma_gcheck_completion_queue(queue_set, is_send, data);
        if (status == KIDDMA_SUCCESS) break;
        if (timeout_sec == 0) break;
        while (poll_remain > 0) {
            if (poll_remain > 1000000) {
                __nanosleep(1000000);
                poll_remain -= 1000000;
            } else {
                __nanosleep(poll_remain);
                break;
            }
        }
        time_count++;
    }
    if (status == KIDDMA_ERROR_POLL_NO_VALID_ELEMENT)
        status = KIDDMA_ERROR_POLL_TIMEOUT;
    if (status == KIDDMA_SUCCESS && data->status == KIDDMA_QUEUE_STATUS_CLOSE_REQUEST) {
        *(gobj->need_close+1) = true;
        return KIDDMA_ERROR_NOT_CONNECTED;
    }
    return status;
}

__device__ iddma_status iddma_gpoll_send(iddma_device_object obj, uint64_t timeout_sec, iddma_queue_element *data) {
    return iddma_gpoll_send_recv(obj, timeout_sec, data, true);
}

__device__ iddma_status iddma_gpoll_recv(iddma_device_object obj, uint64_t timeout_sec, iddma_queue_element *data) {
    return iddma_gpoll_send_recv(obj, timeout_sec, data, false);
}

// sub function implementation
__device__ iddma_status iddma_gadd_queue_element(volatile iddma_queue *buffer, const iddma_queue_element *qe) {
    memcpy((iddma_queue_element *)&buffer->queue_elements[buffer->head], qe, sizeof(iddma_queue_element));
    iddma_gmove_pointer_forward(&buffer->head);
    return KIDDMA_SUCCESS;
}

__device__ void iddma_gcopy_queue_element(volatile iddma_queue_element* dst, volatile iddma_queue_element* src) {
    dst->size = src->size;
    dst->addr = src->addr;
    dst->status = src->status;
    dst->imm = src->imm;
}

__device__ iddma_status iddma_gcheck_completion_queue(volatile iddma_queue_set *queue_set, bool is_send, iddma_queue_element *c_data) {
    if (is_send) {
        bool is_ready = (queue_set->scq.head != queue_set->scq.tail);
        if (!is_ready)
            return KIDDMA_ERROR_POLL_NO_VALID_ELEMENT;
        iddma_gcopy_queue_element(c_data, &(queue_set->scq.queue_elements[queue_set->scq.tail]));
        iddma_gmove_pointer_forward(&queue_set->scq.tail);
    } else {
        bool is_ready = (queue_set->rcq.head != queue_set->rcq.tail);
        if (!is_ready)
            return KIDDMA_ERROR_POLL_NO_VALID_ELEMENT;
        iddma_gcopy_queue_element(c_data, &(queue_set->rcq.queue_elements[queue_set->rcq.tail]));
        iddma_gmove_pointer_forward(&queue_set->rcq.tail);
    }

    return KIDDMA_SUCCESS;
}

__device__ void iddma_gmove_pointer_forward(volatile uint32_t *ptr) {
    *ptr = (*ptr + 1) & (MAX_QUEUE_SIZE - 1);
}

__device__ iddma_status iddma_gcheck_connection(volatile iddma_device_object obj) {
    volatile iddma_gpu_object *gobj = (iddma_gpu_object *)obj;
    bool flag = *(bool *)gobj->need_close;
    if (flag) {
        iddma_gclose(obj);
        return KIDDMA_ERROR_NOT_CONNECTED;
    }
    return KIDDMA_SUCCESS;
}

__device__ iddma_status iddma_gclose(volatile iddma_device_object obj) {
    volatile iddma_gpu_object *gobj = (iddma_gpu_object *)obj;
    bool flag2 = *(gobj->need_close + 1);
    if (!obj)
        return KIDDMA_ERROR_NOT_ALLOCATED;
    if (!flag2) {
        iddma_queue_set *queue_set = gobj->queue_set;
        iddma_queue_element close_qe;
        close_qe.size = 0;
        close_qe.addr = 0;
        close_qe.status = KIDDMA_QUEUE_STATUS_CLOSE_REQUEST;

        bool srq_vacant = ((queue_set->srq.head + 1) & (MAX_QUEUE_SIZE - 1)) != queue_set->srq.tail;
        bool rrq_vacant = ((queue_set->rrq.head + 1) & (MAX_QUEUE_SIZE - 1)) != queue_set->rrq.tail;
        if (srq_vacant) iddma_gadd_queue_element(&queue_set->srq, &close_qe);
        if (rrq_vacant) iddma_gadd_queue_element(&queue_set->rrq, &close_qe);
        // todo: timeout
        while (!srq_vacant && !rrq_vacant) {
            srq_vacant = ((queue_set->srq.head + 1) & (MAX_QUEUE_SIZE - 1)) != queue_set->srq.tail;
            rrq_vacant = ((queue_set->rrq.head + 1) & (MAX_QUEUE_SIZE - 1)) != queue_set->rrq.tail;
            __nanosleep(GPOLL_TIMING);
        }
        if (srq_vacant) iddma_gadd_queue_element(&queue_set->srq, &close_qe);
        if (rrq_vacant) iddma_gadd_queue_element(&queue_set->srq, &close_qe);
        *(gobj->need_close + 1) = true;
    }
    return KIDDMA_SUCCESS;
}

#ifdef __cplusplus
}
#endif

#endif
