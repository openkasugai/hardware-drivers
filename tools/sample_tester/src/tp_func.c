/*************************************************
* Copyright 2024 NTT Corporation, FUJITSU LIMITED
* Licensed under the 3-Clause BSD License, see LICENSE for details.
* SPDX-License-Identifier: BSD-3-Clause
*************************************************/
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <inttypes.h>
#include <unistd.h>
#include <pthread.h>
#include "libshmem.h"
#include "libfpgactl.h"
#include "libdma.h"
#include "libdmacommon.h"
#include "liblldma.h"
#include "libchain.h"
#include "libchain_err.h"
#include "libdirecttrans_err.h"
#include "libfunction_filter_resize_err.h"
#include "libfunction_conv_err.h"
#include "libfunction.h"
#include "common.h"
#include "param_tables.h"
#include "bcdbg.h"
#include "tp.h"

#define JSON_FORMAT "{ \
  \"i_width\"   :%u, \
  \"i_height\"  :%u, \
  \"o_width\"   :%u, \
  \"o_height\"  :%u \
}"

const char *tp_model_name[] = {
	"TP_HOST_HOST",
	"TP_D2D_H_HOST_HOST",
	"TP_D2D_D_HOST_HOST",
	"TP_UNKNOWN"
};

int32_t tp_shmem_allocate(shmem_mode_t shmem_mode, mngque_t *pque)
{
	int32_t ret = 0;

	logfile(LOG_DEBUG, "--- shmem_malloc ---\n");
	for (size_t i=0; i < CH_NUM_MAX; i++) {
		uint32_t ch_id = i;
		if (getopt_ch_en(ch_id)) {
			ret = shmem_malloc(shmem_mode, &pque[ch_id], ch_id);
			if (ret < 0) {
				logfile(LOG_ERROR, "shmem_malloc error(%d)\n",ret);
				return -1;
			}
			prlog_mngque(&pque[ch_id], ch_id);
		}
	}
	return 0;
}

int32_t tp_shmem_free(mngque_t *pque)
{
	logfile(LOG_DEBUG, "--- shmem_free ---\n");
	for (size_t i=0; i < CH_NUM_MAX; i++) {
		uint32_t ch_id = i;
		if (getopt_ch_en(ch_id)) {
			shmem_free(&pque[ch_id], ch_id);
		}
	}
	return 0;
}

int32_t tp_allocate_buffer(void)
{
	int32_t ret = 0;

	// allocate dmacmdinfo buffer
	logfile(LOG_DEBUG, "--- dmacmdinfo_malloc ---\n");
	ret = dmacmdinfo_malloc();
	if (ret < 0) {
		logfile(LOG_ERROR, "dmacmdinfo_alloc error!!!(%d)\n",ret);
		return -1;
	}

	if (getopt_is_receive_data()) {
		// allocate receiveheader buffer
		logfile(LOG_DEBUG, "--- receiveheader_malloc ---\n");
		for (size_t i=0; i < CH_NUM_MAX; i++) {
			uint32_t ch_id = i;
			if (getopt_ch_en(ch_id)) {
				ret = receiveheader_malloc(ch_id);
				if (ret < 0) {
					logfile(LOG_ERROR, "receiveheader_malloc error!!!(%d)\n",ret);
					return -1;
				}
			}
		}
	}

	if (getopt_is_send_data()) {
		// allocate sendimg buffer
		logfile(LOG_DEBUG, "--- sendimg_malloc ---\n");
		for (size_t i=0; i < CH_NUM_MAX; i++) {
			uint32_t ch_id = i;
			if (getopt_ch_en(ch_id)) {
				ret = sendimg_malloc(ch_id);
				if (ret < 0) {
					logfile(LOG_ERROR, "sendimg_malloc error!!!(%d)\n",ret);
					return -1;
				}
			}
		}
	}

	if (getopt_is_outppm_receive_data()) {
		// allocate receiveimg buffer
		logfile(LOG_DEBUG, "--- receiveimg_malloc ---\n");
		for (size_t i=0; i < CH_NUM_MAX; i++) {
			uint32_t ch_id = i;
			if (getopt_ch_en(ch_id)) {
				ret = receiveimg_malloc(ch_id);
				if (ret < 0) {
					logfile(LOG_ERROR, "receiveimg_malloc error!!!(%d)\n",ret);
					return -1;
				}
			}
		}
	}	

	// allocate timestamp buffer
	logfile(LOG_DEBUG, "--- timestamp_malloc ---\n");
	ret = timestamp_malloc();
	if (ret < 0) {
		logfile(LOG_ERROR, "timestamp_malloc error!!!(%d)\n",ret);
		return -1;
	}

	return 0;
}

void tp_free_buffer(void)
{
	if (getopt_is_receive_data()) {
		// free receiveheader buffer
		logfile(LOG_DEBUG, "--- receiveheader_free ---\n");
		for (size_t i=0; i < CH_NUM_MAX; i++) {
			uint32_t ch_id = i;
			if (getopt_ch_en(ch_id)) {
				receiveheader_free(ch_id);
			}
		}
	}

	if (getopt_is_send_data()) {
		// free sendimg buffer
		logfile(LOG_DEBUG, "--- sendimg_free ---\n");
		for (size_t i=0; i < CH_NUM_MAX; i++) {
			uint32_t ch_id = i;
			if (getopt_ch_en(ch_id)) {
				sendimg_free(ch_id);
			}
		}
	}

	if (getopt_is_outppm_receive_data()) {
		// free receiveimg buffer
		logfile(LOG_DEBUG, "--- receiveimg_free ---\n");
		for (size_t i=0; i < CH_NUM_MAX; i++) {
			uint32_t ch_id = i;
			if (getopt_ch_en(ch_id)) {
				receiveimg_free(ch_id);
			}
		}
	}

	// free dmacmdinfo buffer
	logfile(LOG_DEBUG, "--- dmacmdinfo_free ---\n");
	dmacmdinfo_free();

	// free timestamp buffer
	logfile(LOG_DEBUG, "--- timestamp_free ---\n");
	timestamp_free();
}

int32_t tp_open_moviefile(void)
{
	int32_t ret = 0;
	bool err = false;

	logfile(LOG_DEBUG, "--- open_moviefile ---\n");
	for (size_t i=0; i < CH_NUM_MAX; i++) {
		uint32_t ch_id = i;
		if (getopt_ch_en(ch_id)) {
			ret = open_moviefile(ch_id);
			if (ret < 0) {
				logfile(LOG_ERROR, "open_moviefile error!!!(%d)\n", ret);
				// error
				err = true;
			}
		}
	}
	if (err) return -1;

	return 0;
}

int32_t tp_generate_send_image_data(uint32_t run_id)
{
	int32_t ret = 0;
	uint32_t total_ch_num = 0;
	for (size_t i=0; i < LANE_NUM_MAX; i++) {
		total_ch_num += getopt_ch_num(i);
	}

	rslt2file("\n--- generate send image data ---\n");

	// generate send image data
	logfile(LOG_DEBUG, "--- pthread_create thread_gen_sendimgdata ---\n");
	pthread_t thread_gensendimg_id[CH_NUM_MAX];
	thread_genimg_args_t th_gensendimg_args[CH_NUM_MAX];
	int32_t rslt[CH_NUM_MAX];
	memset(&rslt, 0, sizeof(int32_t) * CH_NUM_MAX);

	uint32_t tmp_ch_id=0;
	size_t total_ch_cnt = 0;
	while (total_ch_cnt < total_ch_num) {
		size_t ch_cnt = 0;
		for (size_t i=tmp_ch_id; i < CH_NUM_MAX; i++) {
			uint32_t ch_id = i;
			if (getopt_ch_en(ch_id)) {
				if (ch_cnt < GEN_IMG_PARALLEL_NUM) {
					rslt2file("CH(%zu) generating send image data...\n", ch_id);
					th_gensendimg_args[ch_id] = (thread_genimg_args_t){ch_id, run_id, &rslt[ch_id]};
					ret = pthread_create(&thread_gensendimg_id[ch_id], NULL, (void*)thread_gen_sendimgdata, &th_gensendimg_args[ch_id]);
					if (ret) {
						logfile(LOG_ERROR, " CH(%zu) create thread_gen_sendimgdata error!(%d)\n", ch_id, ret);
					}
					logfile(LOG_DEBUG, "CH(%zu) thread_gen_sendimgdata(%lx),\n", ch_id, thread_gensendimg_id[ch_id]);
					usleep(300*1000);
					ch_cnt++;
				} else {
					break;
				}
			}
		}

		ch_cnt = 0;
		for (size_t i=tmp_ch_id; i < CH_NUM_MAX; i++) {
			uint32_t ch_id = i;
			if (getopt_ch_en(ch_id)) {
				if (ch_cnt < GEN_IMG_PARALLEL_NUM) {
					logfile(LOG_DEBUG, "CH(%zu) pthread_join(thread_gen_sendimgdata: %lx)\n", ch_id, thread_gensendimg_id[ch_id]);
					ret = pthread_join(thread_gensendimg_id[ch_id], NULL);
					if (ret) {
						logfile(LOG_ERROR, " CH(%zu) pthread_join thread_gen_sendimgdata error!(%d)\n", ch_id, ret);
					}
					ch_cnt++;
				} else {
					tmp_ch_id = ch_id;
					break;
				}
			}
		}
		total_ch_cnt += ch_cnt;
	}

	bool is_gen_err = false;
	for (size_t i=0; i < CH_NUM_MAX; i++) {
		uint32_t ch_id = i;
		if (rslt[ch_id] < 0) {
			logfile(LOG_ERROR, "CH(%d) thread_gen_sendimgdata error(%d)\n", ch_id, rslt[ch_id]);
			is_gen_err = true;
		}
	}
	if (is_gen_err) {
		return -1;
	}

	return 0;
}

