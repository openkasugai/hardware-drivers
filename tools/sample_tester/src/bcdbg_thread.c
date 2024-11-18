/*************************************************
* Copyright 2024 NTT Corporation, FUJITSU LIMITED
* Licensed under the 3-Clause BSD License, see LICENSE for details.
* SPDX-License-Identifier: BSD-3-Clause
*************************************************/
#define _GNU_SOURCE
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sched.h>              //CPU_XX
#include <pthread.h>
#include <sys/types.h>          //gettid
#include <sys/syscall.h>        //syscall
#include "libdma.h"
#include "libdmacommon.h"
#include "liblogging.h"
#include "common.h"
#include "param_tables.h"
#include "bcdbg.h"
#include "cppfunc.h"

#define WAIT_TIME_DMA_RX_ENQUEUE	300000	//msec
#define WAIT_TIME_DMA_RX_DEQUEUE	300000	//msec
#define WAIT_TIME_DMA_TX_ENQUEUE	300000	//msec
#define WAIT_TIME_DMA_TX_DEQUEUE	300000	//msec

#define SHMEM_POLLING_INTERVAL	100 //usec

pthread_mutex_t tx_shmmutex[CH_NUM_MAX][SHMEMALLOC_NUM_MAX];
pthread_mutex_t tx_recmutex[CH_NUM_MAX];

static int32_t wait_dma_tx_fpga_dequeue(dma_info_t *dmainfo, dmacmd_info_t *dmacmdinfo, const uint32_t enq_id, const int32_t msec);
static int32_t wait_dma_tx_fpga_enqueue(dma_info_t *dmainfo, dmacmd_info_t *dmacmdinfo, const uint32_t enq_id, const int32_t msec);
static int32_t wait_dma_rx_fpga_dequeue(dma_info_t *dmainfo, dmacmd_info_t *dmacmdinfo, const uint32_t enq_id, const int32_t msec);
static int32_t wait_dma_rx_fpga_enqueue(dma_info_t *dmainfo, dmacmd_info_t *dmacmdinfo, const uint32_t enq_id, const int32_t msec);

int32_t get_cpunum()
{
	return sysconf(_SC_NPROCESSORS_CONF);
}
pid_t gettid(void) {
	return syscall(SYS_gettid);
}

