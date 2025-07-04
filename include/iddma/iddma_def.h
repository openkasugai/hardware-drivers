/*************************************************
* Copyright 2025 NTT Corporation
* Licensed under the 3-Clause BSD License, see LICENSE for details.
* SPDX-License-Identifier: BSD-3-Clause
*************************************************/
/**
 * @file iddma_def.h
 * @brief This file defines enums and constants used in iddma.
 *
 */

#ifndef __IDDMA_ENUM_H__
#define __IDDMA_ENUM_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __KTP_DEBUG__
/** @def printf_debug
 * When debug is active, print a formated strings.
*/
#define printf_debug(fmt, ...) printf(fmt, ## __VA_ARGS__)
/** @def perror_debug
 * When debug is active, print a formated strings when error occurred.
*/
#define perror_debug(fmt, ...) fprintf(stderr, fmt, ## __VA_ARGS__)
#else
#define printf_debug(fmt, ...)
#define perror_debug(fmt, ...)
#endif

/** @def MAX_QUEUE_SIZE
 * Max queue depth for
 * srq (send request queue),
 * rrq (recv request queue),
 * scq (send completion queue),
 * rcq (recv completion queue).
 */
#define MAX_QUEUE_SIZE 8

/** @enum iddma_status
 * iddma internal functions and user API basically returns this enum type.
 * This indicates if the function encounters an error or not.
 * Without any problem, you will receive KIDDMA_SUCCESS.
*/
typedef enum {
    //! Success
    KIDDMA_SUCCESS = 0,
    //! Returns when fails to open a device in a function.
    KIDDMA_ERROR_OPEN_DEVICE_FAILED = -2,
    //! Returns when malloc() failed in a function.
    KIDDMA_ERROR_MALLOC_FAILED = -3,
    //! Returns when queue is full and cannot push add an element.
    KIDDMA_ERROR_QUEUE_IS_FULL = -6,
    //! Returns when a memory region used in a function is not allocated.
    KIDDMA_ERROR_NOT_ALLOCATED = -7,
    //! Returns when a file used by ftok already exists at the time of its creation.
    KIDDMA_ERROR_FILE_ALREADY_EXISTS = -8,
    //! Returns when a file used by ftok does not exist at the time of opening.
    KIDDMA_ERROR_FILE_NOT_EXIST = -9,
    //! Returns when failed creating a new file (ex. a file used by ftok()).
    KIDDMA_ERROR_FILE_CREATE = -10,
    //! Returns when failed removing a file (ex. a file used by ftok()).
    KIDDMA_ERROR_REMOVE_FAILED = -11,
    //! Returns when opening a ftok() file times out (ex. due to "file does not exist").
    KIDDMA_ERROR_FILE_CHECK_TIMEOUT = -12,
    //! Returns when failed acquiring a key by ftok().
    KIDDMA_ERROR_ACQUIRE_KEY_FAILED = -13,
    //! Returns when shmget() failed in ftok utility.
    KIDDMA_ERROR_SHMGET = -14,
    //! Returns when shmat() failed in ftok utility.
    KIDDMA_ERROR_SHMAT = -15,
    //! Returns when connect, accept times out.
    KIDDMA_ERROR_CONNECTION_TIMEOUT = -16,
    //! Returns when not connected to another process or connection is lost.
    //! Use this to detect another process completion when receiving streamed data.
    KIDDMA_ERROR_NOT_CONNECTED = -17,
    //! Returns when a function is not ready to execute. (ex. use connect() before initialize())
    KIDDMA_ERROR_INVALID_OPERATION = -18,
    //! Returns when poll_send() / poll_recv() times out.
    KIDDMA_ERROR_POLL_TIMEOUT = -19,
    //! Returns when arguments are invalid.
    KIDDMA_ERROR_INVALID_ARGUMENT = -20,
    //! Returns when dma protocol mismatched.
    KIDDMA_ERROR_MISMATCH_PROTOCOL = -21,
    //! Returns when a completion queue is empty.
    KIDDMA_ERROR_POLL_NO_VALID_ELEMENT = -22,
    //! Returns when sending size is 0.
    KIDDMA_ERROR_NETWORK_BUF_FULL = -30,
    //! Returns when sending failed
    KIDDMA_ERROR_NETWORK_SEND_FAILED = -31,
    //! Returns when receiving failed
    KIDDMA_ERROR_NETWORK_RECV_FAILED = -32,
    //! Returns when DMA engine creation failed.
    KIDDMA_ERROR_DMA_ENGINE_CREATION_FAILED = -33,
    //! Returns when sharing queue element failed.
    KIDDMA_ERROR_SHARE_QUEUE_ELEMENT_FAILED = -34,
    //! Returns when sharing queue head/tail failed.
    KIDDMA_ERROR_SHARE_QUEUE_HEAD_TAIL_FAILED = -35,
    //! Returns when an invalid device type selected.
    KIDDMA_ERROR_INVALID_DEVICE_TYPE = -40,
    //! Returns when an invalid token specified.
    KIDDMA_ERROR_INVALID_TOKEN = -41,
    //! Returns when executing connect() after listen() completed.
    KIDDMA_ERROR_ALREADY_ACCEPTOR = -50,
    //! Returns when executing listen(), accept() after connect() completed.
    KIDDMA_ERROR_ALREADY_CONNECTOR = -51,
    //! Returns when shmdt() failed in ftok utility.
    KIDDMA_ERROR_SHMEM_DETACH_FAILED = -54,
    //! Returns when shmctl with IPC_RMID failed.
    KIDDMA_ERROR_SHMEM_RELEASE_FAILED = -55,
    //! Returns when address translation failed
    KIDDMA_ERROR_ADDRESS_TRANSLATION = -60,
    //! Returns when failed to open the inter-process communication.
    KIDDMA_ERROR_BUFFER_IPC_OPEN_FAILED = -61,
    //! Returns when trying transfering a buffer that is not mapped by mmap_populate() and mmap_share().
    KIDDMA_ERROR_BUFFER_NOT_MAPPED = -62,
    //! Returns when cudaMemcpy failed.
    KIDDMA_ERROR_BUFFER_CUDAMEMCPY_FAILED = -63,
    //! Returns when failed importing counterpart's mmap data.
    KIDDMA_ERROR_BUFFER_MMAP_IMPORT_FAILED = -64,
    //! Returns when failed exporting mmap data to counterpart.
    KIDDMA_ERROR_BUFFER_MMAP_EXPORT_FAILED = -65,
    //! Returns when failed adding a new element to iddma_memory_map.
    KIDDMA_ERROR_BUFFER_ADD_MAP_FAILED = -66,
    //! Returns when failed with specifying a different memory type for mmap_populate()
    KIDDMA_ERROR_BUFFER_DIFFERENT_TYPE_SPECIFIED = -67,
    //! Returns when failed to get an object for inter-process conmunication.
    KIDDMA_ERROR_BUFFER_IPC_GET_FAILED = -68,
    //! Returns when failed to pin a buffer.
    KIDDMA_ERROR_BUFFER_PIN_FAILED = -69,
    //! Returns when failed to unpin a buffer.
    KIDDMA_ERROR_BUFFER_UNPIN_FAILED = -70,
    //! Returns when failed to get a virtual address of the buffer.
    KIDDMA_ERROR_BUFFER_GET_VADDR_FAILED = -71,
    //! Returns when failed to get physical addresses of the buffer.
    KIDDMA_ERROR_BUFFER_GET_PADDR_FAILED = -72,
    //! Returns when failed to release address information of the buffer.
    KIDDMA_ERROR_BUFFER_RELEASE_ADDR_FAILED = -73,
    //! Returns when cudaMemset failed.
    KIDDMA_ERROR_BUFFER_CUDAMEMSET_FAILED = -74,
    //! Returns when memcpy failed.
    KIDDMA_ERROR_MEMCPY_FAILED = -75,
    //! Returns when device setup failed.
    KIDDMA_ERROR_SETUP_FAILED = -76,
    //! Returns when device finishing failed.
    KIDDMA_ERROR_FINISH_FAILED = -77,
    //! Returns when synchronization failed.
    KIDDMA_ERROR_SYNC_FAILED = -78,
    //! Returns when queue is managed on device side
    KIDDMA_ERROR_QUEUE_MANAGED_ON_DEVICE = -80,
    //! Returns when selected transfer mode does not support the transfer direction
    KIDDMA_ERROR_NOT_SUPPORTED_TRANSFER_DIRECTION_BY_TRANSFER_MODE = -81,
    //! Returns when DMA engine to the device initialization failed
    KIDDMA_ERROR_INIT_DMA_ENGINE_TO_DEV_FAILED = -82,
    //! Returns when DMA engine from the device initialization failed
    KIDDMA_ERROR_INIT_DMA_ENGINE_FROM_DEV_FAILED = -83,
    //! Returns when DMA engine to the device starting failed
    KIDDMA_ERROR_START_DMA_ENGINE_TO_DEV_FAILED = -84,
    //! Returns when DMA engine from the device starting failed
    KIDDMA_ERROR_START_DMA_ENGINE_FROM_DEV_FAILED = -85,
    //! Returns when route controller initialization failed
    KIDDMA_ERROR_INIT_ROUTE_CONTROLLER_FAILED = -86,
    //! Returns when route controller rx stream initialization failed
    KIDDMA_ERROR_INIT_ROUTE_CONTROLLER_RX_FAILED = -87,
    //! Returns when route controller tx stream initialization failed
    KIDDMA_ERROR_INIT_ROUTE_CONTROLLER_TX_FAILED = -88,
    //! Returns when doorbell address setting failed
    KIDDMA_ERROR_SET_DOORBELL_ADDR_FAILED = -89,
    //! Returns when queue information setting failed
    KIDDMA_ERROR_SET_QUEUE_INFO_FAILED = -90,
    //! Returns when vp map setting failed
    KIDDMA_ERROR_SET_VPMAP_FAILED = -91,
    //! Returns when cudaHostRegister failed
    KIDDMA_ERROR_CUDA_HOST_REGISTER_FAILED = -99,
    //! Returns when an unknown exception occurred.
    KIDDMA_ERROR_UNKNOWN_EXCEPTION = -100,
    //! Returns when a function is unsupported.
    KIDDMA_ERROR_UNSUPPORTED = -250,
    //! Returns when a function is not implemented.
    KIDDMA_ERROR_NOT_IMPLEMENTED = -255,
} iddma_status;