int32_t tp_generate_send_image_ppm(uint32_t run_id)
{
	int32_t ret = 0;
	uint32_t total_ch_num = 0;
	for (size_t i=0; i < LANE_NUM_MAX; i++) {
		total_ch_num += getopt_ch_num(i);
	}

	rslt2file("\n--- generate send image ppm ---\n");

	// generate send image ppm
	logfile(LOG_DEBUG, "--- pthread_create thread_gen_sendimgppm ---\n");
	pthread_t thread_gensendimg_id[CH_NUM_MAX];
	thread_genimg_args_t th_gensendimg_args[CH_NUM_MAX];
	int32_t rslt[CH_NUM_MAX];
	memset(&rslt, 0, sizeof(int32_t) * CH_NUM_MAX);

	uint32_t tmp_ch_id=0;
	size_t total_ch_cnt = 0;
	while (total_ch_cnt < total_ch_num) {
		size_t ch_cnt = 0;
		for (size_t i=tmp_ch_id; i < CH_NUM_MAX; i++) {
			uint32_t ch_id = i;
			if (getopt_ch_en(ch_id)) {
				if (ch_cnt < GEN_IMG_PARALLEL_NUM) {
					rslt2file("CH(%zu) generating send image ppm...\n", ch_id);
					th_gensendimg_args[ch_id] = (thread_genimg_args_t){ch_id, run_id, &rslt[ch_id]};
					ret = pthread_create(&thread_gensendimg_id[ch_id], NULL, (void*)thread_gen_sendimgppm, &th_gensendimg_args[ch_id]);
					if (ret) {
						logfile(LOG_ERROR, " CH(%zu) create thread_gen_sendimgppm error!(%d)\n", ch_id, ret);
					}
					logfile(LOG_DEBUG, "CH(%zu) thread_gen_sendimgppm(%lx),\n", ch_id, thread_gensendimg_id[ch_id]);
					usleep(300*1000);
					ch_cnt++;
				} else {
					break;
				}
			}
		}

		ch_cnt = 0;
		for (size_t i=tmp_ch_id; i < CH_NUM_MAX; i++) {
			uint32_t ch_id = i;
			if (getopt_ch_en(ch_id)) {
				if (ch_cnt < GEN_IMG_PARALLEL_NUM) {
					logfile(LOG_DEBUG, "CH(%zu) pthread_join(thread_gen_sendimgppm: %lx)\n", ch_id, thread_gensendimg_id[ch_id]);
					ret = pthread_join(thread_gensendimg_id[ch_id], NULL);
					if (ret) {
						logfile(LOG_ERROR, " CH(%zu) pthread_join thread_gen_sendimgppm error!(%d)\n", ch_id, ret);
					}
					ch_cnt++;
				} else {
					tmp_ch_id = ch_id;
					break;
				}
			}
		}
		total_ch_cnt += ch_cnt;
	}

	bool is_gen_err = false;
	for (size_t i=0; i < CH_NUM_MAX; i++) {
		uint32_t ch_id = i;
		if (rslt[ch_id] < 0) {
			logfile(LOG_ERROR, "CH(%d) thread_gen_sendimgppm error(%d)\n", ch_id, rslt[ch_id]);
			is_gen_err = true;
		}
	}
	if (is_gen_err) {
		return -1;
	}

	return 0;
}

int32_t tp_set_frame_shmem_src(void)
{
	int32_t ret = 0;
	uint32_t enq_num = getopt_enq_num();

	logfile(LOG_DEBUG, "--- set_frame_shmem_src ---\n");
	for (size_t i=0; i < CH_NUM_MAX; i++) {
		uint32_t ch_id = i;
		if (getopt_ch_en(ch_id)) {
			for (size_t j=0; j < enq_num; j++) {
				uint32_t enq_id = j;
				ret = set_frame_shmem_src(ch_id, enq_id);
				if (ret < 0) {
					logfile(LOG_ERROR, "CH(%u) enq(%u) set_frame_shmem_src error(%d)\n", ch_id, enq_id, ret);
					return -1;
				}
			}
		}
	}

	return 0;
}

int32_t tp_check_dev_to_dev_frame_size(uint32_t tx_dev_id, uint32_t rx_dev_id)
{
	int32_t ret = 0;
	uint32_t tx_index = dev_id_to_index(tx_dev_id);
	uint32_t rx_index = dev_id_to_index(rx_dev_id);
	bool err = false;

	logfile(LOG_DEBUG, "--- check_dev_to_dev_frame_size ---\n");
	for (size_t i=0; i < CH_NUM_MAX; i++) {
		uint32_t ch_id = i;
		if (getopt_ch_en(ch_id)) {
			uint32_t tx_height = getparam_frame_height_out(tx_index, ch_id);
			uint32_t tx_width = getparam_frame_width_out(tx_index, ch_id);
			uint32_t rx_height = getparam_frame_height_in(rx_index, ch_id);
			uint32_t rx_width = getparam_frame_width_in(rx_index, ch_id);
			if (tx_height != rx_height) {
				rslt2file("frame_size error: a mismatch between the OUTPUT HEIGHT(%u) of dev_id(%u) and INTPUT HEIGHT(%u) of dev_id(%u).\n", tx_height, tx_dev_id, rx_height, rx_dev_id);
				logfile(LOG_ERROR, "frame size error! a mismatch between the OUTPUT HEIGHT(%u) of dev_id(%u) and INTPUT HEIGHT(%u) of dev_id(%u).\n", tx_height, tx_dev_id, rx_height, rx_dev_id);
				// error
				err = true;
			}
			if (tx_width != rx_width) {
				rslt2file("frame_size error: a mismatch between the OUTPUT WIDTH(%u) of dev_id(%u) and INTPUT WIDTH(%u) of dev_id(%u).\n", tx_width, tx_dev_id, rx_width, rx_dev_id);
				logfile(LOG_ERROR, "frame size error! a mismatch between the OUTPUT WIDTH(%u) of dev_id(%u) and INTPUT WIDTH(%u) of dev_id(%u).\n", tx_width, tx_dev_id, rx_width, rx_dev_id);
				// error
				err = true;
			}
		}
	}
	if (err) return -1;

	return 0;
}