//----------------------------------
// DMA TX Dequeue Thread
//----------------------------------
void thread_dma_tx_deq(thread_deq_args_t *args)
{
	uint32_t ch_id = args->ch_id;
	pthread_t thread = pthread_self();
	uint32_t core_no = getopt_core() + ch_id;
	if (core_no != 0xff) {
		int32_t cpu_num = get_cpunum();
		logfile(LOG_DEBUG, "CPU num(%u)\n", cpu_num);
		cpu_set_t *cpus = CPU_ALLOC(cpu_num);
		size_t sz = CPU_ALLOC_SIZE(cpu_num);
		CPU_ZERO_S(sz, cpus);
		CPU_SET_S(core_no, sz, cpus);
		int32_t result = pthread_setaffinity_np(thread, sizeof(cpu_set_t), cpus);
		if (result != 0) {
			logfile(LOG_ERROR, " sched_setaffinity error!(%d)\n", result);
		}
		CPU_FREE(cpus);
	}

	logfile(LOG_DEBUG, "CH(%u) ...thread_dma_tx_deq start...\n", ch_id);

	const divide_que_t *div_que = get_divide_que();
	bool *deq_shms = get_deq_shmstate(ch_id);
	uint32_t ring = 0;

	uint32_t dev_id = args->dev_id;
	dma_info_t *pdmainfo = get_deqdmainfo(dev_id, ch_id);
	rslt2file("CH(%u) DMA TX dma_info: dir(%d) chid(%u) queue_addr(%p) queue_size(%u)\n", ch_id, pdmainfo->dir, pdmainfo->chid, pdmainfo->queue_addr, pdmainfo->queue_size);
	uint32_t run_id = args->run_id;
	uint32_t enq_num = args->enq_num;
	for (size_t i=0; i < enq_num; i++) {
		uint32_t enq_id = i + run_id * div_que->que_num;
		if (! getopt_is_performance_meas())
			logfile(LOG_DEBUG, " thread_dma_tx_deq(%u): deq(%zu)\n", ch_id, enq_id);
		dmacmd_info_t *pdmacmdinfo = get_deqdmacmdinfo(ch_id, enq_id);
		if (! getopt_is_performance_meas()) {
			prlog_dma_info(pdmainfo, ch_id);
			prlog_dmacmd_info(pdmacmdinfo, ch_id, enq_id);
		}
		uint16_t task_id = pdmacmdinfo-> task_id;

		bool lp = true;
		while (lp) {
			pthread_mutex_lock( &tx_shmmutex[ch_id][ring] );
			// Check shared memory usage status [If false, write to shared memory is enabled]
			bool ds = deq_shms[ring];
			pthread_mutex_unlock( &tx_shmmutex[ch_id][ring] );
			if (!ds) {
				if (! getopt_is_performance_meas())
					logfile(LOG_DEBUG, "CH(%u) DMA TX deq(%zu) task_id(%u) deq_shms[%u]=false dequeue start\n", ch_id, enq_id, task_id, ring);
				lp = false;
			}
			usleep(SHMEM_POLLING_INTERVAL);
		}

		// dequeue data set
		if (! getopt_is_performance_meas())
			rslt2file("CH(%u) DMA TX dmacmd_info: deq(%zu) task_id(%u) dst_len(%u) dst_addr(%p)\n", ch_id, enq_id, pdmacmdinfo->task_id, pdmacmdinfo->data_len, pdmacmdinfo->data_addr);
		int32_t ret = wait_dma_tx_fpga_dequeue(pdmainfo, pdmacmdinfo, enq_id, WAIT_TIME_DMA_TX_DEQUEUE);
		timer_tx_stop(ch_id, enq_id);
		if (ret < 0) {
			logfile(LOG_ERROR, "DMA TX deqerror CH(%u) deq(%zu)\n", ch_id, enq_id);
		}

		// deq shmstate update
		pthread_mutex_lock( &tx_shmmutex[ch_id][ring] );
		deq_shms[ring] = true; // shared memory dequeued [changed to true]
		pthread_mutex_unlock( &tx_shmmutex[ch_id][ring] );

		ring++;
		if (ring >= getopt_shmalloc_num()) {
			ring = 0;
		}


		if (! getopt_is_performance_meas()) {
			//debug
			prlog_dma_info(pdmainfo, ch_id);
			prlog_dmacmd_info(pdmacmdinfo, ch_id, enq_id);
		}
		rslt2file("CH(%u) DMA TX dmacmd_info: deq(%zu) result_task_id(%u) result_status(%u) result_data_len(%u)\n", ch_id, enq_id, pdmacmdinfo->result_task_id, pdmacmdinfo->result_status, pdmacmdinfo->result_data_len);
	}

	logfile(LOG_DEBUG, "CH(%u) ...thread_dma_tx_deq end...\n", ch_id);
}

static int32_t wait_dma_tx_fpga_dequeue(dma_info_t *dmainfo, dmacmd_info_t *dmacmdinfo, const uint32_t enq_id, const int32_t msec)
{
	int32_t timeout = 100; //fpga_deuque timeout 100msec

	uint32_t ch_id = dmainfo->chid;
	uint16_t task_id = dmacmdinfo->task_id;

	int32_t cnt = msec/timeout;
	for (size_t i=0; i < cnt; i++) {
		int32_t ret = fpga_dequeue(dmainfo, dmacmdinfo);
		if (ret == 0) {
			return 0;
		}
	}

	logfile(LOG_ERROR, "  CH(%u) deq(%u) task_id(%u) DMA TX dequeue timeout!!!\n", ch_id, enq_id, task_id);

	return -1;
}


