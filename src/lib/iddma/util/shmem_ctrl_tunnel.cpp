/*************************************************
* Copyright 2025 NTT Corporation
* Licensed under the 3-Clause BSD License, see LICENSE for details.
* SPDX-License-Identifier: BSD-3-Clause
*************************************************/

#include <shmem_ctrl_tunnel.hpp>
#include <ftok_util.hpp>
#include <cstring>

typedef enum {
    KIDDMA_SHARED_SPEC_IPC_INFO = 0,
    KIDDMA_SHARED_SPEC_ACCEPTOR_QUEUE_INFO = 1,
    KIDDMA_SHARED_SPEC_CONNECTOR_QUEUE_INFO = 2,
    KIDDMA_SHARED_SPEC_MEMMAP_KEY = 3,
    KIDDMA_SHARED_SPEC_MEMMAP_VALUE = 4,
} iddma_sharedmem_spec;

const uint32_t shmem_ctrl_tunnel::s_max_token_size_ = 1024;
const uint32_t shmem_ctrl_tunnel::s_retry_max_ = 10000; // 10sec = 1msec x 10000
const struct timespec shmem_ctrl_tunnel::s_tick_ = { .tv_sec = 0, .tv_nsec = 1000000 }; // 1msec

shmem_ctrl_tunnel::shmem_ctrl_tunnel(int id, const std::string& dir, bool is_listen)
    : timeout_sec_(30),
      ftok_id_(124),
      is_acceptor_(false),
      is_connector_(false),
      ipc_info_(nullptr),
      mmap_share_data_(nullptr),
      mmap_share_token_(nullptr),
      ipc_info_id_(-1),
      mmap_share_data_id_(-1),
      mmap_share_token_id_(-1) {
    initialize(id, dir, is_listen);
}

shmem_ctrl_tunnel::shmem_ctrl_tunnel(int id, const std::string& dir, bool is_listen, int timeout_sec, int ftok_id)
    : timeout_sec_(timeout_sec),
      ftok_id_(ftok_id),
      is_acceptor_(false),
      is_connector_(false),
      ipc_info_(nullptr),
      mmap_share_data_(nullptr),
      mmap_share_token_(nullptr),
      ipc_info_id_(-1),
      mmap_share_data_id_(-1),
      mmap_share_token_id_(-1) {
    initialize(id, dir, is_listen);
}

shmem_ctrl_tunnel::~shmem_ctrl_tunnel(void) {
    if (mmap_share_data_) {
        destroy_mmap_share_data(mmap_share_data_);
    }
    if (ipc_info_) {
        if (is_acceptor_) {
            dispose_shared_mem(ipc_info_id_, ipc_info_);
            delete_ftok_file(ftok_filepath_);
        } else {
            detach_shared_mem(ipc_info_);
        }
    }
}

void shmem_ctrl_tunnel::finalize(void) {
    // now nop
}

iddma_status shmem_ctrl_tunnel::listen(iddma_queue_set* queue_set, iddma_device_type dev_type,
                                       int acceptor_dev_id, int acceptor_ch_id) {
    struct ipc_info *i_info = (struct ipc_info*)create_ipc_info();
    if (!i_info) return KIDDMA_ERROR_INVALID_OPERATION;

    bool is_ok = true;
    i_info->is_accepted = false;
    i_info->is_connected = false;
    i_info->connect_needs_close = false;
    i_info->accept_needs_close = false;
    i_info->temp_data = false;
    is_ok &= update(i_info, i_info->acceptor_queue_set, (uintptr_t)queue_set);
    is_ok &= update(i_info, i_info->acceptor_type, dev_type);
    is_ok &= update(i_info, i_info->acceptor_dev_id, acceptor_dev_id);
    is_ok &= update(i_info, i_info->acceptor_ch_id, acceptor_ch_id);
    is_ok &= update(i_info, i_info->protocol, KIDDMA_DMA_PROTOCOL_DEFAULT);
    i_info->accept_valid = false;
    i_info->accept_ready = false;

    if (is_ok) is_acceptor_ = true;

    return !is_ok ? KIDDMA_ERROR_NOT_CONNECTED : KIDDMA_SUCCESS;
}