/** @enum iddma_queue_status
 * This indicates how a queue element is handled.
 * Users can get this through iddma_queue_element.status returned by iddma_poll_send()/iddma_poll_recv().
*/
typedef enum {
    //! Returns when a queue element is invalid.
    KIDDMA_QUEUE_STATUS_INVALID = 0,
    //! Returns when a queue element is valid.
    KIDDMA_QUEUE_STATUS_VALID = 1,
    //! Returns when a queue element is processed correctly.
    KIDDMA_QUEUE_STATUS_SUCCESS = 2,
    //! Returns when a queue element is not handled correctly. (when failed transfering a buffer)
    KIDDMA_QUEUE_STATUS_FAIL = 3,
    //! Returns when a recv buffer is smaller than a send buffer.
    KIDDMA_QUEUE_STATUS_NOT_ENOUGH_BUFFER = 4,
    KIDDMA_QUEUE_STATUS_CLOSE_REQUEST = 5,
} iddma_queue_status;

/** @enum iddma_device_type
 * This enum indicates the device type user tempting to use.
 * Currently, KIDDMA_DEVICE_TYPE_GPU_NV and KIDDMA_DEVICE_TYPE_CPU supported.
 * Users can set a device type by iddma_create_object().
*/
typedef enum {
    //! (Not used) Indicates any devices.
    KIDDMA_DEVICE_TYPE_ANY = 0,
    //! (Not used) Indicates any GPU devices.
    KIDDMA_DEVICE_TYPE_GPU_ANY = 1,
    //! Indicates NVIDIA GPU devices.
    KIDDMA_DEVICE_TYPE_GPU_NV = 2,
    //! (Not used) Indicates AMD GPU devices.
    KIDDMA_DEVICE_TYPE_GPU_AMD = 3,
    //! Indicates FPGA devices.
    KIDDMA_DEVICE_TYPE_FPGA = 10,
    //! Indicates FPGA with XSE devices.
    KIDDMA_DEVICE_TYPE_FPGA_XSE = 12,
    //! Indicates end of FPGA devices.
    KIDDMA_DEVICE_TYPE_FPGA_MAX,
    //! Indicates CPU emulated devices.
    KIDDMA_DEVICE_TYPE_CPU = 20,
} iddma_device_type;

