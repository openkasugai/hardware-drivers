/*************************************************
* Copyright 2024 NTT Corporation, FUJITSU LIMITED
* Licensed under the 3-Clause BSD License, see LICENSE for details.
* SPDX-License-Identifier: BSD-3-Clause
*************************************************/
#ifndef __TP_H__
#define __TP_H__

#include <stdint.h>
#include "bcdbg.h"


//-----------------------------------------------------
// variable
//-----------------------------------------------------
extern const char *tp_model_name[];

//-----------------------------------------------------
// function
//-----------------------------------------------------
// tp main
extern int32_t tp_host_host(void);
extern int32_t tp_d2d_h_host_host(void);
extern int32_t tp_d2d_d_host_host(void);
// tp modules
extern int32_t tp_shmem_allocate(shmem_mode_t shmem_mode, mngque_t *pque);
extern int32_t tp_shmem_free(mngque_t *pque);
extern int32_t tp_allocate_buffer(void);
extern void tp_free_buffer(void);
extern int32_t tp_open_moviefile(void);
extern int32_t tp_generate_send_image_data(uint32_t run_id);
extern int32_t tp_generate_send_image_ppm(uint32_t run_id);
extern int32_t tp_set_frame_shmem_src(void);
extern int32_t tp_function_filter_resize_init(uint32_t dev_id);
extern int32_t tp_enqueue_fdma_init(uint32_t dev_id);
extern int32_t tp_dequeue_fdma_init(uint32_t dev_id);
extern int32_t tp_enqueue_fdma_queue_setup(uint32_t dev_id);
extern int32_t tp_dequeue_fdma_queue_setup(uint32_t dev_id);
extern int32_t tp_enqueue_set_dma_cmd(uint32_t run_id, uint32_t enq_num, mngque_t *pque);
extern int32_t tp_dequeue_set_dma_cmd(uint32_t run_id, uint32_t enq_num, mngque_t *pque);
extern int32_t tp_chain_connect(uint32_t dev_id);
extern int32_t tp_fpga_buf_connect(mngque_t *pque);
extern int32_t tp_fpga_direct_connect(void);
extern void tp_enqueue_fdma_queue_finish(uint32_t dev_id);
extern void tp_dequeue_fdma_queue_finish(uint32_t dev_id);
extern void tp_function_finish(uint32_t dev_id);
extern void tp_enqueue_fdma_finish(uint32_t dev_id);
extern void tp_dequeue_fdma_finish(uint32_t dev_id);
extern void tp_chain_disconnect(uint32_t dev_id);
extern void tp_fpga_buf_disconnect(void);
extern void tp_fpga_direct_disconnect(void);
extern int32_t tp_outppm_send_data(uint32_t run_id, uint32_t enq_num);
extern int32_t tp_check_dev_to_dev_frame_size(uint32_t tx_dev_id, uint32_t rx_dev_id);

#endif /* __TP_H__ */
