/*************************************************
* Copyright 2025 NTT Corporation
* Licensed under the 3-Clause BSD License, see LICENSE for details.
* SPDX-License-Identifier: BSD-3-Clause
*************************************************/
/**
 * @file iddma_common.hpp
 * @brief Common class object of iddma.
 *
 */

#ifndef __IDDMA_COMMON_H__

#define __IDDMA_COMMON_H__

#include <iddma.h>
#include <iddma_memory_map.hpp>
#include <ctrl_tunnel.hpp>
#include <iddma_engine.hpp>
#include <iddma_cuda_util.hpp>
#include <iddma_common_def.hpp>
#include <cstdio>
#include <cstdint>
#include <cstring>
#include <map>
#include <memory>
#include <queue>
#include <set>
#include <vector>

/** @enum iddma_transfer_ready_status
 *
 */
typedef enum {
    KIDDMA_TRANSFER_READY_NONE = 0,
    KIDDMA_TRANSFER_READY_SEND = 1,
    KIDDMA_TRANSFER_READY_RECV = 2,
    KIDDMA_TRANSFER_READY_BOTH = 3,
    KIDDMA_TRANSFER_THIS_CQSEND_FULL = 4,
    KIDDMA_TRANSFER_THIS_CQRECV_FULL = 8,
    KIDDMA_TRANSFER_COUNTERPART_CQSEND_FULL = 16,
    KIDDMA_TRANSFER_COUNTERPART_CQRECV_FULL = 32,
    KIDDMA_TRANSFER_CLOSE_REQUEST = 128,
} iddma_transfer_ready_status;

/** @enum iddma_communication_direction */
typedef enum {
    KIDDMA_DIRECTION_SEND = 0,
    KIDDMA_DIRECTION_RECV = 1
} iddma_communication_direction;

typedef enum {
    KIDDMA_TRANSFER_MODE_SEND = 1,
    KIDDMA_TRANSFER_MODE_RECV = 2,
    KIDDMA_TRANSFER_MODE_BOTH = 3,
} iddma_transfer_mode;

class iddma_common {
public:
    struct shared_memory_info {
        key_t key;
        int id;
        uint64_t address;
        shared_memory_info(void)
            : key(0),
              id(0),
              address(0) {}
    };

    virtual ~iddma_common(void);

    iddma_status initialize(const char **options);
    virtual void finalize(void);

    iddma_status send(const void *addr, size_t size_byte, uint64_t imm);
    iddma_status recv(void *addr, size_t size_byte);
    iddma_status poll_send(uint64_t timeout_sec, iddma_queue_element *data);
    iddma_status poll_recv(uint64_t timeout_sec, iddma_queue_element *data);

    virtual iddma_device_object get_device_object(void) = 0;

    iddma_status listen(int id = 0, const std::string &directory = "./");
    iddma_status accept(void);
    iddma_status connect(int id = 0, const std::string &directory = "./");
    iddma_status close(void);

    virtual iddma_status mmap_populate(void *addr, size_t size) = 0;
    iddma_status mmap_share(void);

protected:
    iddma_common(iddma_device_type device_type);

    struct device_info {
        iddma_device_type device_type;
    };

    struct base_config {
        bool server_mode;
        iddma_dma_protocol protocol;
        iddma_transfer_mode transfer_mode; // transfer mode (send: send only, recv: recv only, both: send and recv)
        uint32_t poll_interval_ns;
        int ftok_id; /* base id used in ftok() */
        int32_t timeout_sec;
        std::string counter_ip_addr;
        uint16_t counter_tcp_port;
        std::string self_ip_addr;
        float limit_gbps;
        base_config(void);
    };

    // pure virtual functions
    // api's sub function
    /*
    void post_close(void)
    used in the final part of close()
    ex. release device specified buffer allocated for mmap function
    */
    virtual void on_close(void);
    virtual void post_close(void);

    /*
    iddma_status post_mmap_share(void)
    used in mmap_share()
    ex. GPU constructs devicde side interval list
    */
    virtual iddma_status post_mmap_share(void);

    // queue management
    /*
    iddma_status send_and_recv(void *addr, size_t size_byte, bool is_send)
    addr: ptr set to send / recv queue element
    size_byte: byte size of transfer
    is_send: true = send, false = recv
    implementation of send() and recv()
    */
    virtual iddma_status send_and_recv(void *addr, size_t size_byte, uint64_t imm, bool is_send) = 0;

    /*
    iddma_status poll_send_and_recv(uint64_t timeout_sec, iddma_queue_element *data, bool is_send)
    timeout_sec: polling will timeout in timeout_sec secs
    data: pointer to iddma_queue_element (completed queue element data will be copied)
    is_send: true = poll_send, false = poll_recv
    */
    virtual iddma_status poll_send_and_recv(uint64_t timeout_sec, iddma_queue_element *data, bool is_send) = 0;