int32_t tp_protocol_mask_set(uint32_t dev_id)
{
	int32_t ret = 0;

	rslt2file("\n--- set protocol error mask ---\n");

	rslt2file("--- chain ---\n");
	for (size_t i=0; i < CHAIN_KRNL_NUM_MAX; i++) {
		uint32_t lane = i;

		for (size_t j=0; j < DIR_MAX; j++) {
			uint32_t direction = j;
			fpga_chain_err_prot_t wvalue_fpga_chain_set_err_prot_mask = { 0 };
			fpga_chain_err_prot_t rvalue_fpga_chain_set_err_prot_mask;
			wvalue_fpga_chain_set_err_prot_mask.prot_datanum = 1;
			wvalue_fpga_chain_set_err_prot_mask.prot_resp_outstanding = 1;

			ret = fpga_chain_set_err_prot_mask(dev_id, lane, direction, wvalue_fpga_chain_set_err_prot_mask);
			if (ret < 0) {
					logfile(LOG_ERROR, "dev(%u) kernel(%u) dir(%u) fpga_chain_set_err_prot_mask :ret(%d) error!!\n", dev_id, lane, direction, ret);
			} else {
				ret = fpga_chain_get_err_prot_mask(dev_id, lane, direction, &rvalue_fpga_chain_set_err_prot_mask);
				if (ret < 0) {
					logfile(LOG_ERROR, "dev(%u) kernel(%u) dir(%u) fpga_chain_get_err_prot_mask :ret(%d) error!!\n", dev_id, lane, direction, ret);
				} else {
					logfile(LOG_DEBUG, "dev(%u) kernel(%u) dir(%u) fpga_chain_get_err_prot_mask prot_ch               (%u)\n", dev_id, lane, direction, rvalue_fpga_chain_set_err_prot_mask.prot_ch               );
					logfile(LOG_DEBUG, "dev(%u) kernel(%u) dir(%u) fpga_chain_get_err_prot_mask prot_len              (%u)\n", dev_id, lane, direction, rvalue_fpga_chain_set_err_prot_mask.prot_len              );
					logfile(LOG_DEBUG, "dev(%u) kernel(%u) dir(%u) fpga_chain_get_err_prot_mask prot_sof              (%u)\n", dev_id, lane, direction, rvalue_fpga_chain_set_err_prot_mask.prot_sof              );
					logfile(LOG_DEBUG, "dev(%u) kernel(%u) dir(%u) fpga_chain_get_err_prot_mask prot_eof              (%u)\n", dev_id, lane, direction, rvalue_fpga_chain_set_err_prot_mask.prot_eof              );
					logfile(LOG_DEBUG, "dev(%u) kernel(%u) dir(%u) fpga_chain_get_err_prot_mask prot_reqresp          (%u)\n", dev_id, lane, direction, rvalue_fpga_chain_set_err_prot_mask.prot_reqresp          );
					logfile(LOG_DEBUG, "dev(%u) kernel(%u) dir(%u) fpga_chain_get_err_prot_mask prot_datanum          (%u)\n", dev_id, lane, direction, rvalue_fpga_chain_set_err_prot_mask.prot_datanum          );
					logfile(LOG_DEBUG, "dev(%u) kernel(%u) dir(%u) fpga_chain_get_err_prot_mask prot_req_outstanding  (%u)\n", dev_id, lane, direction, rvalue_fpga_chain_set_err_prot_mask.prot_req_outstanding  );
					logfile(LOG_DEBUG, "dev(%u) kernel(%u) dir(%u) fpga_chain_get_err_prot_mask prot_resp_outstanding (%u)\n", dev_id, lane, direction, rvalue_fpga_chain_set_err_prot_mask.prot_resp_outstanding );
					logfile(LOG_DEBUG, "dev(%u) kernel(%u) dir(%u) fpga_chain_get_err_prot_mask prot_max_datanum      (%u)\n", dev_id, lane, direction, rvalue_fpga_chain_set_err_prot_mask.prot_max_datanum      );
					logfile(LOG_DEBUG, "dev(%u) kernel(%u) dir(%u) fpga_chain_get_err_prot_mask prot_reqlen           (%u)\n", dev_id, lane, direction, rvalue_fpga_chain_set_err_prot_mask.prot_reqlen           );
					logfile(LOG_DEBUG, "dev(%u) kernel(%u) dir(%u) fpga_chain_get_err_prot_mask prot_reqresplen       (%u)\n", dev_id, lane, direction, rvalue_fpga_chain_set_err_prot_mask.prot_reqresplen       );
				}
			}
		}
	}
	rslt2file("--- direct ---\n");
	for (size_t i=0; i < CHAIN_KRNL_NUM_MAX; i++) {
		uint32_t lane = i;
		for (size_t j=0; j < DIR_TYPE_MAX; j++) {
			fpga_direct_err_prot_t wvalue_fpga_direct_set_err_prot_mask = { 0 };
			fpga_direct_err_prot_t rvalue_fpga_direct_set_err_prot_mask;
			wvalue_fpga_direct_set_err_prot_mask.prot_datanum = 1;
			wvalue_fpga_direct_set_err_prot_mask.prot_resp_outstanding = 1;
			uint8_t dir_type = j;
			ret = fpga_direct_set_err_prot_mask(dev_id, lane, dir_type, wvalue_fpga_direct_set_err_prot_mask);
			if (ret < 0) {
					logfile(LOG_ERROR, "dev(%u) kernel(%u) dir(%u) fpga_direct_set_err_prot_mask :ret(%d) error!!\n", dev_id, lane, dir_type, ret);
			} else {
				ret = fpga_direct_get_err_prot_mask(dev_id, lane, dir_type, &rvalue_fpga_direct_set_err_prot_mask);
				if (ret < 0) {
					logfile(LOG_ERROR, "dev(%u) kernel(%u) dir(%u) fpga_direct_get_err_prot_mask :ret(%d) error!!\n", dev_id, lane, dir_type, ret);
				} else {
					logfile(LOG_DEBUG, "dev(%u) kernel(%u) dir_type(%u) fpga_direct_get_err_prot_mask prot_ch               (%u)\n", dev_id, lane, dir_type, rvalue_fpga_direct_set_err_prot_mask.prot_ch               );
					logfile(LOG_DEBUG, "dev(%u) kernel(%u) dir_type(%u) fpga_direct_get_err_prot_mask prot_len              (%u)\n", dev_id, lane, dir_type, rvalue_fpga_direct_set_err_prot_mask.prot_len              );
					logfile(LOG_DEBUG, "dev(%u) kernel(%u) dir_type(%u) fpga_direct_get_err_prot_mask prot_sof              (%u)\n", dev_id, lane, dir_type, rvalue_fpga_direct_set_err_prot_mask.prot_sof              );
					logfile(LOG_DEBUG, "dev(%u) kernel(%u) dir_type(%u) fpga_direct_get_err_prot_mask prot_eof              (%u)\n", dev_id, lane, dir_type, rvalue_fpga_direct_set_err_prot_mask.prot_eof              );
					logfile(LOG_DEBUG, "dev(%u) kernel(%u) dir_type(%u) fpga_direct_get_err_prot_mask prot_reqresp          (%u)\n", dev_id, lane, dir_type, rvalue_fpga_direct_set_err_prot_mask.prot_reqresp          );
					logfile(LOG_DEBUG, "dev(%u) kernel(%u) dir_type(%u) fpga_direct_get_err_prot_mask prot_datanum          (%u)\n", dev_id, lane, dir_type, rvalue_fpga_direct_set_err_prot_mask.prot_datanum          );
					logfile(LOG_DEBUG, "dev(%u) kernel(%u) dir_type(%u) fpga_direct_get_err_prot_mask prot_req_outstanding  (%u)\n", dev_id, lane, dir_type, rvalue_fpga_direct_set_err_prot_mask.prot_req_outstanding  );
					logfile(LOG_DEBUG, "dev(%u) kernel(%u) dir_type(%u) fpga_direct_get_err_prot_mask prot_resp_outstanding (%u)\n", dev_id, lane, dir_type, rvalue_fpga_direct_set_err_prot_mask.prot_resp_outstanding );
					logfile(LOG_DEBUG, "dev(%u) kernel(%u) dir_type(%u) fpga_direct_get_err_prot_mask prot_max_datanum      (%u)\n", dev_id, lane, dir_type, rvalue_fpga_direct_set_err_prot_mask.prot_max_datanum      );
					logfile(LOG_DEBUG, "dev(%u) kernel(%u) dir_type(%u) fpga_direct_get_err_prot_mask prot_reqlen           (%u)\n", dev_id, lane, dir_type, rvalue_fpga_direct_set_err_prot_mask.prot_reqlen           );
					logfile(LOG_DEBUG, "dev(%u) kernel(%u) dir_type(%u) fpga_direct_get_err_prot_mask prot_reqresplen       (%u)\n", dev_id, lane, dir_type, rvalue_fpga_direct_set_err_prot_mask.prot_reqresplen       );
				}
			}
		}
	}

	rslt2file("--- conv ---\n");
	for (size_t i=0; i < CONV_KRNL_NUM_MAX; i++) {
		uint32_t lane = i;

		// ingr_rcv, egr_snd
		for (size_t j=0; j < DIR_MAX; j++) {
			uint32_t direction = j;

			fpga_func_err_prot_t wvalue_fpga_func_set_err_prot_mask = { 0 };
			fpga_func_err_prot_t rvalue_fpga_func_set_err_prot_mask;
			wvalue_fpga_func_set_err_prot_mask.prot_datanum = 1;
			wvalue_fpga_func_set_err_prot_mask.prot_resp_outstanding = 1;
			ret = fpga_conv_set_err_prot_mask(dev_id, lane, direction, wvalue_fpga_func_set_err_prot_mask);
			if (ret < 0) {
					logfile(LOG_ERROR, "dev(%u) kernel(%u) dir(%u) fpga_conv_set_err_prot_mask :ret(%d) error!!\n", dev_id, lane, direction, ret);
			} else {
				ret = fpga_conv_get_err_prot_mask(dev_id, lane, direction, &rvalue_fpga_func_set_err_prot_mask);
				if (ret < 0) {
					logfile(LOG_ERROR, "dev(%u) kernel(%u) dir(%u) fpga_conv_get_err_prot_mask :ret(%d) error!!\n", dev_id, lane, direction, ret);
				} else {
					logfile(LOG_DEBUG, "dev(%u) kernel(%u) dir(%u) fpga_conv_get_err_prot_mask prot_ch               (%u)\n", dev_id, lane, direction, rvalue_fpga_func_set_err_prot_mask.prot_ch               );
					logfile(LOG_DEBUG, "dev(%u) kernel(%u) dir(%u) fpga_conv_get_err_prot_mask prot_len              (%u)\n", dev_id, lane, direction, rvalue_fpga_func_set_err_prot_mask.prot_len              );
					logfile(LOG_DEBUG, "dev(%u) kernel(%u) dir(%u) fpga_conv_get_err_prot_mask prot_sof              (%u)\n", dev_id, lane, direction, rvalue_fpga_func_set_err_prot_mask.prot_sof              );
					logfile(LOG_DEBUG, "dev(%u) kernel(%u) dir(%u) fpga_conv_get_err_prot_mask prot_eof              (%u)\n", dev_id, lane, direction, rvalue_fpga_func_set_err_prot_mask.prot_eof              );
					logfile(LOG_DEBUG, "dev(%u) kernel(%u) dir(%u) fpga_conv_get_err_prot_mask prot_reqresp          (%u)\n", dev_id, lane, direction, rvalue_fpga_func_set_err_prot_mask.prot_reqresp          );
					logfile(LOG_DEBUG, "dev(%u) kernel(%u) dir(%u) fpga_conv_get_err_prot_mask prot_datanum          (%u)\n", dev_id, lane, direction, rvalue_fpga_func_set_err_prot_mask.prot_datanum          );
					logfile(LOG_DEBUG, "dev(%u) kernel(%u) dir(%u) fpga_conv_get_err_prot_mask prot_req_outstanding  (%u)\n", dev_id, lane, direction, rvalue_fpga_func_set_err_prot_mask.prot_req_outstanding  );
					logfile(LOG_DEBUG, "dev(%u) kernel(%u) dir(%u) fpga_conv_get_err_prot_mask prot_resp_outstanding (%u)\n", dev_id, lane, direction, rvalue_fpga_func_set_err_prot_mask.prot_resp_outstanding );
					logfile(LOG_DEBUG, "dev(%u) kernel(%u) dir(%u) fpga_conv_get_err_prot_mask prot_max_datanum      (%u)\n", dev_id, lane, direction, rvalue_fpga_func_set_err_prot_mask.prot_max_datanum      );
					logfile(LOG_DEBUG, "dev(%u) kernel(%u) dir(%u) fpga_conv_get_err_prot_mask prot_reqlen           (%u)\n", dev_id, lane, direction, rvalue_fpga_func_set_err_prot_mask.prot_reqlen           );
					logfile(LOG_DEBUG, "dev(%u) kernel(%u) dir(%u) fpga_conv_get_err_prot_mask prot_reqresplen       (%u)\n", dev_id, lane, direction, rvalue_fpga_func_set_err_prot_mask.prot_reqresplen       );
				}
			}
		}

		// ingr_snd, egr_rcv
		for (size_t j=0; j < DIR_MAX; j++) {
			uint32_t direction = j;
			for (size_t k=0; k < FR_NUM_MAX; k++) {
				uint32_t fr_id = k;
				fpga_func_err_prot_t wvalue_fpga_func_set_err_prot_mask = { 0 };
				fpga_func_err_prot_t rvalue_fpga_func_set_err_prot_mask;
				if (direction == INGRESS) {
					wvalue_fpga_func_set_err_prot_mask.prot_ch = 1;
					wvalue_fpga_func_set_err_prot_mask.prot_len = 1;
					wvalue_fpga_func_set_err_prot_mask.prot_sof = 1;
					wvalue_fpga_func_set_err_prot_mask.prot_eof = 1;
					wvalue_fpga_func_set_err_prot_mask.prot_reqresp = 1;
					wvalue_fpga_func_set_err_prot_mask.prot_datanum = 1;
					wvalue_fpga_func_set_err_prot_mask.prot_req_outstanding = 1;
					wvalue_fpga_func_set_err_prot_mask.prot_resp_outstanding = 1;
					wvalue_fpga_func_set_err_prot_mask.prot_max_datanum = 1;
					wvalue_fpga_func_set_err_prot_mask.prot_reqlen = 0;
					wvalue_fpga_func_set_err_prot_mask.prot_reqresplen = 1;
				} else {
					wvalue_fpga_func_set_err_prot_mask.prot_datanum = 1;
					wvalue_fpga_func_set_err_prot_mask.prot_resp_outstanding = 1;
				}
				ret = fpga_conv_set_err_prot_func_mask(dev_id, lane, fr_id, direction, wvalue_fpga_func_set_err_prot_mask);
				if (ret < 0) {
						logfile(LOG_ERROR, "dev(%u) kernel(%u) fr_id(%u) dir(%u) fpga_conv_set_err_prot_func_mask :ret(%d) error!!\n", dev_id, lane, fr_id, direction, ret);
				} else {
					ret = fpga_conv_get_err_prot_func_mask(dev_id, lane, fr_id, direction, &rvalue_fpga_func_set_err_prot_mask);
					if (ret < 0) {
						logfile(LOG_ERROR, "dev(%u) kernel(%u) fr_id(%u) dir(%u) fpga_conv_get_err_prot_func_mask :ret(%d) error!!\n", dev_id, lane, fr_id, direction, ret);
					} else {
						logfile(LOG_DEBUG, "dev(%u) kernel(%u) fr_id(%u) dir(%u) fpga_conv_get_err_prot_func_mask prot_ch               (%u)\n", dev_id, lane, fr_id, direction, rvalue_fpga_func_set_err_prot_mask.prot_ch               );
						logfile(LOG_DEBUG, "dev(%u) kernel(%u) fr_id(%u) dir(%u) fpga_conv_get_err_prot_func_mask prot_len              (%u)\n", dev_id, lane, fr_id, direction, rvalue_fpga_func_set_err_prot_mask.prot_len              );
						logfile(LOG_DEBUG, "dev(%u) kernel(%u) fr_id(%u) dir(%u) fpga_conv_get_err_prot_func_mask prot_sof              (%u)\n", dev_id, lane, fr_id, direction, rvalue_fpga_func_set_err_prot_mask.prot_sof              );
						logfile(LOG_DEBUG, "dev(%u) kernel(%u) fr_id(%u) dir(%u) fpga_conv_get_err_prot_func_mask prot_eof              (%u)\n", dev_id, lane, fr_id, direction, rvalue_fpga_func_set_err_prot_mask.prot_eof              );
						logfile(LOG_DEBUG, "dev(%u) kernel(%u) fr_id(%u) dir(%u) fpga_conv_get_err_prot_func_mask prot_reqresp          (%u)\n", dev_id, lane, fr_id, direction, rvalue_fpga_func_set_err_prot_mask.prot_reqresp          );
						logfile(LOG_DEBUG, "dev(%u) kernel(%u) fr_id(%u) dir(%u) fpga_conv_get_err_prot_func_mask prot_datanum          (%u)\n", dev_id, lane, fr_id, direction, rvalue_fpga_func_set_err_prot_mask.prot_datanum          );
						logfile(LOG_DEBUG, "dev(%u) kernel(%u) fr_id(%u) dir(%u) fpga_conv_get_err_prot_func_mask prot_req_outstanding  (%u)\n", dev_id, lane, fr_id, direction, rvalue_fpga_func_set_err_prot_mask.prot_req_outstanding  );
						logfile(LOG_DEBUG, "dev(%u) kernel(%u) fr_id(%u) dir(%u) fpga_conv_get_err_prot_func_mask prot_resp_outstanding (%u)\n", dev_id, lane, fr_id, direction, rvalue_fpga_func_set_err_prot_mask.prot_resp_outstanding );
						logfile(LOG_DEBUG, "dev(%u) kernel(%u) fr_id(%u) dir(%u) fpga_conv_get_err_prot_func_mask prot_max_datanum      (%u)\n", dev_id, lane, fr_id, direction, rvalue_fpga_func_set_err_prot_mask.prot_max_datanum      );
						logfile(LOG_DEBUG, "dev(%u) kernel(%u) fr_id(%u) dir(%u) fpga_conv_get_err_prot_func_mask prot_reqlen           (%u)\n", dev_id, lane, fr_id, direction, rvalue_fpga_func_set_err_prot_mask.prot_reqlen           );
						logfile(LOG_DEBUG, "dev(%u) kernel(%u) fr_id(%u) dir(%u) fpga_conv_get_err_prot_func_mask prot_reqresplen       (%u)\n", dev_id, lane, fr_id, direction, rvalue_fpga_func_set_err_prot_mask.prot_reqresplen       );
					}
				}
			}
		}
	}

	rslt2file("--- filter/resize ---\n");
	for (size_t i=0; i < FUNCTION_KRNL_NUM_MAX; i++) {
		uint32_t lane = i;
		for (size_t j=0; j < DIR_MAX; j++) {
			uint32_t direction = j;
			for (size_t k=0; k < FR_NUM_MAX; k++) {
				uint32_t fr_id = k;
				fpga_func_err_prot_t wvalue_fpga_func_set_err_prot_mask = { 0 };
				fpga_func_err_prot_t rvalue_fpga_func_set_err_prot_mask;
				if (direction == INGRESS) {
					wvalue_fpga_func_set_err_prot_mask.prot_ch = 1;
					wvalue_fpga_func_set_err_prot_mask.prot_len = 1;
					wvalue_fpga_func_set_err_prot_mask.prot_sof = 1;
					wvalue_fpga_func_set_err_prot_mask.prot_eof = 1;
					wvalue_fpga_func_set_err_prot_mask.prot_reqresp = 1;
					wvalue_fpga_func_set_err_prot_mask.prot_datanum = 1;
					wvalue_fpga_func_set_err_prot_mask.prot_req_outstanding = 1;
					wvalue_fpga_func_set_err_prot_mask.prot_resp_outstanding = 1;
					wvalue_fpga_func_set_err_prot_mask.prot_max_datanum = 1;
					wvalue_fpga_func_set_err_prot_mask.prot_reqlen = 0;
					wvalue_fpga_func_set_err_prot_mask.prot_reqresplen = 1;
				} else {
					wvalue_fpga_func_set_err_prot_mask.prot_datanum = 1;
					wvalue_fpga_func_set_err_prot_mask.prot_resp_outstanding = 1;
				}
				ret = fpga_filter_resize_set_err_prot_mask(dev_id, lane, fr_id, direction, wvalue_fpga_func_set_err_prot_mask);
				if (ret < 0) {
						logfile(LOG_ERROR, "dev(%u) kernel(%u) fr_id(%u) dir(%u) fpga_filter_resize_set_err_prot_mask :ret(%d) error!!\n", dev_id, lane, fr_id, direction, ret);
				} else {
					ret = fpga_filter_resize_get_err_prot_mask(dev_id, lane, fr_id, direction, &rvalue_fpga_func_set_err_prot_mask);
					if (ret < 0) {
						logfile(LOG_ERROR, "dev(%u) kernel(%u) fr_id(%u) dir(%u) fpga_filter_resize_get_err_prot_mask :ret(%d) error!!\n", dev_id, lane, fr_id, direction, ret);
					} else {
						logfile(LOG_DEBUG, "dev(%u) kernel(%u) fr_id(%u) dir(%u) fpga_filter_resize_get_err_prot_mask prot_ch               (%u)\n", dev_id, lane, fr_id, direction, rvalue_fpga_func_set_err_prot_mask.prot_ch               );
						logfile(LOG_DEBUG, "dev(%u) kernel(%u) fr_id(%u) dir(%u) fpga_filter_resize_get_err_prot_mask prot_len              (%u)\n", dev_id, lane, fr_id, direction, rvalue_fpga_func_set_err_prot_mask.prot_len              );
						logfile(LOG_DEBUG, "dev(%u) kernel(%u) fr_id(%u) dir(%u) fpga_filter_resize_get_err_prot_mask prot_sof              (%u)\n", dev_id, lane, fr_id, direction, rvalue_fpga_func_set_err_prot_mask.prot_sof              );
						logfile(LOG_DEBUG, "dev(%u) kernel(%u) fr_id(%u) dir(%u) fpga_filter_resize_get_err_prot_mask prot_eof              (%u)\n", dev_id, lane, fr_id, direction, rvalue_fpga_func_set_err_prot_mask.prot_eof              );
						logfile(LOG_DEBUG, "dev(%u) kernel(%u) fr_id(%u) dir(%u) fpga_filter_resize_get_err_prot_mask prot_reqresp          (%u)\n", dev_id, lane, fr_id, direction, rvalue_fpga_func_set_err_prot_mask.prot_reqresp          );
						logfile(LOG_DEBUG, "dev(%u) kernel(%u) fr_id(%u) dir(%u) fpga_filter_resize_get_err_prot_mask prot_datanum          (%u)\n", dev_id, lane, fr_id, direction, rvalue_fpga_func_set_err_prot_mask.prot_datanum          );
						logfile(LOG_DEBUG, "dev(%u) kernel(%u) fr_id(%u) dir(%u) fpga_filter_resize_get_err_prot_mask prot_req_outstanding  (%u)\n", dev_id, lane, fr_id, direction, rvalue_fpga_func_set_err_prot_mask.prot_req_outstanding  );
						logfile(LOG_DEBUG, "dev(%u) kernel(%u) fr_id(%u) dir(%u) fpga_filter_resize_get_err_prot_mask prot_resp_outstanding (%u)\n", dev_id, lane, fr_id, direction, rvalue_fpga_func_set_err_prot_mask.prot_resp_outstanding );
						logfile(LOG_DEBUG, "dev(%u) kernel(%u) fr_id(%u) dir(%u) fpga_filter_resize_get_err_prot_mask prot_max_datanum      (%u)\n", dev_id, lane, fr_id, direction, rvalue_fpga_func_set_err_prot_mask.prot_max_datanum      );
						logfile(LOG_DEBUG, "dev(%u) kernel(%u) fr_id(%u) dir(%u) fpga_filter_resize_get_err_prot_mask prot_reqlen           (%u)\n", dev_id, lane, fr_id, direction, rvalue_fpga_func_set_err_prot_mask.prot_reqlen           );
						logfile(LOG_DEBUG, "dev(%u) kernel(%u) fr_id(%u) dir(%u) fpga_filter_resize_get_err_prot_mask prot_reqresplen       (%u)\n", dev_id, lane, fr_id, direction, rvalue_fpga_func_set_err_prot_mask.prot_reqresplen       );
					}
				}
			}
		}
	}
}

