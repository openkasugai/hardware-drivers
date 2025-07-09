/*************************************************
* Copyright 2025 NTT Corporation
* Licensed under the 3-Clause BSD License, see LICENSE for details.
* SPDX-License-Identifier: BSD-3-Clause
*************************************************/
/**
 * @file iddma_engine.hpp
 * @brief DMA engine base class object of iddma.
 *
 */

#ifndef __IDDMA_ENGINE_HPP__
#define __IDDMA_ENGINE_HPP__

#include <iddma_def.h>
#include <cstdint>

class ctrl_tunnel;

class iddma_engine {
public:
    virtual ~iddma_engine(void) {};

    virtual void wakeup_workers(void) {}
    virtual iddma_status write_queue_element(iddma_transfer_destination type, int idx, const iddma_queue_element* src) { return KIDDMA_SUCCESS; }
    virtual iddma_status write_queue_head(iddma_transfer_destination type, uint32_t head) { return KIDDMA_SUCCESS; }
    virtual iddma_status write_queue_tail(iddma_transfer_destination type, uint32_t tail) { return KIDDMA_SUCCESS; }

    void set_ctrl_tunnel(ctrl_tunnel* tunnel) { ctrl_tunnel_ = tunnel; ctrl_tunnel_set_ = true; wakeup_workers(); }

protected:
    iddma_engine(void)
        : queue_set_(nullptr), ctrl_tunnel_(nullptr), ctrl_tunnel_set_(false) {}
    explicit iddma_engine(iddma_queue_set* queue_set)
        : queue_set_(queue_set), ctrl_tunnel_(nullptr), ctrl_tunnel_set_(false) {}

    iddma_queue_set* queue_set_;
    ctrl_tunnel* ctrl_tunnel_;
    bool ctrl_tunnel_set_;

private:
    iddma_engine(const iddma_engine&) = delete;
};

#endif // __IDDMA_ENGINE_HPP__