    // memory management
    /*
    iddma_status allocate_queue_set(iddma_queue_set **queue_buffer)
    queue_buffer: set iddma_queue_set pointer to be allocated (usually queue_set_)
    allocate iddma_queue_set
    */
    virtual iddma_status allocate_queue_set(iddma_queue_set **queue_set) = 0;
    virtual void free_queue_set(iddma_queue_set *queue_buffer) = 0;

    // device info
    /*
    */
    virtual void get_acceptor_device_info(int& acceptor_dev_id, int& acceptor_ch_id);
    virtual void get_connector_device_info(int& connector_dev_id, int& connector_ch_id);
    virtual void update_acceptor_device_info(int acceptor_dev_id, int acceptor_ch_id);
    virtual void update_connector_device_info(int connector_dev_id, int connector_ch_id);

    // tcp engine override
    /*
    */
    virtual iddma_status create_ctrl_tunnel_tcp(bool is_listen, std::unique_ptr<ctrl_tunnel>& tunnel);

    // common function
    // check function
    /*
    bool check_connection(void)
    check if counterpart process / device is connected
    */
    bool check_connection(void);
    /*
    bool check_initialization(void)
    check if initialize() is called before
    */
    bool check_initialization(void);

    virtual bool check_option(const std::string& key, const std::string& value);
    virtual void check_protocol(void);
    bool is_num_value(const std::string& value, int64_t& value_num);
    bool is_fp_num_value(const std::string& value, float& value_num);

    // DMA worker by TCP
    virtual iddma_status enable_dma_engine(void);
    void disable_dma_engine(void);

    // variables
    // queue buffer info
    iddma_queue_set *queue_set_;             /* pointer to queue_buffer metadata struct */
    iddma_queue_set *counterpart_queue_set_; /* pointer to queue_buffer metadata struct for counterpart */
    // device info
    iddma_device_type device_type_;                /* this device's device type (ex. KIDDMA_DEVICE_TYPE_GPU_NV) */
    iddma_device_type counterpart_device_type_;    /* counterpart's device type (ex. KIDDMA_DEVICE_TYPE_GPU_NV) */
    // memory map metainfo to be shared
    /*
    use mmap_populate() to add each map (addr, handle) to memory_map_export_
    in mmap_share(), import from counterpart to memory_map_import_
    */
    std::vector<struct mmap_share_data> memory_map_export_;
    std::vector<struct mmap_share_data> memory_map_import_;

    base_config base_config_;
    std::unique_ptr<iddma_engine> dma_engine_;

    std::unique_ptr<iddma_cuda_util> cuda_util_;

private:
    // common internal function
    // api's sub function
    /*
    iddma_status mmap_import(void)
    import counterpart exported mmap
    */
    iddma_status mmap_import(void);

    /*
    iddma_status mmap_export(void)
    export mmap created by mmap_populate()
    */
    iddma_status mmap_export(void);
    iddma_status get_mmap_export_info(int idx, struct mmap_share_data& cur_map);

    bool is_tcp_ctrl(void) const;
    iddma_status create_ctrl_tunnel(int id, const std::string& directory, bool is_listen);
    iddma_status release_ctrl_tunnel(void);

    iddma_status create_ctrl_tunnel_shmem(int id, const std::string& directory, bool is_listen);

    // memory management
    /*
    iddma_status obtain_sharedmemory(std::string filepath, int id, struct shared_memory_info *shmem_info, size_t size)
    filepath: path to ftok() file
    id: id used by ftok()
    shmem_info: allocated shared mem metadata written into this
    size: byte size of allocation
    */
   iddma_status obtain_sharedmemory(std::string filepath, int id, struct shared_memory_info *shmem_info, size_t size);

    // option parse function
    void parse_options(const char **options);

    static const uint32_t s_first_magic_ = 0x32da8df0;
    static const uint32_t s_second_magic_ = 0xefa0b23f;

    // ipc info
    bool is_initialized_; /* initialized flag */
    std::string ftok_filepath_; /* filepath to ftok() file */
    bool is_connector_; /* if this device connect() to counterpart, this will be true */
    bool is_acceptor_;  /* if this device accept() counterpart, this will be true */
    struct shared_memory_info ipc_info_; /* interprocess information that needs for synchronization, data exchange */
    struct shared_memory_info memmap_shared_data_;    /* used in mmap_share(). usually indicates original device pointer */

    std::unique_ptr<ctrl_tunnel> ctrl_tunnel_;

    std::vector<uint8_t> memmap_token_data_;
};

#endif