//----------------------------------
// DMA TX Enqueue Thread
//----------------------------------
void thread_dma_tx_enq(thread_enq_args_t *args)
{
	uint32_t ch_id = args->ch_id;
	pthread_t thread = pthread_self();
	uint32_t core_no = getopt_core() + ch_id;
	if (core_no != 0xff) {
		int32_t cpu_num = get_cpunum();
		logfile(LOG_DEBUG, "CPU num(%u)\n", cpu_num);
		cpu_set_t *cpus = CPU_ALLOC(cpu_num);
		size_t sz = CPU_ALLOC_SIZE(cpu_num);
		CPU_ZERO_S(sz, cpus);
		CPU_SET_S(core_no, sz, cpus);
		int32_t result = pthread_setaffinity_np(thread, sizeof(cpu_set_t), cpus);
		if (result != 0) {
			logfile(LOG_ERROR, " sched_setaffinity error!(%d)\n", result);
		}
		CPU_FREE(cpus);
	}

	logfile(LOG_DEBUG, "CH(%u) ...thread_dma_tx_enq start...\n", ch_id);

	const divide_que_t *div_que = get_divide_que();

	uint32_t dev_id = args->dev_id;
	dma_info_t *pdmainfo = get_deqdmainfo(dev_id, ch_id);
	uint32_t run_id = args->run_id;
	uint32_t enq_num = args->enq_num;
	for (size_t i=0; i < enq_num; i++) {
		uint32_t enq_id = i + run_id * div_que->que_num;
		dmacmd_info_t *pdmacmdinfo = get_deqdmacmdinfo(ch_id, enq_id);

		int32_t ret = wait_dma_tx_fpga_enqueue(pdmainfo, pdmacmdinfo, enq_id, WAIT_TIME_DMA_TX_ENQUEUE);
		timer_tx_start(ch_id, enq_id);
		if (ret < 0) {
			logfile(LOG_ERROR, "DMA TX enqerror CH(%u) enq(%zu)\n", ch_id, enq_id);
		}
	}

	logfile(LOG_DEBUG, "CH(%u) ...thread_dma_tx_enq end...\n", ch_id);
}

static int32_t wait_dma_tx_fpga_enqueue(dma_info_t *dmainfo, dmacmd_info_t *dmacmdinfo, const uint32_t enq_id, const int32_t msec)
{
	int32_t wait_msec = 100; // 100msec
	int32_t ret = 0;

	uint32_t ch_id = dmainfo->chid;
	uint32_t task_id = dmacmdinfo->task_id;

	int32_t cnt = msec/wait_msec;
	for (size_t i=0; i < cnt; i++) {
		ret = fpga_enqueue(dmainfo, dmacmdinfo);
		if (ret == 0) {
			return 0;
		} else if (ret == -ENQUEUE_QUEFULL) {
			logfile(LOG_DEBUG, "  CH(%u) deq(%u) task_id(%u) DMA TX fpga_enqueue que full(%d)\n", ch_id, enq_id, task_id, ret);
			usleep(wait_msec * 1000);
		} else {
			logfile(LOG_ERROR, "  CH(%u) deq(%u) task_id(%u) DMA TX fpga_enqueue error!!!(%d)\n", ch_id, enq_id, task_id, ret);
			return ret;
		}
	}

	logfile(LOG_ERROR, "  CH(%u) deq(%u) task_id(%u) DMA TX enqueue timeout!!!\n", ch_id, enq_id, task_id);

	return -1;
}