iddma_status shmem_ctrl_tunnel::accept(iddma_queue_set*& cp_queue_set, iddma_device_type& cp_dev_type,
                                       int& connector_dev_id, int& connector_ch_id) {
    struct ipc_info *i_info = (struct ipc_info*)create_ipc_info();
    if (!i_info) return KIDDMA_ERROR_INVALID_OPERATION;

    if (i_info->is_accepted == true)
        return KIDDMA_SUCCESS;
    bool is_ok = update(i_info, i_info->accept_magic, s_first_magic_);
    if (!is_ok || !wait_sync(i_info, i_info->connect_magic, s_first_magic_)) {
        update(i_info, i_info->accept_magic, 0u);
        return is_ok ? KIDDMA_ERROR_CONNECTION_TIMEOUT : KIDDMA_ERROR_NOT_CONNECTED;
    }
    is_ok &= update(i_info, i_info->accept_magic, s_second_magic_);
    if (!is_ok || !wait_sync(i_info, i_info->connect_magic, s_second_magic_)) {
        update(i_info, i_info->accept_magic, 0u);
        return is_ok ? KIDDMA_ERROR_CONNECTION_TIMEOUT : KIDDMA_ERROR_NOT_CONNECTED;
    }
    is_ok &= update(i_info, i_info->accept_magic, 0u);
    is_ok &= update(i_info, i_info->is_accepted, true);

    if (!is_ok || !wait_sync(i_info, i_info->is_connected, true)) {
        return is_ok ? KIDDMA_ERROR_CONNECTION_TIMEOUT : KIDDMA_ERROR_NOT_CONNECTED;
    }

    cp_queue_set = (iddma_queue_set*)(i_info->connector_queue_set);
    cp_dev_type = i_info->connector_type;
    connector_dev_id = i_info->connector_dev_id;
    connector_ch_id = i_info->connector_ch_id;
    if (!process_synchronize()) {
        is_acceptor_ = false;
        return KIDDMA_ERROR_SYNC_FAILED;
    }
    return KIDDMA_SUCCESS;
}

iddma_status shmem_ctrl_tunnel::connect(iddma_queue_set* queue_set, iddma_device_type dev_type,
                                        iddma_queue_set*& cp_queue_set, iddma_device_type& cp_dev_type,
                                        int connector_dev_id, int connector_ch_id,
                                        int& acceptor_dev_id, int& acceptor_ch_id) {
    struct ipc_info *i_info = (struct ipc_info*)create_ipc_info();
    if (!i_info) return KIDDMA_ERROR_INVALID_OPERATION;

    i_info->connect_ready = false;
    i_info->connect_valid = false;

    if (!wait_sync(i_info, i_info->accept_magic, s_first_magic_)) {
        update(i_info, i_info->connect_magic, 0u);
        return KIDDMA_ERROR_CONNECTION_TIMEOUT;
    }
    bool is_ok = update(i_info, i_info->connect_magic, s_first_magic_);
    if (!is_ok || !wait_sync(i_info, i_info->accept_magic, s_second_magic_)) {
        update(i_info, i_info->connect_magic, 0u);
        return is_ok ? KIDDMA_ERROR_CONNECTION_TIMEOUT : KIDDMA_ERROR_NOT_CONNECTED;
    }
    is_ok &= update(i_info, i_info->connect_magic, s_second_magic_);
    if (!is_ok || !wait_sync(i_info, i_info->accept_magic, 0u)) {
        update(i_info, i_info->connect_magic, 0u);
        return is_ok ? KIDDMA_ERROR_CONNECTION_TIMEOUT : KIDDMA_ERROR_NOT_CONNECTED;
    }
    is_ok &= update(i_info, i_info->connect_magic, 0u);

    if (!is_ok || !wait_sync(i_info, i_info->is_accepted, true)) {
        return is_ok ? KIDDMA_ERROR_CONNECTION_TIMEOUT : KIDDMA_ERROR_NOT_CONNECTED;
    }

    if (i_info->protocol != KIDDMA_DMA_PROTOCOL_DEFAULT) {
        update(i_info, i_info->protocol, KIDDMA_DMA_PROTOCOL_DEFAULT);
        if (!process_synchronize()) return KIDDMA_ERROR_SYNC_FAILED;
        return KIDDMA_ERROR_MISMATCH_PROTOCOL;
    }

    cp_queue_set = (iddma_queue_set*)(i_info->acceptor_queue_set);
    is_ok &= update(i_info, i_info->connector_queue_set, (uintptr_t)queue_set);
    is_ok &= update(i_info, i_info->connector_type, dev_type);
    is_ok &= update(i_info, i_info->connector_dev_id, connector_dev_id);
    is_ok &= update(i_info, i_info->connector_ch_id, connector_ch_id);
    acceptor_dev_id = i_info->acceptor_dev_id;
    acceptor_ch_id = i_info->acceptor_ch_id;
    cp_dev_type = i_info->acceptor_type;
    is_ok &= update(i_info, i_info->is_connected, true);
    if (!is_ok) return KIDDMA_ERROR_NOT_CONNECTED;

    is_connector_ = true;
    if (!process_synchronize()) {
        is_connector_ = false;
        return KIDDMA_ERROR_SYNC_FAILED;
    }

    return KIDDMA_SUCCESS;
}

