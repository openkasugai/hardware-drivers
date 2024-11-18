/*************************************************
* Copyright 2024 NTT Corporation, FUJITSU LIMITED
* Licensed under the 3-Clause BSD License, see LICENSE for details.
* SPDX-License-Identifier: BSD-3-Clause
*************************************************/
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include "xpcie_device.h"
#include "libshmem.h"
#include "libfpgactl.h"
#include "libdma.h"
#include "libdmacommon.h"
#include "liblldma.h"
#include "libchain.h"
#include "common.h"
#include "param_tables.h"
#include "bcdbg.h"
#include "tp.h"

int32_t tp_d2d_h_host_host(void)
{
	logfile(LOG_DEBUG, "--- test tp_d2d_h_host_host start!! ---\n");

	int32_t ret = 0;
	mngque_t pque[CH_NUM_MAX];
	memset(&pque, 0, sizeof(mngque_t) * CH_NUM_MAX);

	int32_t fpga_num = fpga_get_num();
	if (fpga_num != 2) {
		logfile(LOG_ERROR, " Num of FPGA error(%d)\n", fpga_num);
		return -1;
	}

	// check output frame size of device=0 and input frame size of device=1.
	if (tp_check_dev_to_dev_frame_size(*get_dev_id(0), *get_dev_id(1)) < 0) {
		// error
		return -1;
	}

	// deq_shmstate init
	for (size_t i=0; i < CH_NUM_MAX; i++) {
		bool *s = get_deq_shmstate(i);
		for (size_t j=0; j < SHMEMALLOC_NUM_MAX; j++) {
			s[j] = false;
			pthread_mutex_init( &tx_shmmutex[i][j], NULL );
		}
	}
	// deq_receivep init
	for (size_t i=0; i < CH_NUM_MAX; i++) {
		int64_t *rp = get_deq_receivep(i);
		*rp = -1;
		pthread_mutex_init( &tx_recmutex[i], NULL );
	}

	//----------------------------------------------
	// allocate buffer
	//----------------------------------------------
	if (tp_allocate_buffer() < 0) {
		// error
		return -1;
	}

	//----------------------------------------------
	// shared memory allocate
	//----------------------------------------------
	if (tp_shmem_allocate(SHMEM_D2D_SRC_DST, pque) < 0) {
		// error
		goto _END1;
	}

	//----------------------------------------------
	// FPGA kernel init
	//----------------------------------------------
	// for device=0
	ret = tp_function_filter_resize_init(*get_dev_id(0));
	if (ret < 0) {
		// error
		switch (ret) {
			case -2:
				goto _END30;
				break;
			default:
				goto _END2;
		}
	}

	// for device=1
	ret = tp_function_filter_resize_init(*get_dev_id(1));
	if (ret < 0) {
		// error
		switch (ret) {
			case -2:
				goto _END31;
				break;
			default:
				goto _END2;
		}
	}

	//----------------------------------------------
	// D2D fpga buf connect
	//----------------------------------------------
	if (tp_fpga_buf_connect(pque) < 0) {
		// error
		goto _END31;
	}

	//----------------------------------------------
	//----------------------------------------------
	// fpga fdma init
	//----------------------------------------------
	if (tp_enqueue_fdma_init(*get_dev_id(0)) < 0) {
		// error
		goto _END32;
	}

	if (tp_dequeue_fdma_init(*get_dev_id(1)) < 0) {
		// error
		goto _END40;
	}

	//----------------------------------------------
	// fpga fdma queue setup (set dmainfo)
	//----------------------------------------------
	if (tp_enqueue_fdma_queue_setup(*get_dev_id(0)) < 0) {
		// error
		goto _END51;
	}

	if (tp_dequeue_fdma_queue_setup(*get_dev_id(1)) < 0) {
		// error
		goto _END60;
	}

	//----------------------------------------------
	// function chain control
	//----------------------------------------------
	// for device=0
	if (tp_chain_connect(*get_dev_id(0)) < 0) {
		// error
		goto _END71;
	}

	// for device=1
	if (tp_chain_connect(*get_dev_id(1)) < 0) {
		// error
		goto _END80;
	}

	//----------------------------------------------
	// test run loop
	//----------------------------------------------
	const divide_que_t *div_que = get_divide_que();
	prlog_divide_que(div_que);
	bool gen_send_img_en = true; 
	uint32_t run_num = div_que->div_num;

	for (size_t r=0; r < run_num; r++) {
		uint32_t run_id = r;
		uint32_t enq_num = div_que->que_num;
		if (run_id == run_num - 1 && div_que->que_num_rem > 0)
			enq_num = div_que->que_num_rem;

		uint32_t from_task_id = run_id * div_que->que_num + 1;
		uint32_t to_task_id = (enq_num - 1) + run_id * div_que->que_num + 1;
		rslt2file("\n_/_/_/_/_/ TEST No.%u: from TASK(%u) to TASK(%u), enq_num %u _/_/_/_/_/\n", run_id + 1, from_task_id, to_task_id, enq_num);
		logfile(LOG_DEBUG, "_/_/_/_/_/ TEST No.%u: from TASK(%u) to TASK(%u), enq_num %u _/_/_/_/_/\n", run_id + 1, from_task_id, to_task_id, enq_num);
		//----------------------------------------------
		// generate send image data
		//----------------------------------------------
		if (gen_send_img_en) {
			if (tp_generate_send_image_data(run_id) < 0) {
				// error
				goto _END81;
			}

			// debug ppm
			if (getopt_is_outppm_send_data()) {
				if (tp_outppm_send_data(run_id, enq_num) < 0) {
					// error
					goto _END81;
				}
			}

			if (getopt_tester_meas_mode())
				gen_send_img_en = false;
		}

		//----------------------------------------------
		// set dmacmd info
		//----------------------------------------------
		if (tp_enqueue_set_dma_cmd(run_id, enq_num, pque) < 0) {
			// error
			goto _END81;
		}

		if (tp_dequeue_set_dma_cmd(run_id, enq_num, pque) < 0) {
			// error
			goto _END81;
		}

		//----------------------------------------------
		// receive thread start
		//----------------------------------------------
		logfile(LOG_DEBUG, "--- pthread_create thread_receive ---\n");
		rslt2file("\n--- receive thread start ---\n");
		pthread_t thread_receive_id[CH_NUM_MAX];
		thread_receive_args_t threceive_args[CH_NUM_MAX];
		for (size_t i=0; i < CH_NUM_MAX; i++) {
			uint32_t ch_id = i;
			if (getopt_ch_en(ch_id)) {
				threceive_args[ch_id] = (thread_receive_args_t){ch_id, run_id, enq_num};
				ret = pthread_create(&thread_receive_id[ch_id], NULL, (void*)thread_receive, &threceive_args[ch_id]);
				if (ret) {
					logfile(LOG_ERROR, " CH(%zu) create thread_receive error!(%d)\n", ch_id, ret);
					// error
					goto _END81;
				}
				logfile(LOG_DEBUG, "CH(%zu) thread_receive_id(%lx),\n", ch_id, thread_receive_id[ch_id]);
			}
		}

		//----------------------------------------------
		// DMA TX dequeue thread start
		//----------------------------------------------
		logfile(LOG_DEBUG, "--- pthread_create thread_dma_tx_deq ---\n");
		rslt2file("\n--- dma tx dequeue thread start ---\n");
		pthread_t thread_dma_tx_deq_id[CH_NUM_MAX];
		thread_deq_args_t th_dma_tx_deq_args[CH_NUM_MAX];
		for (size_t i=0; i < CH_NUM_MAX; i++) {
			uint32_t ch_id = i;
			if (getopt_ch_en(ch_id)) {
				th_dma_tx_deq_args[ch_id] = (thread_deq_args_t){*get_dev_id(1), ch_id, run_id, enq_num};
			ret = pthread_create(&thread_dma_tx_deq_id[ch_id], NULL, (void*)thread_dma_tx_deq, &th_dma_tx_deq_args[ch_id]);
				if (ret) {
					logfile(LOG_ERROR, " CH(%zu) create thread_dma_tx_deq error!(%d)\n", ch_id, ret);
					// error
					goto _END81;
				}
				logfile(LOG_DEBUG, "CH(%zu) thread_dma_tx_deq_id(%lx),\n", ch_id, thread_dma_tx_deq_id[ch_id]);
			}
		}
		sleep(1);

		//----------------------------------------------
		// DMA TX enqueue thread start
		//----------------------------------------------
		logfile(LOG_DEBUG, "--- pthread_create thread_dma_tx_enq ---\n");
		rslt2file("\n--- dma tx enqueue thread start ---\n");
		pthread_t thread_dma_tx_enq_id[CH_NUM_MAX];
		thread_enq_args_t th_dma_tx_enq_args[CH_NUM_MAX];
		for (size_t i=0; i < CH_NUM_MAX; i++) {
			uint32_t ch_id = i;
			if (getopt_ch_en(ch_id)) {
				th_dma_tx_enq_args[ch_id] = (thread_enq_args_t){*get_dev_id(1), ch_id, run_id, enq_num};
				ret = pthread_create(&thread_dma_tx_enq_id[ch_id], NULL, (void*)thread_dma_tx_enq, &th_dma_tx_enq_args[ch_id]);
				if (ret) {
					logfile(LOG_ERROR, " CH(%zu) create thread_dma_tx_enq error!(%d)\n", ch_id, ret);
					// error
					goto _END81;
				}
				logfile(LOG_DEBUG, "CH(%zu) thread_dma_tx_enq_id(%lx),\n", ch_id, thread_dma_tx_enq_id[ch_id]);
			}
		}

		//----------------------------------------------
		// DMA RX dequeue thread start
		//----------------------------------------------
		logfile(LOG_DEBUG, "--- pthread_create thread_dma_rx_deq ---\n");
		rslt2file("\n--- dma rx dequeue thread start ---\n");
		pthread_t thread_dma_rx_deq_id[CH_NUM_MAX];
		thread_deq_args_t th_dma_rx_deq_args[CH_NUM_MAX];
		for (size_t i=0; i < CH_NUM_MAX; i++) {
			uint32_t ch_id = i;
			if (getopt_ch_en(ch_id)) {
				th_dma_rx_deq_args[ch_id] = (thread_deq_args_t){*get_dev_id(0), ch_id, run_id, enq_num};
				ret = pthread_create(&thread_dma_rx_deq_id[ch_id], NULL, (void*)thread_dma_rx_deq, &th_dma_rx_deq_args[ch_id]);
				if (ret) {
					logfile(LOG_ERROR, " CH(%zu) create thread_dma_rx_deq error!(%d)\n", ch_id, ret);
					// error
					goto _END81;
				}
				logfile(LOG_DEBUG, "CH(%zu) thread_dma_rx_deq_id(%lx),\n", ch_id, thread_dma_rx_deq_id[ch_id]);
			}
		}
		sleep(1);

		//----------------------------------------------
		// send frame start
		//----------------------------------------------
		logfile(LOG_DEBUG, "--- send frame ---\n");
		rslt2file("\n--- send frame ---\n");

		// DMA RX enqueue thread start
		logfile(LOG_DEBUG, "--- pthread_create thread_dma_rx_enq ---\n");
		rslt2file("\n--- dma rx enqueue thread start ---\n");
		pthread_t thread_dma_rx_enq_id[CH_NUM_MAX];
		thread_enq_args_t th_dma_rx_enq_args[CH_NUM_MAX];
		for (size_t i=0; i < CH_NUM_MAX; i++) {
			uint32_t ch_id = i;
			if (getopt_ch_en(ch_id)) {
				th_dma_rx_enq_args[ch_id] = (thread_enq_args_t){*get_dev_id(0), ch_id, run_id, enq_num};
				ret = pthread_create(&thread_dma_rx_enq_id[ch_id], NULL, (void*)thread_dma_rx_enq, &th_dma_rx_enq_args[ch_id]);
				if (ret) {
					logfile(LOG_ERROR, " CH(%zu) create thread_dma_rx_enq error!(%d)\n", ch_id, ret);
					// error
					goto _END81;
				}
				logfile(LOG_DEBUG, "CH(%zu) thread_dma_rx_enq_id(%lx),\n", ch_id, thread_dma_rx_enq_id[ch_id]);
			}
		}

		//----------------------------------------------
		// waitting... all finish
		//----------------------------------------------
		logfile(LOG_DEBUG, " ...waitting for all dequeue process to finish\n");
		rslt2file("\n...waitting for all dequeue process to finish\n");

		// receive thread end
		for (size_t i=0; i < CH_NUM_MAX; i++) {
			uint32_t ch_id = i;
			if (getopt_ch_en(ch_id)) {
				logfile(LOG_DEBUG, "CH(%zu) pthread_join(thread_receive: %lx)\n", ch_id, thread_receive_id[ch_id]);
				ret = pthread_join(thread_receive_id[ch_id], NULL);
				if (ret) {
					logfile(LOG_ERROR, " CH(%zu) pthread_join thread_receive error!(%d)\n", ch_id, ret);
				}
			}
		}

		// DMA TX dequeue thread end
		for (size_t i=0; i < CH_NUM_MAX; i++) {
			uint32_t ch_id = i;
			if (getopt_ch_en(ch_id)) {
				logfile(LOG_DEBUG, "CH(%zu) pthread_join(thread_dma_tx_deq: %lx)\n", ch_id, thread_dma_tx_deq_id[ch_id]);
				ret = pthread_join(thread_dma_tx_deq_id[ch_id], NULL);
				if (ret) {
					logfile(LOG_ERROR, " CH(%zu) pthread_join thread_dma_tx_deq error!(%d)\n", ch_id, ret);
				}
			}
		}

		// DMA TX enqueue thread end
		for (size_t i=0; i < CH_NUM_MAX; i++) {
			uint32_t ch_id = i;
			if (getopt_ch_en(ch_id)) {
				logfile(LOG_DEBUG, "CH(%zu) pthread_join(thread_dma_tx_enq: %lx)\n", ch_id, thread_dma_tx_enq_id[ch_id]);
				ret = pthread_join(thread_dma_tx_enq_id[ch_id], NULL);
				if (ret) {
					logfile(LOG_ERROR, " CH(%zu) pthread_join thread_dma_tx_enq error!(%d)\n", ch_id, ret);
				}
			}
		}

		// DMA RX dequeue thread end
		for (size_t i=0; i < CH_NUM_MAX; i++) {
			uint32_t ch_id = i;
			if (getopt_ch_en(ch_id)) {
				logfile(LOG_DEBUG, "CH(%zu) pthread_join(thread_dma_rx_deq: %lx)\n", ch_id, thread_dma_rx_deq_id[ch_id]);
				ret = pthread_join(thread_dma_rx_deq_id[ch_id], NULL);
				if (ret) {
					logfile(LOG_ERROR, " CH(%zu) pthread_join thread_dma_rx_deq error!(%d)\n", ch_id, ret);
				}
			}
		}

		// DMA RX enqueue thread end
		for (size_t i=0; i < CH_NUM_MAX; i++) {
			uint32_t ch_id = i;
			if (getopt_ch_en(ch_id)) {
				logfile(LOG_DEBUG, "CH(%zu) pthread_join(thread_dma_rx_enq: %lx)\n", ch_id, thread_dma_rx_enq_id[ch_id]);
				ret = pthread_join(thread_dma_rx_enq_id[ch_id], NULL);
				if (ret) {
					logfile(LOG_ERROR, " CH(%zu) pthread_join thread_dma_rx_enq error!(%d)\n", ch_id, ret);
				}
			}
		}
	}

	//----------------------------------------------
	// performance result
	//----------------------------------------------
	pr_perf_normal();
	if (getopt_is_performance_meas()) {
		pr_perf();
	}

	//----------------------------------------------
	// end processing
	//----------------------------------------------
_END81:
	// chain disconnect for device=1
	tp_chain_disconnect(*get_dev_id(1));

_END80:
	// chain disconnect for device=0
	tp_chain_disconnect(*get_dev_id(0));

_END71:
	// fdma queue finish
	tp_dequeue_fdma_queue_finish(*get_dev_id(1));

_END60:
	// fdma queue finish
	tp_enqueue_fdma_queue_finish(*get_dev_id(0));

_END51:
	// fdma finish
	tp_dequeue_fdma_finish(*get_dev_id(1));

_END50:
	// fdma finish
	tp_dequeue_fdma_finish(*get_dev_id(0));

_END41:
	// fdma finish
	tp_enqueue_fdma_finish(*get_dev_id(1));

_END40:
	// fdma finish
	tp_enqueue_fdma_finish(*get_dev_id(0));

_END32:
	// fpga buf disconnect
	tp_fpga_buf_disconnect();

_END31:
	// function finish
	tp_function_finish(*get_dev_id(1));

_END30:
	// function finish
	tp_function_finish(*get_dev_id(0));

_END2:
	// shared memory free
	tp_shmem_free(pque);

_END1:
	// free buffer
	tp_free_buffer();


	logfile(LOG_DEBUG, "...test tp_d2d_h_host_host end\n");

	return 0;
}
