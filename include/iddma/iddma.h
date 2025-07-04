/*************************************************
* Copyright 2025 NTT Corporation
* Licensed under the 3-Clause BSD License, see LICENSE for details.
* SPDX-License-Identifier: BSD-3-Clause
*************************************************/
/**
 * @file iddma.h
 * @brief User Interface of iddma is defined in this file.
 *
 */

#ifndef IDDMA_H_
#define IDDMA_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <iddma_def.h>

/**
 * @brief Create an iddma object.
 *
 * @param [in] dev_type The device type that this object handles.
 * @param [in] options Specify optional parameters for each dev_type.
 * @return iddma_object
 * @sa iddma_device_type
 * @details
 * Creates a iddma object that is used in every host side API.
 */
#ifdef __cplusplus
iddma_object iddma_create_object(iddma_device_type dev_type, const char** options = 0);
#else
iddma_object iddma_create_object(iddma_device_type dev_type, const char** options);
#endif
/**
 * @brief Destroy iddma object
 *
 * @param [in] obj Specify iddma_object to be destroyed.
 * @return iddma_status
 * @details
 * Destroys and cleans up iddma object specified by "obj."
 */
iddma_status iddma_destroy_object(iddma_object obj);

/**
 * @brief Posts a new send request specified by "addr" and "size_byte."
 *
 * @param [in] obj Specify iddma_object of a device to manipulate.
 * @param [in] addr Specify address to copy from.
 * @param [in] size_byte Buffer size in byte unit.
 * @return iddma_status
 * @details
 * Posts a new send request specified by "addr" and "size_byte."
 * If queue is full, KIDDMA_ERROR_QUEUE_IS_FULL returned.
 * The buffer must be registered by mmap_populate when a counterpart device will handle it.
 * Before calling iddma_send(), call iddma_mmap_share() at least.
 * @sa iddma_send_with_imm()
 * @sa iddma_recv()
 * @sa iddma_poll_send()
 * @sa iddma_poll_recv()
 * @sa iddma_mmap_populate()
 * @sa iddma_mmap_share()
 */
iddma_status iddma_send(iddma_object obj, const void *addr, size_t size_byte);

/**
 * @brief Posts a new send request specified by "addr" and "size_byte" with 64bit immediate value.
 *
 * @param [in] obj Specify iddma_object of a device to manipulate.
 * @param [in] addr Specify address to copy from.
 * @param [in] size_byte Buffer size in byte unit.
 * @param [in] imm Specify 64bit immediate value.
 * @return iddma_status
 * @details
 * Posts a new send request specified by "addr" and "size_byte" with 64bit immediate value.
 * If queue is full, KIDDMA_ERROR_QUEUE_IS_FULL returned.
 * The buffer must be registered by mmap_populate when a counterpart device will handle it.
 * Before calling iddma_send_with_imm(), call iddma_mmap_share() at least.
 * @sa iddma_send()
 * @sa iddma_recv()
 * @sa iddma_poll_send()
 * @sa iddma_poll_recv()
 * @sa iddma_mmap_populate()
 * @sa iddma_mmap_share()
 */
iddma_status iddma_send_with_imm(iddma_object obj, const void *addr, size_t size_byte, uint64_t imm);

/**
 * @brief Posts a new receive request specified by "addr" and "size_byte."
 *
 * @param [in] obj Specify iddma_object of a device to manipulate.
 * @param [in] addr Specify address to copy to.
 * @param [in] size_byte Buffer size in byte unit.
 * @return iddma_status
 * @details
 * Posts a new receive request specified by "addr" and "size_byte."
 * If queue is full, KIDDMA_ERROR_QUEUE_IS_FULL returned.
 * The buffer must be registered by mmap_populate when a counterpart device will handle it.
 * Before calling iddma_recv(), call iddma_mmap_share() at least.
 * @sa iddma_send()
 * @sa iddma_poll_send()
 * @sa iddma_poll_recv()
 * @sa iddma_mmap_populate()
 * @sa iddma_mmap_share()
 */
iddma_status iddma_recv(iddma_object obj, void *addr, size_t size_byte);