int32_t tp_function_filter_resize_init(uint32_t dev_id)
{
	int32_t ret = 0;
	uint32_t index = dev_id_to_index(dev_id);
	
	rslt2file("module_id check.\n");
	for (size_t i=0; i < CHAIN_KRNL_NUM_MAX; i++) {
		uint32_t module_id;
		fpga_chain_get_module_id(dev_id, i, &module_id);
		if(module_id == 0x0000F0C0) {
			rslt2file("(dev %u, krnl %u) fpga_chain_get_module_id() ok! 0x%x\n", dev_id, i, module_id);
		} else {
			rslt2file("(dev %u, krnl %u) fpga_chain_get_module_id() error! 0x%x\n", dev_id, i, module_id);
		}
	}
	for (size_t i=0; i < CHAIN_KRNL_NUM_MAX; i++) {
		uint32_t module_id;
		fpga_direct_get_module_id(dev_id, i, &module_id);
		if(module_id == 0x0000F3C0) {
			rslt2file("(dev %u, krnl %u) fpga_direct_get_module_id() ok! 0x%x\n", dev_id, i, module_id);
		} else {
			rslt2file("(dev %u, krnl %u) fpga_direct_get_module_id() error! 0x%x\n", dev_id, i, module_id);
		}
	}
	for (size_t i=0; i < FUNCTION_KRNL_NUM_MAX; i++) {
		uint32_t module_id;
		fpga_filter_resize_get_module_id(dev_id, i, &module_id);
		if(module_id == 0x0000F2C2) {
			rslt2file("(dev %u, krnl %u) fpga_filter_resize_get_module_id() ok! 0x%x\n", dev_id, i, module_id);
		} else {
			rslt2file("(dev %u, krnl %u) fpga_filter_resize_get_module_id() error! 0x%x\n", dev_id, i, module_id);
		}
	}
	for (size_t i=0; i < CONV_KRNL_NUM_MAX; i++) {
		uint32_t module_id;
		fpga_conv_get_module_id(dev_id, i, &module_id);
		if(module_id == 0x0000F1C2) {
			rslt2file("(dev %u, krnl %u) fpga_conv_get_module_id() ok! 0x%x\n", dev_id, i, module_id);
		} else {
			rslt2file("(dev %u, krnl %u) fpga_conv_get_module_id() error! 0x%x\n", dev_id, i, module_id);
		}
	}

	logfile(LOG_DEBUG, "--- fpga_function filter_resize init ---\n");
	for (size_t i=0; i < FUNCTION_KRNL_NUM_MAX; i++) {
		uint32_t krnl_id = i;
		uint32_t lch_idx = i * (CH_NUM_MAX / FUNCTION_KRNL_NUM_MAX);
		for (size_t j=0; j < (CH_NUM_MAX / FUNCTION_KRNL_NUM_MAX); j++) {
			uint32_t ch_id = j + lch_idx;
			if (getopt_ch_en(ch_id)) {
				uint32_t input_height = getparam_frame_height_in(index, ch_id);
				uint32_t input_width = getparam_frame_width_in(index, ch_id);
				uint32_t output_height = getparam_frame_height_out(index, ch_id);
				uint32_t output_width = getparam_frame_width_out(index, ch_id);
				char json_txt[256];
				snprintf(json_txt, 256, JSON_FORMAT, input_width, input_height, output_width, output_height);
				logfile(LOG_DEBUG, "dev(%u) func_kernel(%u) json_txt: %s\n", dev_id, krnl_id, json_txt);
				//filter resize
				logfile(LOG_DEBUG, "dev(%u) func_kernel(%u) fpga_function_config\n", dev_id, krnl_id);
				ret = fpga_function_config(dev_id, krnl_id, "filter_resize");
				if (ret < 0) {
					logfile(LOG_ERROR, "fpga_function_config error!!!(%d)\n",ret);
					//error
					return -1;
				}
				logfile(LOG_DEBUG, "dev(%u) func_kernel(%u) fpga_function_init\n", dev_id, krnl_id);
				ret = fpga_function_init(dev_id, krnl_id, NULL);
				if (ret < 0) {
					logfile(LOG_ERROR, "fpga_function_init error!!!(%d)\n",ret);
					//error
					return -1;
				}
				logfile(LOG_DEBUG, "dev(%u) func_kernel(%u) fpga_function_set\n", dev_id, krnl_id);
				ret = fpga_function_set(dev_id, krnl_id, json_txt);
				if (ret < 0) {
					logfile(LOG_ERROR, "fpga_function_set error!!!(%d)\n",ret);
					//error
					return -2;
				}

				break;
			}
		}
	}
	//conv is executed from within filter/resize, no need for user execution
	//chain configuration
	for (size_t i=0; i < CHAIN_KRNL_NUM_MAX; i++) {
		uint32_t krnl_id = i;
		uint32_t extif_id = EXTIFID;
		fpga_chain_set_ddr(dev_id, krnl_id, extif_id);
	}
	//ddr setting check
	for (size_t i=0; i < CHAIN_KRNL_NUM_MAX; i++) {
		uint32_t krnl_id = i;
		rslt2file("\n--- ddr offset ---\n");
		uint32_t extif_id = EXTIFID;
		fpga_chain_ddr_t chain_ddr;
		fpga_chain_get_ddr(dev_id, krnl_id, extif_id, &chain_ddr);
		rslt2file("dev(%u) kernel(%u) extif(%u)\n", dev_id, krnl_id, extif_id);
		rslt2file("base         0x%" PRIx64 "  \n", chain_ddr.base         );
		rslt2file("rx_offset    0x%" PRIx64 "  \n", chain_ddr.rx_offset    );
		rslt2file("rx_stride    0x%x   \n", chain_ddr.rx_stride    );
		rslt2file("tx_offset    0x%" PRIx64 "  \n", chain_ddr.tx_offset    );
		rslt2file("tx_stride    0x%x   \n", chain_ddr.tx_stride    );
		rslt2file("rx_size      %u   \n", chain_ddr.rx_size      );
		rslt2file("tx_size      %u   \n", chain_ddr.tx_size      );
	}
	// protocol err mask
	tp_protocol_mask_set(dev_id);

	//start chain
	for (size_t i=0; i < CHAIN_KRNL_NUM_MAX; i++) {
		uint32_t krnl_id = i;
		fpga_direct_start(dev_id, krnl_id);
		fpga_chain_start(dev_id, krnl_id);
	}
	return 0;
}
int32_t tp_enqueue_fdma_init(uint32_t dev_id)
{
	int32_t ret = 0;

	logfile(LOG_DEBUG, "--- enqueue fpga_lldma_init ---\n");
	for (size_t i=0; i < CH_NUM_MAX; i++) {
		uint32_t ch_id = i;
		if (getopt_ch_en(ch_id)) {
			dma_info_t *pdmainfo_ch = get_enqdmainfo_channel(dev_id, ch_id);
			memset(pdmainfo_ch, 0, sizeof(dma_info_t));
			char *connector_id = (char*)getparam_enq_connector_id(ch_id);
			logfile(LOG_DEBUG, "dev(%u) CH(%zu) enqueue fpga_lldma_init\n", dev_id, ch_id);
			ret = fpga_lldma_init(dev_id, DMA_HOST_TO_DEV, ch_id, connector_id, pdmainfo_ch);
			if (ret < 0) {
				logfile(LOG_ERROR, "enqueue fpga_lldma_init error!!!(%d)\n",ret);
				// error
				return -1;
			}
			prlog_dma_info(pdmainfo_ch, ch_id);
		}
	}

	return 0;
}