//----------------------------------
// DMA RX Enqueue Thread
//----------------------------------
void thread_dma_rx_enq(thread_enq_args_t *args)
{
	uint32_t ch_id = args->ch_id;
	pthread_t thread = pthread_self();
	uint32_t core_no = getopt_core() + ch_id;
	if (core_no != 0xff) {
		int32_t cpu_num = get_cpunum();
		logfile(LOG_DEBUG, "CPU num(%u)\n", cpu_num);
		cpu_set_t *cpus = CPU_ALLOC(cpu_num);
		size_t sz = CPU_ALLOC_SIZE(cpu_num);
		CPU_ZERO_S(sz, cpus);
		CPU_SET_S(core_no, sz, cpus);
		int32_t result = pthread_setaffinity_np(thread, sizeof(cpu_set_t), cpus);
		if (result != 0) {
			logfile(LOG_ERROR, " sched_setaffinity error!(%d)\n", result);
		}
		CPU_FREE(cpus);
	}

	logfile(LOG_DEBUG, "CH(%u) ...thread_dma_rx_enq start...\n", ch_id);

	const divide_que_t *div_que = get_divide_que();

	uint32_t dev_id = args->dev_id;
	dma_info_t *pdmainfo = get_enqdmainfo(dev_id, ch_id);
	uint32_t run_id = args->run_id;
	uint32_t enq_num = args->enq_num;
	uint32_t fps = getopt_fps();
	uint64_t fps_nsec = 0;
	if (fps > 0) {
		fps_nsec = 1000000000/fps; // nsec
	}
	for (size_t i=0; i < enq_num; i++) {
		uint32_t enq_id = i + run_id * div_que->que_num;
		dmacmd_info_t *pdmacmdinfo = get_enqdmacmdinfo(ch_id, enq_id);
		uint32_t task_id = pdmacmdinfo-> task_id;

		// FPS
		struct timespec t1, t2;
		if (fps > 0)
			clock_gettime(CLOCK_MONOTONIC, &t1);

		if (getopt_is_receive_data()) {
			int64_t remaintime = WAIT_TIME_DMA_RX_DEQUEUE * 1000; // usec
			bool lp = true;
			while (lp) {
				pthread_mutex_lock( &tx_recmutex[ch_id] );
				// Check shared memory received position of TX receiver
				int64_t *deq_receivep = get_deq_receivep(ch_id);
				pthread_mutex_unlock( &tx_recmutex[ch_id] );
				if (enq_id < *deq_receivep + SHMEMALLOC_NUM_MAX/2) {
					if (! getopt_is_performance_meas())
						logfile(LOG_DEBUG, "CH(%u) DMA RX enq(%zu) task_id(%u) deq_receivep(%ld) enqueue start\n", ch_id, enq_id, task_id, *deq_receivep);
					lp = false;
				}
				usleep(SHMEM_POLLING_INTERVAL);

				remaintime -= SHMEM_POLLING_INTERVAL;
				if (remaintime < 0)
					lp = false;
			}
		}

		// set frame
		int32_t ret = set_frame_shmem_src(ch_id, enq_id);
		if (ret < 0) {
			logfile(LOG_ERROR, "CH(%u) enq(%u) set_frame_shmem_src error(%d)\n", ch_id, enq_id, ret);
		}

		// enqueue data set
		ret = wait_dma_rx_fpga_enqueue(pdmainfo, pdmacmdinfo, enq_id, WAIT_TIME_DMA_RX_ENQUEUE);
		timer_rx_start(ch_id, enq_id);
		if (ret < 0) {
			logfile(LOG_ERROR, "DMA RX enqerror CH(%u) enq(%zu)\n", ch_id, enq_id);
		}

		// FPS
		if (fps > 0) {
			while (true) {
				clock_gettime(CLOCK_MONOTONIC, &t2);
				uint64_t duration = time_duration(&t1, &t2);
				if (duration > fps_nsec) {
					// FPS Time Elapsed
					logfile(LOG_DEBUG, "CH(%u) DMA RX enq(%u) fps duration time: %lu nsec (>%lu)\n", ch_id, enq_id, duration, fps_nsec);
					break;
				}
				usleep(1000);
			}
		}
	}

	logfile(LOG_DEBUG, "CH(%u) ...thread_dma_rx_enq end...\n", ch_id);
}