iddma_status shmem_ctrl_tunnel::close_pre(void) {
    struct ipc_info *i_info = (struct ipc_info*)create_ipc_info();
    if (!i_info) return KIDDMA_ERROR_INVALID_OPERATION;

    if (is_acceptor_) {
        if (!update(i_info, i_info->accept_needs_close, true)) return KIDDMA_ERROR_NOT_CONNECTED;
    } else if (is_connector_) {
        if (!update(i_info, i_info->connect_needs_close, true)) return KIDDMA_ERROR_NOT_CONNECTED;
    }
    return KIDDMA_SUCCESS;
}

iddma_status shmem_ctrl_tunnel::close_post(void) {
    struct ipc_info *i_info = (struct ipc_info*)create_ipc_info();
    if (!i_info) return KIDDMA_ERROR_INVALID_OPERATION;

    bool is_ok = true;
    if (is_acceptor_) {
        if (is_ok && !wait_sync(i_info, i_info->connect_needs_close, true)) {
            return KIDDMA_ERROR_CONNECTION_TIMEOUT;
        }
        is_ok &= update(i_info, i_info->is_accepted, false);
        is_acceptor_ = false;
    } else if (is_connector_) {
        if (is_ok && !wait_sync(i_info, i_info->accept_needs_close, true)) {
            return KIDDMA_ERROR_CONNECTION_TIMEOUT;
        }
        is_ok &= update(i_info, i_info->is_connected, false);
        is_connector_ = false;
    }
    return is_ok ? KIDDMA_SUCCESS : KIDDMA_ERROR_NOT_CONNECTED;
}

bool shmem_ctrl_tunnel::is_connected(void) const {
    const struct ipc_info *i_info = (const struct ipc_info*)create_ipc_info();
    if (!i_info) return false;
    if (is_connector_ && i_info->is_accepted) {
        return true;
    } else if (is_acceptor_ && i_info->is_connected) {
        return true;
    }
    return false;
}

iddma_status shmem_ctrl_tunnel::mmap_import(int& map_size) {
    struct ipc_info *i_info = (struct ipc_info*)create_ipc_info();
    if (!i_info) return KIDDMA_ERROR_INVALID_OPERATION;

    if (!process_synchronize()) return KIDDMA_ERROR_SYNC_FAILED; // wait for size input
    map_size = i_info->temp_data;
    return KIDDMA_SUCCESS;
}

iddma_status shmem_ctrl_tunnel::mmap_export(int map_size) {
    struct ipc_info *i_info = (struct ipc_info*)create_ipc_info();
    if (!i_info) return KIDDMA_ERROR_INVALID_OPERATION;

    bool is_ok = update(i_info, i_info->temp_data, map_size);
    if (!is_ok) return KIDDMA_ERROR_NOT_CONNECTED;
    if (!process_synchronize()) return KIDDMA_ERROR_SYNC_FAILED; // wait for size input
    return KIDDMA_SUCCESS;
}

iddma_status shmem_ctrl_tunnel::execute_mmap_import(struct mmap_share_data& map_data) {
    struct ipc_info *i_info = (struct ipc_info*)create_ipc_info();
    if (!i_info) return KIDDMA_ERROR_INVALID_OPERATION;

    struct mmap_share_data tmp_shdat;
    if (!process_synchronize()) return KIDDMA_ERROR_SYNC_FAILED; // wait for export end

    if (!process_synchronize()) return KIDDMA_ERROR_SYNC_FAILED; // shared data imported
    if (!process_synchronize()) return KIDDMA_ERROR_SYNC_FAILED; // wait for shared token set

    if (!import_mmap(&map_data)) return KIDDMA_ERROR_BUFFER_MMAP_IMPORT_FAILED;

    if (!update(i_info, i_info->temp_data, 0ul)) return KIDDMA_ERROR_NOT_CONNECTED;

    if (!process_synchronize()) return KIDDMA_ERROR_SYNC_FAILED; // import end
    if (!process_synchronize()) return KIDDMA_ERROR_SYNC_FAILED; // wait for export postprocess end

    return KIDDMA_SUCCESS;
}