int32_t tp_dequeue_fdma_init(uint32_t dev_id)
{
	int32_t ret = 0;

	logfile(LOG_DEBUG, "--- dequeue fpga_lldma_init ---\n");
	for (size_t i=0; i < CH_NUM_MAX; i++) {
		uint32_t ch_id = i;
		if (getopt_ch_en(ch_id)) {
			dma_info_t *pdmainfo_ch = get_deqdmainfo_channel(dev_id, ch_id);
			memset(pdmainfo_ch, 0, sizeof(dma_info_t));
			char *connector_id = (char*)getparam_deq_connector_id(ch_id);
			logfile(LOG_DEBUG, "dev(%u) CH(%zu) dequeue fpga_lldma_init\n", dev_id, ch_id);
			ret = fpga_lldma_init(dev_id, DMA_DEV_TO_HOST, ch_id, connector_id, pdmainfo_ch);
			if (ret < 0) {
				logfile(LOG_ERROR, "dequeue fpga_lldma_init error!!!(%d)\n",ret);
				// error
				return -1;
			}
			prlog_dma_info(pdmainfo_ch, ch_id);
		}
	}

	return 0;
}

int32_t tp_fpga_buf_connect(mngque_t *pque)
{
	int32_t ret = 0;

	rslt2file("\n--- fpga d2d buf connect ---\n");
	logfile(LOG_DEBUG, "--- fpga_lldma_buf_connect ---\n");
	for (size_t i=0; i < CH_NUM_MAX; i++) {
		uint32_t ch_id = i;
		if (getopt_ch_en(ch_id)) {
			fpga_lldma_connect_t *pconnectinfo = get_connectinfo(ch_id);
			memset(pconnectinfo, 0, sizeof(fpga_lldma_connect_t));
			pconnectinfo->tx_dev_id = *get_dev_id(0);
			pconnectinfo->tx_chid = ch_id;
			pconnectinfo->rx_dev_id = *get_dev_id(1);
			pconnectinfo->rx_chid = ch_id;
			pconnectinfo->buf_size = pque[ch_id].d2dbuflen;
			pconnectinfo->buf_addr = pque[ch_id].d2dbufp;
			char con[64];
			sprintf(con, "d2d_connector_id%u", ch_id);
			pconnectinfo->connector_id = con;
			prlog_connect_info(pconnectinfo, ch_id);
			logfile(LOG_DEBUG, "CH(%zu) fpga_lldma_buf_connect\n", ch_id);
			rslt2file("CH(%u) tx_dev(%u) rx_dev(%u) d2dbuf_len(%u) d2dbuf_addr(%p)\n", ch_id, pconnectinfo->tx_dev_id, pconnectinfo->rx_dev_id, pconnectinfo->buf_size, pconnectinfo->buf_addr);
			ret = fpga_lldma_buf_connect(pconnectinfo);
			if (ret < 0) {
				logfile(LOG_ERROR, "fpga_lldma_buf_connect error!!!(%d)\n",ret);
				// error
				return -1;
			}
		}
	}

	return 0;
}