/**
 * @brief Does polling for a send request completion.
 *
 * @param [in] obj Specify iddma_object of a device to manipulate.
 * @param [in] timeout_sec Time in seconds for polling to time out.
 * @param [out] data A pointer to iddma_queue_element.
 * If there is an element in a completion queue, the completed element will be written to where "data" points.
 * @return iddma_status
 *
 * @details
 * Do polling for a send request completion.
 * When waiting for timeout_sec seconds and getting no completed element,
 * KIDDMA_ERROR_POLL_TIMEOUT returns.
 * If KIDDMA_SUCCESS returns, an element of completion queue is returned.
 * If data is NULL, this function returns KIDDMA_ERROR_INVALID_OPERATION.
 *
 * @sa iddma_send()
 * @sa iddma_recv()
 * @sa iddma_poll_recv()
 */
iddma_status iddma_poll_send(iddma_object obj, uint64_t timeout_sec, iddma_queue_element *data);

/**
 * @brief Does polling for a recv request completion.
 *
 * @param [in] obj Specify iddma_object of a device to manipulate.
 * @param [in] timeout_sec Time in seconds for polling to time out.
 * @param [out] data A pointer to iddma_queue_element.
 * If there is an element in a completion queue, the completed element will be written to where "data" points.
 * @return iddma_status
 *
 * @details
 * Do polling for a recv request completion.
 * When waiting for timeout_sec seconds and getting no completed element,
 * KIDDMA_ERROR_POLL_TIMEOUT returns.
 * If KIDDMA_SUCCESS returns, an element of completion queue is returned.
 * If data is NULL, this function returns KIDDMA_ERROR_INVALID_OPERATION.
 *
 * @sa iddma_send()
 * @sa iddma_recv()
 * @sa iddma_poll_send()
 */
iddma_status iddma_poll_recv(iddma_object obj, uint64_t timeout_sec, iddma_queue_element *data);

/**
 * @brief Gets device-side object. (ex. for GPU)
 *
 * @param [in] obj Specify iddma_object of a device to manipulate.
 * @return iddma_device_object (device-side object's pointer)
 *
 * @details
 * This function returns a pointer to a device-side object.
 * CPU does not have the object, so users will receive NULL pointer.
 * GPU devices have the object, users will receive the pointer and can pass it to a device kernel.
 */
iddma_device_object iddma_get_device_object(iddma_object obj);

/**
 * @brief Prepares for accepting connection.
 *
 * @param [in] obj Specify iddma_object of a device to manipulate.
 * @param [in] id An id used for connection identification.
 * @param [in] directory Directory where a file is created. If not specified, set NULL.
 * @return iddma_status
 *
 * @details
 * Prepares for accepting connection specified by "id" and "directory."
 *
 * This function can be used for process connection as a server.
 * Prepare for iddma_accept() and the connection is identified by "id."
 * A file is created in specified "directory."
 *
 * @sa iddma_accept()
 * @sa iddma_connect()
 * @sa iddma_close()
 */
iddma_status iddma_listen(iddma_object obj, int id, const char *directory);

/**
 * @brief Accepts connection from the counterpart process.
 *
 * @param [in] obj Specify iddma_object of a device to manipulate.
 * @return iddma_status
 *
 * @details
 * Accept and establish the connection that another process requests by iddma_connect().
 *
 * iddma_accept() must be called after iddma_listen().
 * Like a server, it waits for counterpart process calls iddma_connect() using the same "id" and "directory" as iddma_listen() does.
 * After the function completes, users can call iddma_mmap_populate() and iddma_mmap_share().
 * When the application finishes, call iddma_close().
 *
 * @sa iddma_listen()
 * @sa iddma_connect()
 * @sa iddma_close()
 * @sa iddma_mmap_populate()
 * @sa iddma_mmap_share()
 */
iddma_status iddma_accept(iddma_object obj);

