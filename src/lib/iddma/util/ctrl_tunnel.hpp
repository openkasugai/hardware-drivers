/*************************************************
* Copyright 2025 NTT Corporation
* Licensed under the 3-Clause BSD License, see LICENSE for details.
* SPDX-License-Identifier: BSD-3-Clause
*************************************************/

#ifndef _CTRL_TUNNEL_HPP__
#define _CTRL_TUNNEL_HPP__

#include <cstdint>
#include <iddma_def.h>
#include <iddma_common_def.hpp>
#include <string>
#include <vector>

class iddma_engine;

class ctrl_tunnel {
public:
    virtual ~ctrl_tunnel(void) {}

    virtual iddma_status listen(iddma_queue_set* queue_set, iddma_device_type dev_type,
                                int acceptor_dev_id, int acceptor_ch_id) = 0;
    virtual iddma_status accept(iddma_queue_set*& cp_queue_set, iddma_device_type& cp_dev_type,
                                int& connector_dev_id, int& connector_ch_id) = 0;
    virtual iddma_status connect(iddma_queue_set* queue_set, iddma_device_type dev_type,
                                 iddma_queue_set*& cp_queue_set, iddma_device_type& cp_dev_type,
                                 int connector_dev_id, int connector_ch_id,
                                 int& acceptor_dev_id, int& acceptor_ch_id) = 0;
    virtual iddma_status close_pre(void) = 0;
    virtual iddma_status close_post(void) = 0;
    virtual bool is_connected(void) const = 0;

    virtual iddma_status mmap_import(int& map_size) = 0;
    virtual iddma_status mmap_export(int map_size) = 0;
    virtual iddma_status execute_mmap_import(struct mmap_share_data& map_data) = 0;
    virtual iddma_status execute_mmap_export(struct mmap_share_data& tmp_shdat) = 0;
    virtual bool process_synchronize(void) = 0;

    virtual void finalize(void) = 0;

protected:
    ctrl_tunnel(void) {}
};

#endif // _CTRL_TUNNEL_HPP__