/** @enum iddma_memory_type
 * This enum indicates the memory type user tempting to use.
 *
*/
#define KIDDMA_PAGESIZE_IDX_DEFAULT (0)
#define KIDDMA_PAGESIZE_IDX_4KB (1)
#define KIDDMA_PAGESIZE_IDX_2MB (2)
#define KIDDMA_PAGESIZE_IDX_1GB (3)
#define KIDDMA_PAGESIZE_IDX_SHIFT (16)
typedef enum {
    //! (Not used) Indicates any memory.
    KIDDMA_MEMORY_TYPE_ANY = 0,
    //! (Not used) Indicates any GPU memory.
    KIDDMA_MEMORY_TYPE_GPU_ANY = 1,
    //! Indicates NVIDIA GPU device memory.
    KIDDMA_MEMORY_TYPE_GPU_NV = 2,
    //! (Not used) Indicates AMD GPU device memory.
    KIDDMA_MEMORY_TYPE_GPU_AMD = 3,
    //! Indicates FPGA device memory.
    KIDDMA_MEMORY_TYPE_FPGA = 10,
    //! Indicates CPU memory.
    KIDDMA_MEMORY_TYPE_CPU = 20,
    //! (Not used) Indicates CPU shared memory.
    KIDDMA_MEMORY_TYPE_CPU_HUGEPAGE_SHMEM = 21,
    KIDDMA_MEMORY_TYPE_CPU_HUGEPAGE_SHMEM_2MB = 21 | (KIDDMA_PAGESIZE_IDX_2MB << KIDDMA_PAGESIZE_IDX_SHIFT),
    KIDDMA_MEMORY_TYPE_CPU_HUGEPAGE_SHMEM_1GB = 21 | (KIDDMA_PAGESIZE_IDX_1GB << KIDDMA_PAGESIZE_IDX_SHIFT),
    //! Indicates CPU mmap memory.
    KIDDMA_MEMORY_TYPE_CPU_HUGEPAGE_MMAP = 22,
    KIDDMA_MEMORY_TYPE_CPU_HUGEPAGE_MMAP_2MB = 22 | (KIDDMA_PAGESIZE_IDX_2MB << KIDDMA_PAGESIZE_IDX_SHIFT),
    KIDDMA_MEMORY_TYPE_CPU_HUGEPAGE_MMAP_1GB = 22 | (KIDDMA_PAGESIZE_IDX_1GB << KIDDMA_PAGESIZE_IDX_SHIFT),
} iddma_memory_type;

