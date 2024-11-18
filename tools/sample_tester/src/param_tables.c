/*************************************************
* Copyright 2024 NTT Corporation, FUJITSU LIMITED
* Licensed under the 3-Clause BSD License, see LICENSE for details.
* SPDX-License-Identifier: BSD-3-Clause
*************************************************/
#include <stdio.h>
#include <stdlib.h>
#include "common.h"
#include "bcdbg.h"
#include "param_tables.h"


//-----------------------------------------------------
// Test Parameter Setting
//-----------------------------------------------------
// Set Frame Size
static const framesize_t g_param_framesizes_func[][CH_NUM_MAX] = {
	{
	/* { INPUT HEIGHT, INPUT WIDTH, OUTPUT HEIGHT, OUTPUT WIDTH } */
		{ 2160, 3840, 1280, 1280 },
		{ 2160, 3840, 1280, 1280 },
		{ 2160, 3840, 1280, 1280 },
		{ 2160, 3840, 1280, 1280 },
		{ 2160, 3840, 1280, 1280 },
		{ 2160, 3840, 1280, 1280 },
		{ 2160, 3840, 1280, 1280 },
		{ 2160, 3840, 1280, 1280 },
		{ 2160, 3840, 1280, 1280 },
		{ 2160, 3840, 1280, 1280 },
		{ 2160, 3840, 1280, 1280 },
		{ 2160, 3840, 1280, 1280 },
		{ 2160, 3840, 1280, 1280 },
		{ 2160, 3840, 1280, 1280 },
		{ 2160, 3840, 1280, 1280 },
		{ 2160, 3840, 1280, 1280 },
	},
	{
		{ 1280, 1280, 1280, 1280 },
		{ 1280, 1280, 1280, 1280 },
		{ 1280, 1280, 1280, 1280 },
		{ 1280, 1280, 1280, 1280 },
		{ 1280, 1280, 1280, 1280 },
		{ 1280, 1280, 1280, 1280 },
		{ 1280, 1280, 1280, 1280 },
		{ 1280, 1280, 1280, 1280 },
		{ 1280, 1280, 1280, 1280 },
		{ 1280, 1280, 1280, 1280 },
		{ 1280, 1280, 1280, 1280 },
		{ 1280, 1280, 1280, 1280 },
		{ 1280, 1280, 1280, 1280 },
		{ 1280, 1280, 1280, 1280 },
		{ 1280, 1280, 1280, 1280 },
		{ 1280, 1280, 1280, 1280 },
	}
};

// Chain Control Settings
static const chain_ctrl_cid_t g_param_chain_ctrl_tbls[CH_NUM_MAX] = {
	/* { FUNCTION CHID, FDMA EXTIF ID, FDMA CID} */
	/* Lane#0 */
	{ 0, 0, 0 },
	{ 1, 0, 1 },
	{ 2, 0, 2 },
	{ 3, 0, 3 },
	{ 4, 0, 4 },
	{ 5, 0, 5 },
	{ 6, 0, 6 },
	{ 7, 0, 7 },
	/* Lane#1 */
	{ 0, 0, 8 },
	{ 1, 0, 9 },
	{ 2, 0, 10 },
	{ 3, 0, 11 },
	{ 4, 0, 12 },
	{ 5, 0, 13 },
	{ 6, 0, 14 },
	{ 7, 0, 15 },
};
// set chain control flag
static const chain_flags_t g_param_chain_flags_tbls[][CH_NUM_MAX] = {
	/* IG_DIRECT_FLAG, IG_ACTIVE_FLAG, EG_ACTIVE_FLAG, EG_VIRTUAL_FLAG, EG_BLOCKING_FLAG */
	{
		{ 0, 1, 1, 0, 1 },
		{ 0, 1, 1, 0, 1 },
		{ 0, 1, 1, 0, 1 },
		{ 0, 1, 1, 0, 1 },
		{ 0, 1, 1, 0, 1 },
		{ 0, 1, 1, 0, 1 },
		{ 0, 1, 1, 0, 1 },
		{ 0, 1, 1, 0, 1 },
		{ 0, 1, 1, 0, 1 },
		{ 0, 1, 1, 0, 1 },
		{ 0, 1, 1, 0, 1 },
		{ 0, 1, 1, 0, 1 },
		{ 0, 1, 1, 0, 1 },
		{ 0, 1, 1, 0, 1 },
		{ 0, 1, 1, 0, 1 },
		{ 0, 1, 1, 0, 1 },
	},
	{
		{ 1, 1, 1, 0, 0 },
		{ 1, 1, 1, 0, 0 },
		{ 1, 1, 1, 0, 0 },
		{ 1, 1, 1, 0, 0 },
		{ 1, 1, 1, 0, 0 },
		{ 1, 1, 1, 0, 0 },
		{ 1, 1, 1, 0, 0 },
		{ 1, 1, 1, 0, 0 },
		{ 1, 1, 1, 0, 0 },
		{ 1, 1, 1, 0, 0 },
		{ 1, 1, 1, 0, 0 },
		{ 1, 1, 1, 0, 0 },
		{ 1, 1, 1, 0, 0 },
		{ 1, 1, 1, 0, 0 },
		{ 1, 1, 1, 0, 0 },
		{ 1, 1, 1, 0, 0 },
	}

};
// movie file for image data expected value generation
// Generate filter/resize expectations
static const char *g_param_moviefile[CH_NUM_MAX] = {
	"/home/data/4K_ch00.mp4",
	"/home/data/4K_ch01.mp4",
	"/home/data/4K_ch02.mp4",
	"/home/data/4K_ch03.mp4",
	"/home/data/4K_ch04.mp4",
	"/home/data/4K_ch05.mp4",
	"/home/data/4K_ch06.mp4",
	"/home/data/4K_ch07.mp4",
	"/home/data/4K_ch08.mp4",
	"/home/data/4K_ch09.mp4",
	"/home/data/4K_ch10.mp4",
	"/home/data/4K_ch11.mp4",
	"/home/data/4K_ch12.mp4",
	"/home/data/4K_ch13.mp4",
	"/home/data/4K_ch14.mp4",
	"/home/data/4K_ch15.mp4",
};