iddma_status shmem_ctrl_tunnel::execute_mmap_export(struct mmap_share_data& tmp_shdat) {
    struct ipc_info *i_info = (struct ipc_info*)create_ipc_info();
    if (!i_info) return KIDDMA_ERROR_INVALID_OPERATION;

    void* meminfo_addr = create_mmap_share_data();

    iddma_status status = KIDDMA_SUCCESS;
    mmap_share_data* target((mmap_share_data*)meminfo_addr);

    if (!update(meminfo_addr, *target, tmp_shdat)) {
        return KIDDMA_ERROR_BUFFER_MMAP_EXPORT_FAILED;
    }

    if (!process_synchronize()) return KIDDMA_ERROR_SYNC_FAILED; // export end
    if (!process_synchronize()) return KIDDMA_ERROR_SYNC_FAILED; // wait for shared data imported

    if (!update((void*)target->token_ptr, (const void*)tmp_shdat.token_ptr, tmp_shdat.token_size)) {
        return KIDDMA_ERROR_BUFFER_MMAP_EXPORT_FAILED;
    }

    if (!process_synchronize()) return KIDDMA_ERROR_SYNC_FAILED; // shared token set end
    if (!process_synchronize()) return KIDDMA_ERROR_SYNC_FAILED; // wait for import end
    size_t counter_status = i_info->temp_data;

    if (!process_synchronize()) return KIDDMA_ERROR_SYNC_FAILED; // export postprocess end
    if (counter_status != 0)
        status = KIDDMA_ERROR_BUFFER_MMAP_EXPORT_FAILED;
    return status;
}

bool shmem_ctrl_tunnel::process_synchronize(void) {
    struct ipc_info *i_info = (struct ipc_info*)create_ipc_info();
    if (!i_info) return false;

    bool is_ok = true;
    if (is_connector_) {
        if (!wait_sync(i_info, i_info->accept_ready, true)) return false;
        is_ok = update(i_info, i_info->connect_ready, true);
        if (!is_ok || !wait_sync(i_info, i_info->accept_ready, false)) return false;
        is_ok = update(i_info, i_info->connect_ready, false);
        if (!is_ok || !wait_sync(i_info, i_info->accept_valid, true)) return false;
        is_ok = update(i_info, i_info->connect_valid, true);
        if (!is_ok || !wait_sync(i_info, i_info->accept_valid, false)) return false;
        is_ok = update(i_info, i_info->connect_valid, false);
    } else if (is_acceptor_) {
        is_ok = update(i_info, i_info->accept_ready, true);
        if (!is_ok || !wait_sync(i_info, i_info->connect_ready, true)) return false;
        is_ok = update(i_info, i_info->accept_ready, false);
        if (!is_ok || !wait_sync(i_info, i_info->connect_ready, false)) return false;
        is_ok = update(i_info, i_info->accept_valid, true);
        if (!is_ok || !wait_sync(i_info, i_info->connect_valid, true)) return false;
        is_ok = update(i_info, i_info->accept_valid, false);
    }
    return is_ok;
}

bool shmem_ctrl_tunnel::import_mmap(mmap_share_data* mmap_data) {
    void* meminfo_addr = create_mmap_share_data();
    memcpy(mmap_data, meminfo_addr, sizeof(mmap_share_data));
    buffers_.push_back(std::vector<char>(mmap_data->token_size));
    mmap_data->token_ptr = (uintptr_t)&buffers_.back()[0];
    memcpy((void*)mmap_data->token_ptr, mmap_share_token_, mmap_data->token_size);
    return true;
}

bool shmem_ctrl_tunnel::update(void* base, bool& target, bool value) {
    return update_core(target, value);
}

bool shmem_ctrl_tunnel::update(void* base, uint32_t& target, uint32_t value) {
    return update_core(target, value);
}

bool shmem_ctrl_tunnel::update(void* base, uint64_t& target, uint64_t value) {
    return update_core(target, value);
}

bool shmem_ctrl_tunnel::update(void* base, iddma_device_type& target, iddma_device_type value) {
    return update_core(target, value);
}

bool shmem_ctrl_tunnel::update(void* base, iddma_dma_protocol& target, iddma_dma_protocol value) {
    return update_core(target, value);
}

bool shmem_ctrl_tunnel::update(void* base, mmap_share_data& target, const mmap_share_data& value) {
    target.addr = value.addr;
    target.size = value.size;
    target.token_size = value.token_size;
    target.mem_type = value.mem_type;
    return true;
}

bool shmem_ctrl_tunnel::update(void* target, const void* src, uint32_t len) {
    memcpy(mmap_share_token_, src, len);
    return true;
}