/**
 * @brief Establish connection to the counterpart process.
 *
 * @param [in] obj Specify iddma_object of a device to manipulate.
 * @param [in] id An id used for connection identification.
 * @param [in] directory Directory where a file to open exists. If not specified, set NULL.
 * @return iddma_status
 *
 * @details
 * Establish connection specified by "id" and "directory."
 *
 * iddma_connect() opens a file for shared memory by using "id" and "directory."
 * Like a client, it waits for the file creation and the server's acception.
 * After the function completes, users can call iddma_mmap_populate() and iddma_mmap_share().
 * When the application finishes, call iddma_close().
 *
 * @sa iddma_listen()
 * @sa iddma_accept()
 * @sa iddma_close()
 * @sa iddma_mmap_populate()
 * @sa iddma_mmap_share()
 */
iddma_status iddma_connect(iddma_object obj, int id, const char *directory);

/**
 * @brief Close process connection established by iddma_accept() or iddma_connect().
 *
 * @param [in] obj Specify iddma_object of a device to manipulate.
 * @return iddma_status
 *
 * @details
 * Close connection established by iddma_accept() or iddma_connect().
 *
 * This function waits for another process to attempt to close connection,
 * and then close connection.
 * The file created will be deleted on server side process.
 * After iddma_close(), any API cannot be executed, and users will receive KIDDMA_ERROR_NOT_CONNECTED.
 *
 * @sa iddma_listen()
 * @sa iddma_accept()
 * @sa iddma_connect()
 */
iddma_status iddma_close(iddma_object obj);

/**
 * @brief Create a memory map for device-side buffer.
 *
 * @param [in] obj Specify iddma_object of a device to manipulate.
 * @param [in] addr Device side buffer address to register.
 * @param [in] size_byte Buffer size in byte unit.
 * @return iddma_status
 *
 * @details
 * Create a memory map for device-side buffer specified by "addr" and "size_byte."
 *
 * This function can be called after iddma_accept()/iddma_connect().
 * If a counterpart device manipulates this device buffer, users must call iddma_mmap_populate() for buffer registration.
 * Calling iddma_mmap_populate() after iddma_mmap_share() is not supported.
 *
 * @sa iddma_accept()
 * @sa iddma_connect()
 * @sa iddma_mmap_share()
 */
iddma_status iddma_mmap_populate(iddma_object obj, void *addr, size_t size_byte);

/**
 * @brief Share a memory map with a counterpart process.
 *
 * @param [in] obj Specify iddma_object of a device to manipulate.
 * @return iddma_status
 *
 * @details
 * Share the memory map created by iddma_mmap_populate() with a counterpart process.
 *
 * This function can be called after iddma_accept()/iddma_connect().
 * In the function, exports buffer information created by iddma_mmap_populate() to a counterpart device,
 * and imports buffer information from the counterpart device.
 * After iddma_mmap_share(), do not call iddma_mmap_populate().
 *
 * @sa iddma_accept()
 * @sa iddma_connect()
 * @sa iddma_mmap_populate()
 * @sa iddma_send()
 * @sa iddma_recv()
 */
iddma_status iddma_mmap_share(iddma_object obj);

/**
 * @brief Allocate a buffer for iddma transfer.
 *
 * @param [in] size Size of buffer to be allocated.
 * @param [in] type Memory type of buffer to be allocated.
 * @param [in] align Memory alignment of buffer to be allocated (defulat = 64 byte).
 * @return void*
 *
 * @details
 * Allocate a buffer for iddma transfer.
 * Currently, type can accept KIDDMA_MEMORY_TYPE_CPU_MMAP. KIDDMA_MEMORY_TYPE_CPU_SHMEM is not fully supported.
 *
 * @sa iddma_mem_free()
 */
void *iddma_mem_allocate(size_t size, iddma_memory_type type, size_t align);

/**
 * @brief Free the buffer allocated by iddma_mem_allocate().
 *
 * @param [in] addr A pointer to the buffer to be freed.
 * @return void
 *
 * @details
 * Free the buffer allocated by iddma_mem_allocate().
 *
 * @sa iddma_mem_allocate()
 */
void iddma_mem_free(void *addr);

#ifdef __cplusplus
}
#endif

#endif // IDDMA_H_