static int32_t wait_dma_rx_fpga_enqueue(dma_info_t *dmainfo, dmacmd_info_t *dmacmdinfo, const uint32_t enq_id, const int32_t msec)
{
	int32_t wait_msec = 100; // 100msec
	int32_t ret = 0;

	uint32_t ch_id = dmainfo->chid;
	uint32_t task_id = dmacmdinfo->task_id;

	int32_t cnt = msec/wait_msec;
	for (size_t i=0; i < cnt; i++) {
		ret = fpga_enqueue(dmainfo, dmacmdinfo);
		if (ret == 0) {
			return 0;
		} else if (ret == -ENQUEUE_QUEFULL) {
			logfile(LOG_DEBUG, "  CH(%u) enq(%u) task_id(%u) DMA RX fpga_enqueue que full(%d)\n", ch_id, enq_id, task_id, ret);
			usleep(wait_msec * 1000);
		} else {
			logfile(LOG_ERROR, "  CH(%u) enq(%u) task_id(%u) DMA RX fpga_enqueue error!!!(%d)\n", ch_id, enq_id, task_id, ret);
			return ret;
		}
	}

	logfile(LOG_ERROR, "  CH(%u) enq(%u) task_id(%u) DMA RX enqueue timeout!!!\n", ch_id, enq_id, task_id);

	return -1;
}

//----------------------------------
// DMA RX Dequeue Thread
//----------------------------------
void thread_dma_rx_deq(thread_deq_args_t *args)
{
	uint32_t ch_id = args->ch_id;
	pthread_t thread = pthread_self();
	uint32_t core_no = getopt_core() + ch_id;
	if (core_no != 0xff) {
		int32_t cpu_num = get_cpunum();
		logfile(LOG_DEBUG, "CPU num(%u)\n", cpu_num);
		cpu_set_t *cpus = CPU_ALLOC(cpu_num);
		size_t sz = CPU_ALLOC_SIZE(cpu_num);
		CPU_ZERO_S(sz, cpus);
		CPU_SET_S(core_no, sz, cpus);
		int32_t result = pthread_setaffinity_np(thread, sizeof(cpu_set_t), cpus);
		if (result != 0) {
			logfile(LOG_ERROR, " sched_setaffinity error!(%d)\n", result);
		}
		CPU_FREE(cpus);
	}

	logfile(LOG_DEBUG, "CH(%u) ...thread_dma_rx_deq start...\n", ch_id);

	const divide_que_t *div_que = get_divide_que();

	uint32_t dev_id = args->dev_id;
	dma_info_t *pdmainfo = get_enqdmainfo(dev_id, ch_id);
	rslt2file("CH(%u) DMA RX dma_info: dir(%d) chid(%u) queue_addr(%p) queue_size(%u)\n", ch_id, pdmainfo->dir, pdmainfo->chid, pdmainfo->queue_addr, pdmainfo->queue_size);
	uint32_t run_id = args->run_id;
	uint32_t enq_num = args->enq_num;
	for (size_t i=0; i < enq_num; i++) {
		uint32_t enq_id = i + run_id * div_que->que_num;
		if (! getopt_is_performance_meas())
			logfile(LOG_DEBUG, " thread_dma_rx_deq(%u): enq(%zu)\n", ch_id, enq_id);
		dmacmd_info_t *pdmacmdinfo = get_enqdmacmdinfo(ch_id, enq_id);
		if (! getopt_is_performance_meas()) {
			prlog_dma_info(pdmainfo, ch_id);
			prlog_dmacmd_info(pdmacmdinfo, ch_id, enq_id);
		}
		// dequeue data set
		if (! getopt_is_performance_meas())
			rslt2file("CH(%u) DMA RX dmacmd_info: enq(%zu) task_id(%u) src_len(%u) src_addr(%p)\n", ch_id, enq_id, pdmacmdinfo->task_id, pdmacmdinfo->data_len, pdmacmdinfo->data_addr);
		int32_t ret = wait_dma_rx_fpga_dequeue(pdmainfo, pdmacmdinfo, enq_id, WAIT_TIME_DMA_RX_DEQUEUE);
		timer_rx_stop(ch_id, enq_id);
		if (ret < 0) {
			logfile(LOG_ERROR, "DMA RX deqerror CH(%u) enq(%zu)\n", ch_id, enq_id);
		}

		if (! getopt_is_performance_meas()) {
			//debug
			prlog_dma_info(pdmainfo, ch_id);
			prlog_dmacmd_info(pdmacmdinfo, ch_id, enq_id);
			rslt2file("CH(%u) DMA RX dmacmd_info: enq(%zu) result_task_id(%u) result_status(%u) result_data_len(%u)\n", ch_id, enq_id, pdmacmdinfo->result_task_id, pdmacmdinfo->result_status, pdmacmdinfo->result_data_len);
		}
	}

	logfile(LOG_DEBUG, "CH(%u) ...thread_dma_rx_deq end...\n", ch_id);
}