int32_t tp_fpga_direct_connect(void)
{
	int32_t ret = 0;

	rslt2file("\n--- fpga d2d direct connect ---\n");
	logfile(LOG_DEBUG, "--- fpga_lldma_direct_connect ---\n");
	for (size_t i=0; i < CH_NUM_MAX; i++) {
		uint32_t ch_id = i;
		if (getopt_ch_en(ch_id)) {
			fpga_lldma_connect_t *pconnectinfo = get_connectinfo(ch_id);
			memset(pconnectinfo, 0, sizeof(fpga_lldma_connect_t));
			pconnectinfo->tx_dev_id = *get_dev_id(0);
			pconnectinfo->tx_chid = ch_id;
			pconnectinfo->rx_dev_id = *get_dev_id(1);
			pconnectinfo->rx_chid = ch_id;
			pconnectinfo->buf_size = 0;
			pconnectinfo->buf_addr = NULL;
			char con[64];
			sprintf(con, "d2d_connector_id%u", ch_id);
			pconnectinfo->connector_id = con;
			prlog_connect_info(pconnectinfo, ch_id);
			logfile(LOG_DEBUG, "CH(%zu) fpga_lldma_direct_connect\n", ch_id);
			rslt2file("CH(%u) tx_dev(%u) rx_dev(%u)\n", ch_id, pconnectinfo->tx_dev_id, pconnectinfo->rx_dev_id);
			ret = fpga_lldma_direct_connect(pconnectinfo);
			if (ret < 0) {
				logfile(LOG_ERROR, "fpga_lldma_direct_connect error!!!(%d)\n",ret);
				// error
				return -1;
			}
		}
	}

	return 0;
}

void tp_fpga_buf_disconnect(void)
{
	int32_t ret = 0;

	logfile(LOG_DEBUG, "--- fpga_lldma_buf_disconnect ---\n");
	for (size_t i=0; i < CH_NUM_MAX; i++) {
		uint32_t ch_id = i;
		if (getopt_ch_en(ch_id)) {
			fpga_lldma_connect_t *pconnectinfo = get_connectinfo(ch_id);
			logfile(LOG_DEBUG, "CH(%zu) fpga_lldma_buf_disconnect\n", ch_id);
			ret = fpga_lldma_buf_disconnect(pconnectinfo);
			if (ret < 0) {
				// error
				logfile(LOG_ERROR, "fpga_lldma_buf_disconnect error!!!(%d)\n",ret);
			}
		}
	}
}

void tp_fpga_direct_disconnect(void)
{
	int32_t ret = 0;

	logfile(LOG_DEBUG, "--- fpga_direct_disconnect ---\n");
	for (size_t i=0; i < CH_NUM_MAX; i++) {
		uint32_t ch_id = i;
		if (getopt_ch_en(ch_id)) {
			fpga_lldma_connect_t *pconnectinfo = get_connectinfo(ch_id);
			logfile(LOG_DEBUG, "CH(%zu) fpga_direct_disconnect\n", ch_id);
			ret = fpga_lldma_direct_disconnect(pconnectinfo);
			if (ret < 0) {
				// error
				logfile(LOG_ERROR, "fpga_direct_disconnect error!!!(%d)\n",ret);
			}
		}
	}
}

int32_t tp_enqueue_fdma_queue_setup(uint32_t dev_id)
{
	int32_t ret = 0;

	logfile(LOG_DEBUG, "--- enqueue fpga_lldma_queue_setup ---\n");
	for (size_t i=0; i < CH_NUM_MAX; i++) {
		uint32_t ch_id = i;
		if (getopt_ch_en(ch_id)) {
			dma_info_t *pdmainfo_ch = get_enqdmainfo_channel(dev_id, ch_id);
			dma_info_t *pdmainfo = get_enqdmainfo(dev_id, ch_id);
			memset(pdmainfo, 0, sizeof(dma_info_t));
			char *connector_id = pdmainfo_ch->connector_id;
			logfile(LOG_DEBUG, "dev(%zu) CH(%zu) enqueue fpga_lldma_queue_setup\n", dev_id, ch_id);
			ret = fpga_lldma_queue_setup(connector_id, pdmainfo);
			if (ret < 0) {
				logfile(LOG_ERROR, "enqueue fpga_lldma_queue_setup error!!!(%d)\n",ret);
				// error
				return -1;
			}
			prlog_dma_info(pdmainfo, ch_id);
		}
	}

	return 0;
}