bool shmem_ctrl_tunnel::update_remote(iddma_transfer_destination type, int idx, const iddma_queue_element* src) {
    return true;
}

bool shmem_ctrl_tunnel::update_remote(bool is_send, int req_tail, int cpl_head) {
    return true;
}

bool shmem_ctrl_tunnel::wait_sync(void* base, bool& target, bool wait_value) {
    return wait_sync_core(target, wait_value);
}

bool shmem_ctrl_tunnel::wait_sync(void* base, uint32_t& target, uint32_t wait_value) {
    return wait_sync_core(target, wait_value);
}

static iddma_status obtain_sharedmemory(const std::string& filepath, int id, size_t size,
                                        void** ptr, int* shmem_id) {
    key_t key;
    iddma_status status;
    if ((status = acquire_key_from_file_and_id(filepath, id , &key)))
        return status;
    if ((status = acquire_id_from_key(key, size, shmem_id)) != KIDDMA_SUCCESS)
        return status;
    if ((status = attach_shared_mem_from_id(*shmem_id, ptr)) != KIDDMA_SUCCESS)
        return status;
    return status;
}

void* shmem_ctrl_tunnel::create_ipc_info() {
    if (ipc_info_) return ipc_info_;

    int cur_id = ftok_id_ + (int)KIDDMA_SHARED_SPEC_IPC_INFO;
    iddma_status status = obtain_sharedmemory(ftok_filepath_, cur_id, sizeof(ipc_info), &ipc_info_, &ipc_info_id_);
    return ipc_info_;
}

const void* shmem_ctrl_tunnel::create_ipc_info() const {
    if (ipc_info_) return ipc_info_;
    return nullptr;
}

void* shmem_ctrl_tunnel::create_mmap_share_data() {
    if (mmap_share_data_) return mmap_share_data_;

    int mmap_id = ftok_id_ + (int)KIDDMA_SHARED_SPEC_MEMMAP_KEY;
    iddma_status status = obtain_sharedmemory(ftok_filepath_, mmap_id, sizeof(mmap_share_data),
                                              &mmap_share_data_, &mmap_share_data_id_);
    if (status == KIDDMA_SUCCESS) {
        int token_id = ftok_id_ + (int)KIDDMA_SHARED_SPEC_MEMMAP_VALUE;
        status = obtain_sharedmemory(ftok_filepath_, token_id, s_max_token_size_,
                                     &mmap_share_token_, &mmap_share_token_id_);
        if (status != KIDDMA_SUCCESS) {
            destroy_mmap_share_data(mmap_share_data_);
        }
    }
    return mmap_share_data_;
}

void shmem_ctrl_tunnel::destroy_mmap_share_data(void* ptr) {
    if (mmap_share_data_ && mmap_share_data_ == ptr) {
        if (is_acceptor_) {
            dispose_shared_mem(mmap_share_data_id_, mmap_share_data_);
        } else {
            detach_shared_mem(mmap_share_data_);
        }
        mmap_share_data_ = nullptr;
        mmap_share_data_id_ = -1;

        if (mmap_share_token_) {
            if (is_acceptor_) {
                dispose_shared_mem(mmap_share_token_id_, mmap_share_token_);
            } else {
                detach_shared_mem(mmap_share_token_);
            }
            mmap_share_token_ = nullptr;
            mmap_share_token_id_ = -1;
        }
    }
}

void shmem_ctrl_tunnel::initialize(int id, const std::string& dir, bool is_listen) {
    if (dir.empty()) throw std::runtime_error("invalid operation");
    ftok_filepath_ = dir + "shared_tmp_" + std::to_string(id);

    iddma_status status;
    if (is_listen) {
        status = create_ftok_file(ftok_filepath_);
        if (status == KIDDMA_ERROR_FILE_CREATE) throw std::runtime_error("file creation failed");
        is_acceptor_ = true;
    } else {
        if ((status = wait_for_ftok_file(ftok_filepath_, timeout_sec_)) != KIDDMA_SUCCESS) {
            throw std::runtime_error("wait for file creation failed");
        }
    }
}

template<typename T>
bool shmem_ctrl_tunnel::update_core(T& target, const T& value) {
    target = value;
    return true;
}

template<typename T>
bool shmem_ctrl_tunnel::wait_sync_core(T& target, const T& value) {
    int retry = s_retry_max_;
    while (target != value && retry) {
        nanosleep(&s_tick_, NULL);
        retry--;
    }
    return retry;
}