static int32_t wait_dma_rx_fpga_dequeue(dma_info_t *dmainfo, dmacmd_info_t *dmacmdinfo, const uint32_t enq_id, const int32_t msec)
{
	int32_t timeout = 100; //fpga_deuque timeout 100msec

	uint32_t ch_id = dmainfo->chid;
	uint16_t task_id = dmacmdinfo->task_id;

	int32_t cnt = msec/timeout;
	for (size_t i=0; i < cnt; i++) {
		int32_t ret = fpga_dequeue(dmainfo, dmacmdinfo);
		if (ret == 0) {
			return 0;
		}
	}

	logfile(LOG_ERROR, "  CH(%u) enq(%u) task_id(%u) DMA RX dequeue timeout!!!\n", ch_id, enq_id, task_id);

	return -1;
}

//----------------------------------
// Generate send image Thread
//----------------------------------
void thread_gen_sendimgdata(thread_genimg_args_t *args)
{
	uint32_t ch_id = args->ch_id;
	pthread_t thread = pthread_self();
	uint32_t core_no = getopt_core() + ch_id;
	if (core_no != 0xff) {
		int32_t cpu_num = get_cpunum();
		logfile(LOG_DEBUG, "CPU num(%u)\n", cpu_num);
		cpu_set_t *cpus = CPU_ALLOC(cpu_num);
		size_t sz = CPU_ALLOC_SIZE(cpu_num);
		CPU_ZERO_S(sz, cpus);
		CPU_SET_S(core_no, sz, cpus);
		int32_t result = pthread_setaffinity_np(thread, sizeof(cpu_set_t), cpus);
		if (result != 0) {
			logfile(LOG_ERROR, " sched_setaffinity error!(%d)\n", result);
		}
		CPU_FREE(cpus);
	}

	logfile(LOG_DEBUG, "CH(%u) ...thread_gen_sendimgdata start...\n", ch_id);

	int32_t *rslt = args->result;
	uint8_t *img = get_sendimg_addr(ch_id);
	size_t gen_frame_num = getopt_frame_num();
	if (getopt_tester_meas_mode()) {
		gen_frame_num = 1;
	}

	const char *moviefile = getparam_moviefile(ch_id);
	uint32_t *dev_id = get_dev_id(0);
	uint32_t index = dev_id_to_index(*dev_id);
	size_t height = getparam_frame_height_in(index, ch_id);
	size_t width = getparam_frame_width_in(index, ch_id);

	size_t rslt_frame_num = 0;
	logfile(LOG_DEBUG, "  CH(%u) gennerate send image data : movie file (%s)\n", ch_id, moviefile);
	int32_t ret = movie2image(moviefile, ch_id, height, width, gen_frame_num, img, &rslt_frame_num);
	if (ret < 0) {
		logfile(LOG_ERROR, "  CH(%u) failed to generate send image data from movie file (%s)!\n", ch_id, moviefile);
		rslt2file("CH(%u) failed to generate send image data from movie file (%s)!\n", ch_id, moviefile);
		*rslt = -1;
	} else {
		logfile(LOG_DEBUG, "  CH(%u) generate send image data (%p): generate frame num (%zu)\n", ch_id, img, rslt_frame_num);
		*rslt = 0;
	}

	logfile(LOG_DEBUG, "CH(%u) ...thread_gen_sendimgdata end...\n", ch_id);
}

