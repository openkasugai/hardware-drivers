/*************************************************
* Copyright 2025 NTT Corporation
* Licensed under the 3-Clause BSD License, see LICENSE for details.
* SPDX-License-Identifier: BSD-3-Clause
*************************************************/
/**
 * @file ftok_util.cpp
 * @brief Implementation of ftok() utility.
 *
 */

#include <ftok_util.hpp>
#include <sys/mman.h>
#include <sys/shm.h>
#include <sys/ipc.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <cstring>
#include <fstream>

bool exists_ftok_file(std::string file_path) {
    struct stat stat_buf;
    if (stat(file_path.c_str(), &stat_buf) != 0) {
        return false;
    }
    if ((stat_buf.st_mode & S_IFMT) == S_IFREG)
        return true;
    return false;
}

iddma_status create_ftok_file(std::string file_path) {
    if (exists_ftok_file(file_path)) {
        perror_debug("%s already exists\n", file_path.c_str());
        return KIDDMA_ERROR_FILE_ALREADY_EXISTS;
    }

    if(!std::ofstream(file_path.c_str()))
        return KIDDMA_ERROR_FILE_CREATE;

    return KIDDMA_SUCCESS;
}

iddma_status delete_ftok_file(std::string file_path) {
    if (!exists_ftok_file(file_path)) {
        perror_debug("%s does not exist\n", file_path.c_str());
        return KIDDMA_ERROR_FILE_NOT_EXIST;
    }

    if (remove(file_path.c_str()) != 0)
        return KIDDMA_ERROR_REMOVE_FAILED;

    return KIDDMA_SUCCESS;
}

iddma_status wait_for_ftok_file(std::string file_path, uint64_t timeout_sec) {
    iddma_status file_check_status;
    std::future_status file_check_future_status;
    std::promise<bool> file_check_promise;
    std::future<bool> file_check_future = file_check_promise.get_future();
    bool timeout_thread_exit_flag = false;

    std::thread file_check_thread(timeout_thread_wait_for_ftok_file, std::move(file_check_promise), file_path, &timeout_thread_exit_flag);

    file_check_future_status = file_check_future.wait_for(std::chrono::seconds(timeout_sec));
    if (file_check_future_status != std::future_status::timeout) {
        if (file_check_future.get())
            file_check_status = KIDDMA_SUCCESS;
        else
            file_check_status = KIDDMA_ERROR_FILE_NOT_EXIST;
    } else {
        perror_debug("timeout\n");
        file_check_status = KIDDMA_ERROR_FILE_CHECK_TIMEOUT;
    }
    timeout_thread_exit_flag = true;
    file_check_thread.join();

    return file_check_status;
}

void timeout_thread_wait_for_ftok_file(std::promise<bool> file_check_promise, std::string file_path, volatile bool *exit_flag) {
    while (!(*exit_flag)) {
        if (exists_ftok_file(file_path)) {
            file_check_promise.set_value(true);
            return;
        }
    }
    file_check_promise.set_value(false);
}

iddma_status acquire_key_from_file_and_id(std::string file_path, int id, key_t *key) {
    *key = ftok(file_path.c_str(), id);
    if (*key == -1) {
        perror_debug("ftok failed to acquire key\n");
        return KIDDMA_ERROR_ACQUIRE_KEY_FAILED;
    }
    return KIDDMA_SUCCESS;
}

iddma_status acquire_id_from_key(key_t shmem_key, size_t shmem_size, int *shmem_id) {
    if ((*shmem_id = shmget(shmem_key, shmem_size, IPC_CREAT|0666)) == -1) {
        perror_debug("shmget error\n");
        return KIDDMA_ERROR_SHMGET;
    }
    return KIDDMA_SUCCESS;
}

iddma_status create_shared_mem_from_key(key_t shmem_key, size_t shmem_size, int *shmem_id, void **shmem_address) {
    if ((*shmem_id = shmget(shmem_key, shmem_size, IPC_CREAT|0666)) == -1) {
        perror_debug("shmget error\n");
        return KIDDMA_ERROR_SHMGET;
    }
    *shmem_address = shmat(*shmem_id, 0, 0);
    if (*shmem_address == (void *)-1) {
        perror_debug("shmat error\n");
        return KIDDMA_ERROR_SHMAT;
    }
    return KIDDMA_SUCCESS;
}

