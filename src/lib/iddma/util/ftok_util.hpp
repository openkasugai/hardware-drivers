/*************************************************
* Copyright 2025 NTT Corporation
* Licensed under the 3-Clause BSD License, see LICENSE for details.
* SPDX-License-Identifier: BSD-3-Clause
*************************************************/
/**
 * @file ftok_util.hpp
 * @brief ftok() utility functions.
 *
 */

#ifndef __FTOK_UTIL_H__

#define __FTOK_UTIL_H__
#include <iddma_def.h>
#include <cstdint>
#include <string>
#include <future>

bool exists_ftok_file(std::string file_path);
iddma_status create_ftok_file(std::string file_path);
iddma_status delete_ftok_file(std::string file_path);
iddma_status wait_for_ftok_file(std::string file_path, uint64_t timeout_sec);
void timeout_thread_wait_for_ftok_file(std::promise<bool> file_check_promise, std::string file_path, volatile bool *exit_flag);
iddma_status acquire_key_from_file_and_id(std::string file_path, int id, key_t *key);
iddma_status acquire_id_from_key(key_t shmem_key, size_t shmem_size, int *shmem_id);
iddma_status create_shared_mem_from_key(key_t shmem_key, size_t shmem_size, int *shmem_id, void **shmem_address);
iddma_status attach_shared_mem_from_key(key_t shmem_key, int *shmem_id, void **shmem_address, uint64_t timeout_sec);
iddma_status attach_shared_mem_from_id(int shmem_id, void **shmem_address);
iddma_status wait_until_shared_mem_detached_by_creator(int shmem_id);
int acquire_shared_mem_nattach(int shmem_id);
iddma_status dispose_shared_mem(int id, void *addr);
iddma_status detach_shared_mem(void *addr);

#endif