int32_t tp_dequeue_fdma_queue_setup(uint32_t dev_id)
{
	int32_t ret = 0;

	logfile(LOG_DEBUG, "--- dequeue fpga_lldma_queue_setup ---\n");
	for (size_t i=0; i < CH_NUM_MAX; i++) {
		uint32_t ch_id = i;
		if (getopt_ch_en(ch_id)) {
			dma_info_t *pdmainfo_ch = get_deqdmainfo_channel(dev_id, ch_id);
			dma_info_t *pdmainfo = get_deqdmainfo(dev_id, ch_id);
			memset(pdmainfo, 0, sizeof(dma_info_t));
			char *connector_id = pdmainfo_ch->connector_id;
			logfile(LOG_DEBUG, "dev(%zu) CH(%zu) dequeue fpga_lldma_queue_setup\n", dev_id, ch_id);
			ret = fpga_lldma_queue_setup(connector_id, pdmainfo);
			if (ret < 0) {
				logfile(LOG_ERROR, "dequeue fpga_lldma_queue_setup error!!!(%d)\n",ret);
				// error
				return -1;
			}
			prlog_dma_info(pdmainfo, ch_id);
		}
	}

	return 0;
}

int32_t tp_enqueue_set_dma_cmd(uint32_t run_id, uint32_t enq_num, mngque_t *pque)
{
	int32_t ret = 0;
	const divide_que_t *div_que = get_divide_que();

	logfile(LOG_DEBUG, "--- enqueue set_dma_cmd ---\n");
	rslt2file("\n--- enqueue set dma cmd ---\n");
	for (size_t i=0; i < CH_NUM_MAX; i++) {
		uint32_t ch_id = i;
		if (getopt_ch_en(ch_id)) {
			uint32_t data_len = pque[ch_id].srcbuflen;
			uint32_t dsize = pque[ch_id].srcdsize;
			uint32_t dstidx = 0;
			uint16_t taskidx = 1 + run_id * div_que->que_num;
			rslt2file("CH(%zu) dma rx data size=%d Byte\n", ch_id, dsize);
			for (size_t k=0; k < enq_num; k++) {
				uint32_t enq_id = k + run_id * div_que->que_num;
				uint16_t task_id = taskidx;
				if (dstidx >= getopt_shmalloc_num()) {
					dstidx = 0;
				}
				void *data_addr = pque[ch_id].enqbuf[dstidx].srcbufp;
				dstidx++;
				dmacmd_info_t *pdmacmdinfo = get_enqdmacmdinfo(ch_id, enq_id);
				memset(pdmacmdinfo, 0, sizeof(dmacmd_info_t));
				logfile(LOG_DEBUG, "CH(%zu) ENQ(%zu) set_dma_cmd\n", ch_id, enq_id);
				ret = set_dma_cmd(pdmacmdinfo, task_id, data_addr, data_len);
				if (ret < 0) {
					logfile(LOG_ERROR, "enqueue set_dma_cmd error!!!(%d)\n",ret);
					// error
					return -1;
				}
				prlog_dmacmd_info(pdmacmdinfo, ch_id, enq_id);

				if (taskidx == 0xFFFF) {
					taskidx = 1;
				} else {
					taskidx++;
				}
			}
		}
	}

	return 0;
}

int32_t tp_dequeue_set_dma_cmd(uint32_t run_id, uint32_t enq_num, mngque_t *pque)
{
	int32_t ret = 0;
	const divide_que_t *div_que = get_divide_que();

	logfile(LOG_DEBUG, "--- dequeue set_dma_cmd ---\n");
	rslt2file("\n--- dequeue set dma cmd ---\n");
	for (size_t i=0; i < CH_NUM_MAX; i++) {
		uint32_t ch_id = i;
		if (getopt_ch_en(ch_id)) {
			uint32_t data_len = pque[ch_id].dst1buflen;
			uint32_t dsize = pque[ch_id].dst1dsize;
			uint32_t dstidx = 0;
			uint16_t taskidx = 1 + run_id * div_que->que_num;
			rslt2file("CH(%zu) dma tx data size=%d Byte\n", ch_id, dsize);
			for (size_t k=0; k < enq_num; k++) {
				uint32_t enq_id = k + run_id * div_que->que_num;
				uint16_t task_id = taskidx;
				if (dstidx >= getopt_shmalloc_num()) {
					dstidx = 0;
				}
				void *data_addr = pque[ch_id].enqbuf[dstidx].dst1bufp;
				dstidx++;
				dmacmd_info_t *pdmacmdinfo = get_deqdmacmdinfo(ch_id, enq_id);
				memset(pdmacmdinfo, 0, sizeof(dmacmd_info_t));
				logfile(LOG_DEBUG, "CH(%zu) DEQ(%zu) set_dma_cmd\n", ch_id, enq_id);
				ret = set_dma_cmd(pdmacmdinfo, task_id, data_addr, data_len);
				if (ret < 0) {
					logfile(LOG_ERROR, "dequeue set_dma_cmd error!!!(%d)\n",ret);
					// error
					return -1;
				}
				prlog_dmacmd_info(pdmacmdinfo, ch_id, enq_id);

				if (taskidx == 0xFFFF) {
					taskidx = 1;
				} else {
					taskidx++;
				}
			}
		}
	}

	return 0;
}


int32_t tp_chain_connect(uint32_t dev_id)
{
	int32_t ret = 0;

	rslt2file("\n--- function chain connect ---\n");
	logfile(LOG_DEBUG, "--- fpga_chain_connect ---\n");

	for (size_t i=0; i < CH_NUM_MAX; i++) {
		uint32_t ch_id = i;
		if (getopt_ch_en(ch_id)) {
			uint32_t chain_krnl_id = get_chain_krnl_id(ch_id);
			uint32_t fchid = getparam_function_chid(ch_id);
			uint8_t direct_flag = (uint8_t)getparam_is_direct_flag(dev_id, ch_id);
			uint8_t ig_active_flag = (uint8_t)getparam_is_ig_active_flag(dev_id, ch_id);
			uint8_t eg_active_flag = (uint8_t)getparam_is_eg_active_flag(dev_id, ch_id);
			uint8_t virtual_flag = (uint8_t)getparam_is_eg_virtual_flag(dev_id, ch_id);
			uint8_t blocking_flag = (uint8_t)getparam_is_eg_blocking_flag(dev_id, ch_id);
			uint32_t ingress_extif_id;
			uint32_t ingress_cid;
			ingress_extif_id = (getparam_fdma_extif_id(ch_id) == 0) ? 0: 1;
			ingress_cid = getparam_fdma_cid(ch_id);
			uint32_t egress_extif_id;
			uint32_t egress_cid;
			egress_extif_id = (getparam_fdma_extif_id(ch_id) == 0) ? 0: 1;
			egress_cid = getparam_fdma_cid(ch_id);
			logfile(LOG_DEBUG, "dev(%u) CH(%zu) fpga_chain_connect\n", dev_id, ch_id);
			logfile(LOG_DEBUG, "  func_kernel_id(%u), fchid(%u) ingress_extif_id(%u) ingress_cid(%u) egress_extif_id(%u) egress_cid(%u)\n", chain_krnl_id, fchid, ingress_extif_id, ingress_cid, egress_extif_id, egress_cid);

			// confirm ingress connection establishment
			uint32_t con_status;
			bool con_err = false;
			ret = fpga_chain_get_con_status(dev_id, chain_krnl_id, ingress_extif_id, ingress_cid, &con_status);
			if (ret < 0) {
				logfile(LOG_ERROR, "dev(%u) CH(%zu) func_kernel_id(%u) fpga_chain_get_con_status() error!!!(%d)\n", dev_id, ch_id, chain_krnl_id, ret);
				con_err = true;
			}
			if (con_status == 0) {
				logfile(
					LOG_ERROR,
					"dev(%u) CH(%zu) func_kernel_id(%u) fpga_chain_get_con_status() chain connection error. ingress_extif_id(%u) ingress_cid(%u) status(0x%x)\n",
					dev_id, ch_id, chain_krnl_id, ingress_extif_id, ingress_cid, con_status
				);
				rslt2file(
					"dev(%u) CH(%zu) func_kernel_id(%u) chain connection error! ingress_extif_id(%u) ingress_cid(%u) status(0x%x)\n",
					dev_id, ch_id, chain_krnl_id, ingress_extif_id, ingress_cid, con_status
				);
				con_err = true;
			} else {
				logfile(
					LOG_DEBUG,
					"dev(%u) CH(%zu) func_kernel_id(%u) fpga_chain_get_con_status() chain connection established. ingress_extif_id(%u) ingress_cid(%u) status(0x%x)\n",
					dev_id, ch_id, chain_krnl_id, ingress_extif_id, ingress_cid, con_status
				);
				rslt2file(
					"dev(%u) CH(%zu) func_kernel_id(%u) chain connection established. ingress_extif_id(%u) ingress_cid(%u) status(0x%x)\n",
					dev_id, ch_id, chain_krnl_id, ingress_extif_id, ingress_cid, con_status
				);
			}
			// egress connection establishment confirmation
			ret = fpga_chain_get_con_status(dev_id, chain_krnl_id, egress_extif_id, egress_cid, &con_status);
			if (ret < 0) {
				logfile(LOG_ERROR, "dev(%u) func_kernel_id(%u) fpga_chain_get_con_status() error!!!(%d)\n", dev_id, chain_krnl_id, ret);
				con_err = true;
			}
			if (con_status == 0) {
				logfile(
					LOG_ERROR,
					"dev(%u) CH(%zu) func_kernel_id(%u) fpga_chain_get_con_status() chain connection error. egress_extif_id(%u) egress_cid(%u) status(0x%x)\n",
					dev_id, ch_id, chain_krnl_id, egress_extif_id, egress_cid, con_status
				);
				rslt2file(
					"dev(%u) CH(%zu) func_kernel_id(%u) chain connection error! egress_extif_id(%u) egress_cid(%u) status(0x%x)\n",
					dev_id, ch_id, chain_krnl_id, egress_extif_id, egress_cid, con_status
				);
				con_err = true;
			} else {
				logfile(
					LOG_DEBUG,
					"dev(%u) CH(%zu) func_kernel_id(%u) fpga_chain_get_con_status() chain connection established. egress_extif_id(%u) egress_cid(%u) status(0x%x)\n",
					dev_id, ch_id, chain_krnl_id, egress_extif_id, egress_cid, con_status
				);
				rslt2file(
					"dev(%u) CH(%zu) func_kernel_id(%u) chain connection established. egress_extif_id(%u) egress_cid(%u) status(0x%x)\n",
					dev_id, ch_id, chain_krnl_id, egress_extif_id, egress_cid, con_status
				);
			}
			if (con_err) {
				return -1;
			}

			rslt2file("dev(%u) CH(%u) func_kernel_id(%u), fchid(%u) ingress_extif_id(%u) ingress_cid(%u) egress_extif_id(%u) egress_cid(%u)\n", dev_id, ch_id, chain_krnl_id, fchid, ingress_extif_id, ingress_cid, egress_extif_id, egress_cid);
			ret = fpga_chain_connect(dev_id, chain_krnl_id, fchid, ingress_extif_id, ingress_cid, egress_extif_id, egress_cid, ig_active_flag, eg_active_flag, direct_flag, virtual_flag, blocking_flag);
			if (ret < 0) {
				logfile(LOG_ERROR, "fpga_chain_connect error!!!(%d)\n",ret);
				// error
				return -1;
			}
		}
	}
	return 0;
}