void timeout_thread_wait_until_shmget_succeed_for_attach(std::promise<int> shmem_promise, key_t shmem_key, volatile bool *exit_flag) {
    int shmem_id = -1;
    while (!(*exit_flag)) {
        shmem_id = shmget(shmem_key, 0, 0);
        if (shmem_id != -1) {
            shmem_promise.set_value(shmem_id);
            return;
        }
    }
    shmem_promise.set_value(shmem_id);
}

void timeout_thread_wait_until_shmat_succeed_for_attach(std::promise<void *> shmem_promise, int shmem_id, volatile bool *exit_flag) {
    void *shmem_address;
    shmem_address = (void *)-1;
    while (!(*exit_flag)) {
        shmem_address = shmat(shmem_id, 0, 0);
        if (shmem_address != (void *)-1) {
            shmem_promise.set_value(shmem_address);
            return;
        }
    }
    shmem_promise.set_value(shmem_address);
}

iddma_status attach_shared_mem_from_key(key_t shmem_key, int *shmem_id, void **shmem_address, uint64_t timeout_sec) {
    std::future_status shmget_future_status;
    std::promise<int> shmget_promise;
    std::future<int> shmget_future = shmget_promise.get_future();
    bool timeout_thread_exit_flag = false;

    std::future_status shmat_future_status;
    std::promise<void *> shmat_promise;
    std::future<void *> shmat_future = shmat_promise.get_future();

    // get shared buffer
    std::thread shmget_thread(timeout_thread_wait_until_shmget_succeed_for_attach, std::move(shmget_promise), shmem_key, &timeout_thread_exit_flag);

    shmget_future_status = shmget_future.wait_for(std::chrono::seconds(timeout_sec));
    timeout_thread_exit_flag = true;
    shmget_thread.join();

    if (shmget_future_status == std::future_status::timeout) {
        *shmem_id = -1;
        perror_debug("shmget error timeout\n");
        return KIDDMA_ERROR_UNKNOWN_EXCEPTION;
    } else {
        *shmem_id = shmget_future.get();
    }

    // attach shared buffer
    timeout_thread_exit_flag = false;
    std::thread shmat_thread(timeout_thread_wait_until_shmat_succeed_for_attach, std::move(shmat_promise), *shmem_id, &timeout_thread_exit_flag);

    shmat_future_status = shmat_future.wait_for(std::chrono::seconds(timeout_sec));
    timeout_thread_exit_flag = true;
    shmat_thread.join();

    if (shmat_future_status == std::future_status::timeout) {
        *shmem_address = (void *)-1;
        return KIDDMA_ERROR_UNKNOWN_EXCEPTION;
    } else {
        *shmem_address = shmat_future.get();
    }

    return KIDDMA_SUCCESS;
}

iddma_status attach_shared_mem_from_id(int shmem_id, void **shmem_address) {
    *shmem_address = shmat(shmem_id, 0, 0);
    if (*shmem_address == (void *)-1) {
        perror_debug("shmat error\n");
        return KIDDMA_ERROR_SHMAT;
    }
    return KIDDMA_SUCCESS;
}

int acquire_shared_mem_nattach(int shmem_id) {
    struct shmid_ds shmem_stat;
    if (shmctl(shmem_id, IPC_STAT, &shmem_stat) == -1) {
        return -1;
    }

    return (int)shmem_stat.shm_nattch;
}

// assume use process already attached shmem_id's shared memory
iddma_status wait_until_shared_mem_detached_by_creator(int shmem_id) {
    int nattach = 2; // nattach may be 2 or 1. initially initiated with acquire_*() function so this value has no meaning.
    while ((nattach = acquire_shared_mem_nattach(shmem_id)) > 1) {
        if (nattach == -1) return KIDDMA_ERROR_UNKNOWN_EXCEPTION;
        if (nattach == 1) break;
    }
    return KIDDMA_SUCCESS;
}

iddma_status dispose_shared_mem(int id, void *addr) {
    if (shmdt(addr) == -1)
        return KIDDMA_ERROR_SHMEM_DETACH_FAILED;
    if (shmctl(id, IPC_RMID, 0) == -1)
        return KIDDMA_ERROR_SHMEM_RELEASE_FAILED;
    return KIDDMA_SUCCESS;
}

iddma_status detach_shared_mem(void *addr) {
    if (shmdt(addr) == -1)
        return KIDDMA_ERROR_SHMEM_DETACH_FAILED;
    return KIDDMA_SUCCESS;
}
