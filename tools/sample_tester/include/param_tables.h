/*************************************************
* Copyright 2024 NTT Corporation, FUJITSU LIMITED
* Licensed under the 3-Clause BSD License, see LICENSE for details.
* SPDX-License-Identifier: BSD-3-Clause
*************************************************/
#ifndef __PARAM_TABLES_H__
#define __PARAM_TABLES_H__

#include <stdint.h>
//-----------------------------------------------------
// parameter
//-----------------------------------------------------
typedef struct framesize {
	uint32_t height_in;
	uint32_t width_in;
	uint32_t height_out;
	uint32_t width_out;
} framesize_t;

typedef struct chain_ctrl_cid {
	uint32_t function_chid;
	uint32_t fdma_extif_id;
	uint32_t fdma_cid;
} chain_ctrl_cid_t;

typedef struct chain_flags {
	uint8_t direct_flag;
	uint8_t ig_active_flag;
	uint8_t eg_active_flag;
	uint8_t eg_virtual_flag;
	uint8_t eg_blocking_flag;
} chain_flags_t;

typedef struct connector_id {
	char *enq_id;
	char *deq_id;
} connector_id_t;

typedef struct input_setting {
	uint32_t fps;
	uint32_t framenum;
	uint32_t tx_wait;
} input_setting_t;

//-----------------------------------------------------
// function
//-----------------------------------------------------
extern const char* getparam_moviefile(uint32_t ch_id);

extern uint32_t getparam_frame_height_in(uint32_t dev_id, uint32_t ch_id);
extern uint32_t getparam_frame_width_in(uint32_t dev_id, uint32_t ch_id);
extern uint32_t getparam_frame_height_out(uint32_t dev_id, uint32_t ch_id);
extern uint32_t getparam_frame_width_out(uint32_t dev_id, uint32_t ch_id);

extern uint32_t getparam_function_chid(uint32_t ch_id);
extern uint32_t getparam_fdma_extif_id(uint32_t ch_id);
extern uint32_t getparam_fdma_cid(uint32_t ch_id);

extern bool getparam_is_direct_flag(uint32_t dev_id, uint32_t ch_id);
extern bool getparam_is_ig_active_flag(uint32_t dev_id, uint32_t ch_id);
extern bool getparam_is_eg_active_flag(uint32_t dev_id, uint32_t ch_id);
extern bool getparam_is_eg_virtual_flag(uint32_t dev_id, uint32_t ch_id);
extern bool getparam_is_eg_blocking_flag(uint32_t dev_id, uint32_t ch_id);

extern const char* getparam_enq_connector_id(uint32_t ch_id);
extern const char* getparam_deq_connector_id(uint32_t ch_id);

#endif /* __PARAM_TABLES_H__ */
