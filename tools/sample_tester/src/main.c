/*************************************************
* Copyright 2024 NTT Corporation, FUJITSU LIMITED
* Licensed under the 3-Clause BSD License, see LICENSE for details.
* SPDX-License-Identifier: BSD-3-Clause
*************************************************/
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <rte_common.h>
#include <rte_eal.h>
#include "xpcie_device.h"
#include "libshmem.h"
#include "libfpgactl.h"
#include "libdma.h"
#include "libdmacommon.h"
#include "liblldma.h"
#include "libfpgabs.h"
#include "libchain.h"
#include "liblogging.h"
#include "common.h"
#include "param_tables.h"
#include "bcdbg.h"
#include "tp.h"


static int32_t parse_app_args(int argc, char **argv)
{
	int32_t ret = 0;

	ret = parse_app_args_func(argc, argv);
	if (ret < 0) {
		return ret;
	}

	ret = check_options();
	if (ret < 0) {
		return ret;
	}

	return 0;
}

int main(int argc, char **argv)
{
	int32_t ret = 0;

	libfpga_log_set_level(LIBFPGA_LOG_ERROR);
	rslt2file("\nVersion: %s\n", VERSION);

	// define test function
	int32_t (*tp_funcp[])(void) = {
		tp_host_host,
		tp_d2d_h_host_host,
		tp_d2d_d_host_host
	};

	// set command name
	set_cmdname(argv[0]);

	// print usage
	if (argc == 1) {
		print_usage();
		return 0;
	}

	// initialize DPDK
	ret = fpga_shmem_init_arg(argc, argv);
	if (ret < 0) {
		rte_exit(EXIT_FAILURE, "Initialize failed\n");
	}
	logfile(LOG_DEBUG, "fpga_shmem_init_arg:ret(%d)\n",ret);

	// initialize FPGA
	ret = fpga_init(argc, argv);
	if (ret < 0) {
		logfile(LOG_ERROR, "fpga init error!!\n");
		rslt2file("fpga init error!!\n");
		goto _END1;
	}
	logfile(LOG_DEBUG, "fpga_init:ret(%d)\n",ret);
	argc -= ret;
	argv += ret;

	// parse arguments
	ret = parse_app_args(argc, argv);
	if (ret < 0) {
		logfile(LOG_ERROR, "app option error!!\n");
		rslt2file("app option error!!\n");
		goto _END2;
	}
	logfile(LOG_DEBUG, "parse_app_options:ret(%d)\n",ret);

	logfile(LOG_FORCE, "Version: %s\n", VERSION);

	// set dev_id list
	ret = set_dev_id_list();
	if (ret < 0) {
		goto _END2;
	}

	// lock FPGA
	for (size_t i=0; i < fpga_get_num(); i++ ) {
		uint32_t *dev_id = get_dev_id(i);
		ret = fpga_ref_acquire(*dev_id);
		if (ret < 0) {
			logfile(LOG_ERROR, "dev(%u) fpga_ref_acquire:ret(%d) error!!\n", *dev_id, ret);
			rslt2file("dev(%u) fpga_ref_acquire error!!\n", *dev_id, ret);
			goto _END3;
		}
		logfile(LOG_DEBUG, "dev(%u) fpga_ref_acquire:ret(%d)\n", *dev_id, ret);
	}

	// fpga fdma setup buffer
	for (size_t i=0; i < fpga_get_num(); i++ ) {
		uint32_t *dev_id = get_dev_id(i);
		ret = fpga_lldma_setup_buffer(*dev_id);
		if (ret < 0) {
			logfile(LOG_ERROR, "dev(%u) fpga_lldma_setup_buffer:ret(%d) error!!\n", *dev_id, ret);
			rslt2file("dev(%u) fpga_lldma_setup_buffer error!!\n", *dev_id);
			goto _END3;
		}
		logfile(LOG_DEBUG, "dev(%u) fpga_lldma_setup_buffer:ret(%d)\n", *dev_id, ret);
	}

	// fpga enable regrw
	for (size_t i=0; i < fpga_get_num(); i++ ) {
		uint32_t *dev_id = get_dev_id(i);
		ret = fpga_enable_regrw(*dev_id);
		if (ret < 0) {
			logfile(LOG_ERROR, "dev(%u) fpga_enable_regrw:ret(%d) error!!\n", *dev_id, ret);
			rslt2file("dev(%u) fpga_enable_regrw error!!\n", *dev_id);
			goto _END3;
		}
		logfile(LOG_DEBUG, "dev(%u) fpga_enable_regrw:ret(%d)\n", *dev_id, ret);
	}

	// device info
	pr_device_info();

	// make dir
	if (getopt_is_outppm_send_data()) {
		ret = make_dir(SEND_DATA_DIR);
		if (ret < 0)
			goto _END3;
	}
	if (getopt_is_outppm_receive_data()) {
		ret = make_dir(RECEIVE_DATA_DIR);
		if (ret < 0)
			goto _END3;
	}

	// execute TP
	rslt2file("//--- TEST START ---\n");

	tp_model_t tp_model_id = getopt_tp_model();
	logfile(LOG_DEBUG, "tp_funcp[%s]\n", tp_model_name[tp_model_id]);
	ret = tp_funcp[tp_model_id]();
	if (ret < 0) {
		logfile(LOG_ERROR, "tp_funcp[%s] error(%d)\n", tp_model_name[tp_model_id], ret);
		rslt2file("tp_funcp[%s] error(%d)\n", tp_model_name[tp_model_id], ret);
	}

	rslt2file("//--- TEST END ---//\n");

_END3:
	// unlock FPGA
	for (size_t i=0; i < fpga_get_num(); i++ ) {
		uint32_t *dev_id = get_dev_id(i);
		ret = fpga_ref_release(*dev_id);
		if (ret < 0) {
			logfile(LOG_ERROR, "dev(%u) fpga_ref_release:ret(%d) error!!\n", *dev_id, ret);
			rslt2file("dev(%u) fpga_ref_release error!!\n", *dev_id);
		}
		logfile(LOG_DEBUG, "dev(%u) fpga_ref_release:ret(%d)\n", *dev_id, ret);
	}

_END2:
	// finish FPGA
	ret = fpga_finish();
	if (ret < 0) {
		logfile(LOG_ERROR, "fpga finish error!!\n");
		rslt2file("fpga finish error!!\n");
	}
	logfile(LOG_DEBUG, "fpga_finish:ret(%d)\n",ret);

_END1:
	// finish DPDK shmem
	ret = fpga_shmem_finish();
	if (ret < 0) {
		logfile(LOG_ERROR, "fpga shmem finish error!!\n");
		rslt2file("fpga shmem finish error!!\n");
	}
	logfile(LOG_DEBUG, "fpga_shmem_finish:ret(%d)\n",ret);


	return 0;
}