typedef enum {
    KIDDMA_DMA_PROTOCOL_DEFAULT = 0, // "default"
    KIDDMA_DMA_PROTOCOL_MMAP = 0,    // "mmap"
    KIDDMA_DMA_PROTOCOL_TCP = 1,     // "tcp"
    KIDDMA_DMA_PROTOCOL_RDMA = 2,    // "rdma" (unsupported)
} iddma_dma_protocol;

/** @struct iddma_queue_element
 * This is an element of iddma_queue
*/
typedef struct {
    //! Address of specified buffer.
    //! Usually as a form of device side address,
    //! that is passed to iddma_mmap_populate().
    uint64_t addr;
    //! Buffer size in byte unit.
    uint64_t size;
    //! This is iddma_queue_status.
    uint64_t status;
    //! Just a dummy to make this struct 32-byte-data.
    uint64_t imm;
} iddma_queue_element;

/** @struct iddma_queue
 * This represents a queue that contains elements, its head and tail.
*/
typedef struct {
    //! Queue elements (size is MAX_QUEUE_SIZE)
    iddma_queue_element queue_elements[MAX_QUEUE_SIZE];
    //! head represents next position to write to.
    uint32_t head;
    //! tail represents a positon to read from.
    uint32_t tail;
} iddma_queue;

/** @struct iddma_queue_set
 * This represents a set of queues.
 * Contains
 * srq (send request queue),
 * rrq (recv request queue),
 * scq (send completion queue),
 * rcq (recv completion queue).
*/
typedef struct {
    //! Send Request Queue
    iddma_queue srq;
    //! Recv Request Queue
    iddma_queue rrq;
    //! Send Completion Queue
    iddma_queue scq;
    //! Recv Completion Queue
    iddma_queue rcq;
} iddma_queue_set;

//! Pointing to iddma_common class object inherited by iddma_cpu, iddma_gpu
typedef void* iddma_object;
//! Pointing to a device side object (ex. iddma_gpu_object)
//! This is not used by CPU but by other devices like GPU.
typedef void* iddma_device_object;

#ifdef __cplusplus
}
#endif

#endif