//----------------------------------
// Generate send image ppm Thread
//----------------------------------
void thread_gen_sendimgppm(thread_genimg_args_t *args)
{
	uint32_t ch_id = args->ch_id;
	pthread_t thread = pthread_self();
	uint32_t core_no = getopt_core() + ch_id;
	if (core_no != 0xff) {
		int32_t cpu_num = get_cpunum();
		logfile(LOG_DEBUG, "CPU num(%u)\n", cpu_num);
		cpu_set_t *cpus = CPU_ALLOC(cpu_num);
		size_t sz = CPU_ALLOC_SIZE(cpu_num);
		CPU_ZERO_S(sz, cpus);
		CPU_SET_S(core_no, sz, cpus);
		int32_t result = pthread_setaffinity_np(thread, sizeof(cpu_set_t), cpus);
		if (result != 0) {
			logfile(LOG_ERROR, " sched_setaffinity error!(%d)\n", result);
		}
		CPU_FREE(cpus);
	}

	logfile(LOG_DEBUG, "CH(%u) ...thread_gen_sendimgppm start...\n", ch_id);

	int32_t *rslt = args->result;
	size_t gen_frame_num = getopt_frame_num();
	if (getopt_tester_meas_mode()) {
		gen_frame_num = 1;
	}

	const char *moviefile = getparam_moviefile(ch_id);

	size_t rslt_frame_num = 0;
	logfile(LOG_DEBUG, "  CH(%u) gennerate send image ppm : movie file (%s)\n", ch_id, moviefile);
	int32_t ret = movie2sendppm(moviefile, ch_id, gen_frame_num, &rslt_frame_num, SEND_DATA_DIR, DUMP_PPM_NUM_MAX);
	if (ret < 0) {
		logfile(LOG_ERROR, "  CH(%u) failed to generate send image ppm from movie file (%s)!\n", ch_id, moviefile);
		rslt2file("CH(%u) failed to generate send image ppm from movie file (%s)!\n", ch_id, moviefile);
		*rslt = -1;
	} else {
		logfile(LOG_DEBUG, "  CH(%u) generate send image ppm : generate frame num (%zu)\n", ch_id, rslt_frame_num);
		rslt2file("dump ppm -> \"%s/ch%02u_task*_send.ppm\"\n", SEND_DATA_DIR, ch_id);
		*rslt = 0;
	}

	logfile(LOG_DEBUG, "CH(%u) ...thread_gen_sendimgppm end...\n", ch_id);
}

