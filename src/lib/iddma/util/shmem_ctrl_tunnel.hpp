/*************************************************
* Copyright 2025 NTT Corporation
* Licensed under the 3-Clause BSD License, see LICENSE for details.
* SPDX-License-Identifier: BSD-3-Clause
*************************************************/

#ifndef _SHMEM_CTRL_TUNNEL_HPP__
#define _CHMEM_CTRL_TUNNEL_HPP__

#include <ctrl_tunnel.hpp>
#include <iddma_def.h>
#include <map>

class shmem_ctrl_tunnel : public ctrl_tunnel {
public:
    shmem_ctrl_tunnel(int id, const std::string& dir, bool is_listen);
    shmem_ctrl_tunnel(int id, const std::string& dir, bool is_listen, int timeout_sec, int ftok_id);
    ~shmem_ctrl_tunnel(void);

    iddma_status listen(iddma_queue_set* queue_set, iddma_device_type dev_type,
                        int acceptor_dev_id, int acceptor_ch_id);
    iddma_status accept(iddma_queue_set*& cp_queue_set, iddma_device_type& cp_dev_type,
                        int& connector_dev_id, int& connector_ch_id);
    iddma_status connect(iddma_queue_set* queue_set, iddma_device_type dev_type,
                         iddma_queue_set*& cp_queue_set, iddma_device_type& cp_dev_type,
                         int connector_dev_id, int connector_ch_id,
                         int& acceptor_dev_id, int& acceptor_ch_id);
    iddma_status close_pre(void);
    iddma_status close_post(void);
    bool is_connected(void) const;

    iddma_status mmap_import(int& map_size);
    iddma_status mmap_export(int map_size);
    iddma_status execute_mmap_import(struct mmap_share_data& map_data);
    iddma_status execute_mmap_export(struct mmap_share_data& tmp_shdat);
    bool process_synchronize(void);

    void finalize(void);

private:
    void* create_mmap_share_data(void);
    void destroy_mmap_share_data(void* ptr);

    bool import_mmap(mmap_share_data* mmap_data);

    bool update(void* base, bool& target, bool value);
    bool update(void* base, uint32_t& target, uint32_t value);
    bool update(void* base, uint64_t& target, uint64_t value);
    bool update(void* base, iddma_device_type& target, iddma_device_type value);
    bool update(void* base, iddma_dma_protocol& target, iddma_dma_protocol value);
    bool update(void* base, mmap_share_data& target, const mmap_share_data& value);
    bool update(void* target, const void* src, uint32_t len);

    bool update_remote(iddma_transfer_destination type, int idx, const iddma_queue_element* src);
    bool update_remote(bool is_send, int req_tail, int cpl_head);

    bool wait_sync(void* base, bool& target, bool wait_value);
    bool wait_sync(void* base, uint32_t& target, uint32_t wait_value);

    void initialize(int id, const std::string& dir, bool is_listen);
    void* create_ipc_info(void);
    const void* create_ipc_info(void) const;

    template<typename T> bool update_core(T& target, const T& value);
    template<typename T> bool wait_sync_core(T& target, const T& value);

    static const uint32_t s_retry_max_;
    static const uint32_t s_max_token_size_;
    static const struct timespec s_tick_;
    static const uint32_t s_first_magic_ = 0x32da8df0;
    static const uint32_t s_second_magic_ = 0xefa0b23f;

    std::string ftok_filepath_;
    int timeout_sec_;
    int ftok_id_;
    bool is_acceptor_;
    bool is_connector_;

    void* ipc_info_;
    void* mmap_share_data_;
    void* mmap_share_token_;
    int ipc_info_id_;
    int mmap_share_data_id_;
    int mmap_share_token_id_;

    std::vector<std::vector<char>> buffers_;
};

#endif // _TCP_CTRL_TUNNEL_HPP__