// Set Connector ID
static const connector_id_t g_param_connector_id[CH_NUM_MAX] = {
	/* { ENQUEUE, DEQUEUE } */
	{ "enq_connector_id0", "deq_connector_id0" },
	{ "enq_connector_id1", "deq_connector_id1" },
	{ "enq_connector_id2", "deq_connector_id2" },
	{ "enq_connector_id3", "deq_connector_id3" },
	{ "enq_connector_id4", "deq_connector_id4" },
	{ "enq_connector_id5", "deq_connector_id5" },
	{ "enq_connector_id6", "deq_connector_id6" },
	{ "enq_connector_id7", "deq_connector_id7" },
	{ "enq_connector_id8", "deq_connector_id8" },
	{ "enq_connector_id9", "deq_connector_id9" },
	{ "enq_connector_id10", "deq_connector_id10" },
	{ "enq_connector_id11", "deq_connector_id11" },
	{ "enq_connector_id12", "deq_connector_id12" },
	{ "enq_connector_id13", "deq_connector_id13" },
	{ "enq_connector_id14", "deq_connector_id14" },
	{ "enq_connector_id15", "deq_connector_id15" },
};


//-----------------------------------------------------
// getparam Function
//-----------------------------------------------------
// framesize (func)
uint32_t getparam_frame_height_in(uint32_t dev_id, uint32_t ch_id)
{
	return g_param_framesizes_func[dev_id][ch_id].height_in;
}
uint32_t getparam_frame_width_in(uint32_t dev_id, uint32_t ch_id)
{
	return g_param_framesizes_func[dev_id][ch_id].width_in;
}
uint32_t getparam_frame_height_out(uint32_t dev_id, uint32_t ch_id)
{
	return g_param_framesizes_func[dev_id][ch_id].height_out;
}
uint32_t getparam_frame_width_out(uint32_t dev_id, uint32_t ch_id)
{
	return g_param_framesizes_func[dev_id][ch_id].width_out;
}

// chain control
uint32_t getparam_function_chid(uint32_t ch_id)
{
	return g_param_chain_ctrl_tbls[ch_id].function_chid;
}
uint32_t getparam_fdma_extif_id(uint32_t ch_id)
{
	return g_param_chain_ctrl_tbls[ch_id].fdma_extif_id;
}
uint32_t getparam_fdma_cid(uint32_t ch_id)
{
	return g_param_chain_ctrl_tbls[ch_id].fdma_cid;
}

// chain control flags
bool getparam_is_direct_flag(uint32_t dev_id, uint32_t ch_id)
{
	return g_param_chain_flags_tbls[dev_id][ch_id].direct_flag == 0 ? false : true;
}
bool getparam_is_ig_active_flag(uint32_t dev_id, uint32_t ch_id)
{
	return g_param_chain_flags_tbls[dev_id][ch_id].ig_active_flag == 0 ? false : true;
}
bool getparam_is_eg_active_flag(uint32_t dev_id, uint32_t ch_id)
{
	return g_param_chain_flags_tbls[dev_id][ch_id].eg_active_flag == 0 ? false : true;
}
bool getparam_is_eg_virtual_flag(uint32_t dev_id, uint32_t ch_id)
{
	return g_param_chain_flags_tbls[dev_id][ch_id].eg_virtual_flag == 0 ? false : true;
}
bool getparam_is_eg_blocking_flag(uint32_t dev_id, uint32_t ch_id)
{
	return g_param_chain_flags_tbls[dev_id][ch_id].eg_blocking_flag == 0 ? false : true;
}

// verify moviefile
const char* getparam_moviefile(uint32_t ch_id)
{
	return g_param_moviefile[ch_id];
}

// connector_id
const char* getparam_enq_connector_id(uint32_t ch_id)
{
	return g_param_connector_id[ch_id].enq_id;
}
const char* getparam_deq_connector_id(uint32_t ch_id)
{
	return g_param_connector_id[ch_id].deq_id;
}