//----------------------------------
// Receive Thread (host receive)
//----------------------------------
void thread_receive(thread_receive_args_t *args)
{
	uint32_t ch_id = args->ch_id;
	pthread_t thread = pthread_self();
	uint32_t core_no = getopt_core() + ch_id;
	if (core_no != 0xff) {
		int32_t cpu_num = get_cpunum();
		logfile(LOG_DEBUG, "CPU num(%u)\n", cpu_num);
		cpu_set_t *cpus = CPU_ALLOC(cpu_num);
		size_t sz = CPU_ALLOC_SIZE(cpu_num);
		CPU_ZERO_S(sz, cpus);
		CPU_SET_S(core_no, sz, cpus);
		int32_t result = pthread_setaffinity_np(thread, sizeof(cpu_set_t), cpus);
		if (result != 0) {
			logfile(LOG_ERROR, " sched_setaffinity error!(%d)\n", result);
		}
		CPU_FREE(cpus);
	}

	logfile(LOG_DEBUG, "CH(%u) ...thread_receive start...\n", ch_id);

	const divide_que_t *div_que = get_divide_que();

	uint32_t *dev_id = get_dev_id(fpga_get_num() - 1);
	uint32_t index = dev_id_to_index(*dev_id);
	size_t height = getparam_frame_height_out(index, ch_id);
	size_t width = getparam_frame_width_out(index, ch_id);

	void *rhap = get_receiveheader_addr(ch_id);
	frameheader_t *rfh = (frameheader_t*)rhap;

	uint8_t *rimg = get_receiveimg_addr(ch_id);

	bool *deq_shms = get_deq_shmstate(ch_id);
	uint32_t ring = 0;

	uint32_t run_id = args->run_id;
	uint32_t enq_num = args->enq_num;
	for (size_t i=0; i < enq_num; i++) {
		uint32_t enq_id = i + run_id * div_que->que_num;
		const dmacmd_info_t *pdmacmdinfo = get_deqdmacmdinfo(ch_id, enq_id);
		uint32_t task_id = pdmacmdinfo-> task_id;
		void *data_addr = (void*)pdmacmdinfo->data_addr;

		bool lp = true;
		while (lp) {
			pthread_mutex_lock( &tx_shmmutex[ch_id][ring] );
			// Judges whether shared memory has been dequeued [If true, shared memory can be read]
			bool ds = deq_shms[ring];
			pthread_mutex_unlock( &tx_shmmutex[ch_id][ring] );
			if (ds) {
				if (! getopt_is_performance_meas())
					logfile(LOG_DEBUG, "CH(%u) deq(%zu) task_id(%u) deq_shms[%u]=true receive start\n", ch_id, enq_id, task_id, ring);
				lp = false;
			}
			usleep(SHMEM_POLLING_INTERVAL);
		}

		if (pdmacmdinfo->result_task_id == 0) {
			// In case of dequeue error, the shared memory is not updated, so it is initialized.
			uint32_t data_len = sizeof(frameheader_t) + (height * width * 3);
			init_data((uint8_t*)((uint64_t)data_addr), data_len, 1); //0xff
		}

		//----------------------------------------------
		// receive frameheader
		//----------------------------------------------
		// frameheader
		void *head_addr = data_addr;
		size_t head_len = sizeof(frameheader_t);
		if (! getopt_is_performance_meas())
			logfile(LOG_DEBUG, "  CH(%u) deq(%zu) task_id(%u) receive frameheader from (%p) to (%p)\n", ch_id, enq_id, task_id, head_addr, rfh);
		memcpy(rfh, head_addr, head_len);
		// get timestamp
		timer_header_start(ch_id, enq_id, rfh->local_ts);

		rfh++;

		//----------------------------------------------
		// receive imagedata
		//----------------------------------------------
		// imagedata
		void *img_addr = data_addr + head_len;
		size_t img_len = height * width * 3;

		// imagedata to ppm
		if (getopt_is_outppm_receive_data() && enq_id < DUMP_PPM_NUM_MAX) {
			logfile(LOG_DEBUG, "  CH(%u) deq(%zu) task_id(%u) receive imagedata from (%p) to (%p)\n", ch_id, enq_id, task_id, img_addr, rimg);
			memcpy(rimg, img_addr, img_len);
			rimg += (height * width * 3);
		}

		// deq shmstate update
		pthread_mutex_lock( &tx_shmmutex[ch_id][ring] );
		deq_shms[ring] = false; // set shared memory unused [set to false]
		pthread_mutex_unlock( &tx_shmmutex[ch_id][ring] );

		// deq receivep update
		pthread_mutex_lock( &tx_recmutex[ch_id] );
		int64_t *deq_receivep = get_deq_receivep(ch_id);
		*deq_receivep = (int64_t)enq_id;
		pthread_mutex_unlock( &tx_recmutex[ch_id] );

		ring++;

		if (ring >= getopt_shmalloc_num()) {
			ring = 0;
		}

		if (! getopt_is_performance_meas())
			logfile(LOG_DEBUG, "  CH(%u) deq(%zu) task_id(%u) receive end\n", ch_id, enq_id, task_id);
	}

	logfile(LOG_DEBUG, "CH(%u) ...thread_receive end...\n", ch_id);
}

