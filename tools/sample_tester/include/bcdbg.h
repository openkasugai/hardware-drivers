/*************************************************
* Copyright 2024 NTT Corporation, FUJITSU LIMITED
* Licensed under the 3-Clause BSD License, see LICENSE for details.
* SPDX-License-Identifier: BSD-3-Clause
*************************************************/
#ifndef __BCDBG_H__
#define __BCDBG_H__

#include <stdint.h>
#include <stdatomic.h>
#include "libdmacommon.h"
#include "liblldma.h"
#include "libchain.h"
#include "libdirecttrans.h"
#include "libfunction_conv.h"
#include "libfunction_filter_resize.h"
//-----------------------------------------------------
// define
//-----------------------------------------------------
#define VERSION	"0.1.00"
#define INTERNAL_VER

#define DATA_SIZE_1KB	1024
#define DATA_SIZE_4KB	4096
#define CH_NUM_MAX	16
#define DEQ_NUM_MAX	255
#define CORE_NUM_MAX	64
#define SHMEMALLOC_NUM_MAX	10
#define ALIGN_SRC_LEN	64
#define ALIGN_DST_LEN	64
#define GEN_IMG_PARALLEL_NUM	8
#define DUMP_PPM_NUM_MAX	255

#define LANE_NUM_MAX		2
#define CHAIN_KRNL_NUM_MAX	2
#define CONV_KRNL_NUM_MAX	2
#define FUNCTION_KRNL_NUM_MAX	2
#define FR_NUM_MAX		2
#define EXTIFID	0

#define SEND_DATA_DIR		"send_data"
#define RECEIVE_DATA_DIR	"receive_data"

#define IMG_WIDTH_MAX		3840
#define IMG_HEIGHT_MAX		2160
#define IMG_DATA_SIZE_MAX	(IMG_WIDTH_MAX * IMG_HEIGHT_MAX * 3)

typedef enum TP_MODEL {
	TP_HOST_HOST,
	TP_D2D_H_HOST_HOST,
	TP_D2D_D_HOST_HOST,
	TP_UNKNOWN
} tp_model_t;

typedef enum SHMEM_MODE {
	SHMEM_SRC,
	SHMEM_DST,
	SHMEM_SRC_DST,
	SHMEM_DST1_DST2,
	SHMEM_SRC_DST1_DST2,
	SHMEM_D2D_SRC,
	SHMEM_D2D_DST,
	SHMEM_D2D_SRC_DST,
	SHMEM_D2D
} shmem_mode_t; 

typedef enum DIRECT_DIRTYPE {
	INGRESS_RCV,
	INGRESS_SND,
	EGRESS_RCV,
	EGRESS_SND,
	DIR_TYPE_MAX
} direct_dirtype_t;

typedef enum CHAIN_DIR {
	INGRESS,
	EGRESS,
	DIR_MAX
} chain_dir_t;

//-----------------------------------------------------
// frame header
//-----------------------------------------------------
typedef struct frameheader {
	uint32_t marker;
	uint32_t payload_len;
	uint8_t reserved1[4];
	uint32_t frame_index;//sequence_num
	uint8_t reserved2[8];
	uint64_t local_ts;//timestamp
	uint32_t channel_id;//data_id
	uint8_t reserved3[8];
	uint16_t h_checksum;
	uint8_t reserved4[2];
} frameheader_t;

//-----------------------------------------------------
// data format
//-----------------------------------------------------
typedef struct enqbuf {
	uint64_t *srcbufp;
	uint64_t *dst1bufp;
	uint64_t *dst2bufp;
} enqbuf_t;

typedef struct mngque {
	uint32_t enq_num;
	uint32_t srcdsize;
	uint32_t dst1dsize;
	uint32_t dst2dsize;
	uint32_t d2ddsize;
	uint32_t srcbuflen;
	uint32_t dst1buflen;
	uint32_t dst2buflen;
	uint32_t d2dbuflen;
	uint64_t *d2dbufp;
	enqbuf_t enqbuf[SHMEMALLOC_NUM_MAX];
} mngque_t;

typedef struct divide_que {
	uint32_t que_num; // Queues Per Split
	uint32_t que_num_rem; // number of split-queue residues
	uint32_t div_num; // number of queue splits
} divide_que_t;

//-----------------------------------------------------
// thread args
//-----------------------------------------------------
typedef struct thread_genimg_args {
	uint32_t ch_id;
	uint32_t run_id;
	int32_t *result;
} thread_genimg_args_t;

typedef struct thread_enq_args {
	uint32_t dev_id;
	uint32_t ch_id;
	uint32_t run_id;
	uint32_t enq_num;
} thread_enq_args_t;

typedef struct thread_deq_args {
	uint32_t dev_id;
	uint32_t ch_id;
	uint32_t run_id;
	uint32_t enq_num;
} thread_deq_args_t;

typedef struct thread_send_args {
	uint32_t ch_id;
	uint32_t run_id;
	uint32_t enq_num;
} thread_send_args_t;

typedef struct thread_receive_args {
	uint32_t ch_id;
	uint32_t run_id;
	uint32_t enq_num;
} thread_receive_args_t;

//-----------------------------------------------------
// timestamp
//-----------------------------------------------------
typedef struct timestamp {
	struct timespec start_time;
	struct timespec end_time;
} timestamp_t;