void tp_enqueue_fdma_queue_finish(uint32_t dev_id)
{
	int32_t ret = 0;

	logfile(LOG_DEBUG, "--- enqueue fpga_lldma_queue_finish ---\n");
	for (size_t i=0; i < CH_NUM_MAX; i++) {
		uint32_t ch_id = i;
		if (getopt_ch_en(ch_id)) {
			logfile(LOG_DEBUG, "dev(%zu) CH(%zu) enqueue fpga_lldma_queue_finish\n", dev_id, ch_id);
			dma_info_t *pdmainfo = get_enqdmainfo(dev_id, ch_id);
			ret = fpga_lldma_queue_finish(pdmainfo);
			if (ret < 0) {
				logfile(LOG_ERROR, "enqueue fpga_lldma_queue_finish error!!!(%d)\n",ret);
				// error
			}
			pdmainfo->connector_id = NULL;
			prlog_dma_info(pdmainfo, ch_id);
		}
	}
}

void tp_dequeue_fdma_queue_finish(uint32_t dev_id)
{
	int32_t ret = 0;

	logfile(LOG_DEBUG, "--- dequeue fpga_lldma_queue_finish ---\n");
	for (size_t i=0; i < CH_NUM_MAX; i++) {
		uint32_t ch_id = i;
		if (getopt_ch_en(ch_id)) {
			logfile(LOG_DEBUG, "dev(%zu) CH(%zu) dequeue fpga_lldma_queue_finish\n", dev_id, ch_id);
			dma_info_t *pdmainfo = get_deqdmainfo(dev_id, ch_id);
			ret = fpga_lldma_queue_finish(pdmainfo);
			if (ret < 0) {
				logfile(LOG_ERROR, "dequeue fpga_lldma_queue_finish error!!!(%d)\n",ret);
				// error
			}
			pdmainfo->connector_id = NULL;
			prlog_dma_info(pdmainfo, ch_id);
		}
	}
}

void tp_function_finish(uint32_t dev_id)
{
	int32_t ret = 0;

	logfile(LOG_DEBUG, "--- fpga_function_finish ---\n");
	for (size_t i=0; i < FUNCTION_KRNL_NUM_MAX; i++) {
		uint32_t krnl_id = i;
		uint32_t lch_idx = i * (CH_NUM_MAX / FUNCTION_KRNL_NUM_MAX);
		for (size_t j=0; j < (CH_NUM_MAX / FUNCTION_KRNL_NUM_MAX); j++) {
			uint32_t ch_id = j + lch_idx;
			if (getopt_ch_en(ch_id)) {
				logfile(LOG_DEBUG, "dev(%u) func_kernel(%u) fpga_function_finish\n", dev_id, krnl_id);
				ret = fpga_function_finish(dev_id, krnl_id, NULL);
				if (ret < 0) {
					logfile(LOG_ERROR, "fpga_function_finish error!!!(%d)\n",ret);
					// error
				}

				break;
			}
		}
	}
}

void tp_enqueue_fdma_finish(uint32_t dev_id)
{
	int32_t ret = 0;

	logfile(LOG_DEBUG, "--- enqueue fpga_lldma_finish ---\n");
	for (size_t i=0; i < CH_NUM_MAX; i++) {
		uint32_t ch_id = i;
		if (getopt_ch_en(ch_id)) {
			logfile(LOG_DEBUG, "dev(%zu) CH(%zu) enqueue fpga_dma_finish\n", dev_id, ch_id);
			dma_info_t *pdmainfo_ch = get_enqdmainfo_channel(dev_id, ch_id);
			ret = fpga_lldma_finish(pdmainfo_ch);
			if (ret < 0) {
				logfile(LOG_ERROR, "enqueue fpga_dma_finish error!!!(%d)\n",ret);
				// error
			}
			pdmainfo_ch->connector_id = NULL;
			prlog_dma_info(pdmainfo_ch, ch_id);
		}
	}
}

void tp_dequeue_fdma_finish(uint32_t dev_id)
{
	int32_t ret = 0;

	logfile(LOG_DEBUG, "--- dequeue fpga_lldma_finish ---\n");
	for (size_t i=0; i < CH_NUM_MAX; i++) {
		uint32_t ch_id = i;
		if (getopt_ch_en(ch_id)) {
			logfile(LOG_DEBUG, "dev(%zu) CH(%zu) dequeue fpga_dma_finish\n", dev_id, ch_id);
			dma_info_t *pdmainfo_ch = get_deqdmainfo_channel(dev_id, ch_id);
			ret = fpga_lldma_finish(pdmainfo_ch);
			if (ret < 0) {
				logfile(LOG_ERROR, "dequeue fpga_dma_finish error!!!(%d)\n",ret);
				// error
			}
			pdmainfo_ch->connector_id = NULL;
			prlog_dma_info(pdmainfo_ch, ch_id);
		}
	}
}

void tp_chain_disconnect(uint32_t dev_id)
{
	int32_t ret = 0;

	logfile(LOG_DEBUG, "--- fpga_chain_disconnect ---\n");
	for (size_t i=0; i < CH_NUM_MAX; i++) {
		uint32_t ch_id = i;
		if (getopt_ch_en(ch_id)) {
			uint32_t chain_krnl_id = get_chain_krnl_id(ch_id);
			uint32_t fchid = getparam_function_chid(ch_id);
			logfile(LOG_DEBUG, "dev(%u) CH(%zu) fpga_chain_disconnect\n", dev_id, ch_id);
			logfile(LOG_DEBUG, "  func_kernel_id(%u), fchid(%u)\n", chain_krnl_id, fchid);
			ret = fpga_chain_disconnect(dev_id, chain_krnl_id, fchid);
			if (ret < 0) {
				logfile(LOG_ERROR, "fpga_chain_disconnect error!!!(%d)\n",ret);
				// error
			}
		}
	}
}

int32_t tp_outppm_send_data(uint32_t run_id, uint32_t enq_num)
{
	int32_t ret = 0;

	const divide_que_t *div_que = get_divide_que();
	if (getopt_tester_meas_mode()) {
		enq_num = 1;
	}

	logfile(LOG_DEBUG, "--- outppm_send_data ---\n");
	for (size_t i=0; i < CH_NUM_MAX; i++) {
		uint32_t ch_id = i;
		if (getopt_ch_en(ch_id)) {
			for (size_t j=0; j < enq_num; j++) {
				uint32_t enq_id = j + run_id * div_que->que_num;
				ret = outppm_send_data(ch_id, enq_id);
				if (ret < 0) {
					logfile(LOG_ERROR, "outppm_send_data error!!!(%d)\n", ret);
					// error
					return -1;
				}
			}
		}
	}

	return 0;
}