//-----------------------------------------------------
// mutex
//-----------------------------------------------------
extern pthread_mutex_t tx_shmmutex[CH_NUM_MAX][SHMEMALLOC_NUM_MAX];
extern pthread_mutex_t tx_recmutex[CH_NUM_MAX];

//-----------------------------------------------------
// function
//-----------------------------------------------------
extern void set_cmdname(const char *cmd);
extern void print_usage(void);
extern int32_t parse_app_args_func(int argc, char **argv);
extern int32_t check_options(void);

extern tp_model_t getopt_tp_model(void);
extern bool getopt_ch_en(uint32_t i);
extern uint32_t getopt_ch_num(uint32_t i);
extern uint32_t getopt_fps(void);
extern uint32_t getopt_frame_num(void);
extern uint32_t getopt_enq_num(void);
extern uint32_t getopt_shmalloc_num(void);

extern bool getopt_is_send_data(void);
extern bool getopt_is_receive_data(void);
extern bool getopt_is_outppm_send_data(void);
extern bool getopt_is_outppm_receive_data(void);
extern bool getopt_tester_meas_mode(void);
extern bool getopt_is_performance_meas(void);
extern uint32_t getopt_core(void);
extern int32_t getopt_loglevel(void);

extern uint32_t get_chain_krnl_id(uint32_t ch_id);
extern uint32_t get_conv_krnl_id(uint32_t ch_id);
extern uint32_t get_function_krnl_id(uint32_t ch_id);

extern int32_t shmem_malloc(shmem_mode_t mode, mngque_t* p, uint32_t ch_id);
extern int32_t shmem_free(const mngque_t* p, uint32_t ch_id);
extern int32_t deq_shmem_malloc(mngque_t* p, uint32_t ch_id);
extern int32_t deq_shmem_free(const mngque_t* p, uint32_t ch_id);
extern bool* get_deq_shmstate(uint32_t ch_id);
extern int64_t* get_deq_receivep(uint32_t ch_id);

extern void prlog_mngque(const mngque_t *p, uint32_t ch_id);
extern void prlog_divide_que(const divide_que_t *p);
extern int32_t prlog_dma_info(const dma_info_t *p, uint32_t ch_id);
extern int32_t prlog_dmacmd_info(const dmacmd_info_t *p, uint32_t ch_id, uint32_t enq_id);
extern int32_t prlog_connect_info(const fpga_lldma_connect_t *p, uint32_t ch_id);

extern fpga_lldma_connect_t* get_connectinfo(uint32_t ch_id);

extern int32_t set_dev_id_list(void);
extern uint32_t* get_dev_id(uint32_t index);
extern uint32_t dev_id_to_index(uint32_t dev_id);

extern int32_t dmacmdinfo_malloc(void);
extern void dmacmdinfo_free(void);
extern dma_info_t* get_enqdmainfo_channel(uint32_t dev_id, uint32_t ch_id);
extern dma_info_t* get_deqdmainfo_channel(uint32_t dev_id, uint32_t ch_id);
extern dma_info_t* get_enqdmainfo(uint32_t dev_id, uint32_t ch_id);
extern dma_info_t* get_deqdmainfo(uint32_t dev_id, uint32_t ch_id);
extern dmacmd_info_t* get_enqdmacmdinfo(uint32_t ch_id, uint32_t enq_id);
extern dmacmd_info_t* get_deqdmacmdinfo(uint32_t ch_id, uint32_t enq_id);
extern const divide_que_t* get_divide_que(void);

extern void thread_gen_sendimgdata(thread_genimg_args_t *args);
extern void thread_gen_sendimgppm(thread_genimg_args_t *args);
extern void thread_dma_tx_deq(thread_deq_args_t *args);
extern void thread_dma_tx_enq(thread_enq_args_t *args);
extern void thread_dma_rx_deq(thread_deq_args_t *args);
extern void thread_dma_rx_enq(thread_enq_args_t *args);
extern void thread_receive(thread_receive_args_t *args);
extern void thread_send(thread_send_args_t *args);

extern int32_t open_moviefile(uint32_t ch_id);
extern int32_t sendimg_malloc(uint32_t ch_id);
extern void* get_sendimg_addr(uint32_t ch_id);
extern void sendimg_free(uint32_t ch_id);
extern int32_t receiveheader_malloc(uint32_t ch_id);
extern void* get_receiveheader_addr(uint32_t ch_id);
extern void receiveheader_free(uint32_t ch_id);
extern int32_t receiveimg_malloc(uint32_t ch_id);
extern uint8_t* get_receiveimg_addr(uint32_t ch_id);
extern void receiveimg_free(uint32_t ch_id);
extern int32_t set_frame_shmem_src(uint32_t ch_id, uint32_t enq_id);
extern void pr_device_info(void);
extern int32_t timestamp_malloc(void);
extern void timestamp_free(void);
extern void timer_header_start(uint32_t ch_id, uint32_t enq_id, uint64_t timestamp);
extern void timer_rx_start(uint32_t ch_id, uint32_t enq_id);
extern void timer_rx_stop(uint32_t ch_id, uint32_t enq_id);
extern void timer_tx_start(uint32_t ch_id, uint32_t enq_id);
extern void timer_tx_stop(uint32_t ch_id, uint32_t enq_id);
extern void pr_perf_normal(void);
extern void pr_perf(void);
extern int32_t outppm_send_data(uint32_t ch_id, uint32_t enq_id);

#endif /* __BCDBG_H__ */
