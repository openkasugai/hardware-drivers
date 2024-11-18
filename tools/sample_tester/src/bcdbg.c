/*************************************************
* Copyright 2024 NTT Corporation, FUJITSU LIMITED
* Licensed under the 3-Clause BSD License, see LICENSE for details.
* SPDX-License-Identifier: BSD-3-Clause
*************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <assert.h>
#include <time.h>
#include <math.h> 
#include <stdbool.h>
#include "xpcie_device.h"
#include "libshmem.h"
#include "liblldma.h"
#include "libdma.h"
#include "libfpgactl.h"
#include "liblogging.h"
#include "libfunction.h"
#include "common.h"
#include "bcdbg.h"
#include "param_tables.h"
#include "cppfunc.h"
#include "tp.h"

static const struct option app_long_options[];
static const char app_short_options[];
static int32_t check_chid_args(const char *str, uint32_t *ch_id_p);
static int32_t parse_chid_string(char *string);

static void* gmm[CH_NUM_MAX][SHMEMALLOC_NUM_MAX];
static void* gmmd2d[CH_NUM_MAX];
static bool gdeqshmstate[CH_NUM_MAX][SHMEMALLOC_NUM_MAX];
static int64_t gdeqreceivep[CH_NUM_MAX];
static uint8_t* gsendimg[CH_NUM_MAX];
static void* greceiveheader[CH_NUM_MAX];
static uint8_t* greceiveimg[CH_NUM_MAX];

//----------------------------------
//  options
//----------------------------------
static const char *g_cmdname = NULL;
static tp_model_t g_tp_model = TP_UNKNOWN;
static bool g_ch_en[CH_NUM_MAX];
static int32_t g_ch_num[LANE_NUM_MAX];
static int32_t g_fps = 0;
static int32_t g_frame_num = 0;
static int32_t g_enq_num = 0;
static int32_t g_loglevel=LOG_ERROR;
static int32_t g_core=0xff;
static int32_t g_shmalloc_num = 0;
static divide_que_t g_divide_que;
static bool g_is_send_data = true;
static bool g_is_receive_data = true;
static bool g_is_outppm_send_data = false;
static bool g_is_outppm_receive_data = false;
static bool g_tester_meas_mode = false;
static bool g_is_performance_meas = false;

static const struct option app_long_options[] = {
	{ "chid", required_argument, NULL, 1 },
	{ "tp", required_argument, NULL, 2 },
	{ "loglevel", required_argument, NULL, 3 },
	{ "deqtcore", required_argument, NULL, 4 },
	{ "ppms", no_argument, NULL, 5 },
	{ "ppmr", no_argument, NULL, 6 },
	{ NULL, 0, NULL, 0 },
};

static const char app_short_options[] = {
	"f:"	// FPS.
	"r:"	// number of send frames.
	"m"		// enable tester measurement mode.
	"p"		// enable performance measurement.
};

void print_usage(void)
{
	printf("\nUsage: %s -- -d <device> -- --chid <num> -f <num> -r <num> --deqtcore <num>\n", g_cmdname);
	printf("  -d <device> : device file name. (e.g., -d /dev/xpcie_<serial_id>)\n");
	printf("  --tp <model> : test model.\n");
	printf("                   \"hh\" : %s [HOST->FPGA->HOST]\n", tp_model_name[TP_HOST_HOST]);
	printf("                   \"d2dh_hh\" : %s [D2D-H(HOST->FPGA0->HOST->FPGA1->HOST)]\n", tp_model_name[TP_D2D_H_HOST_HOST]);
	printf("                   \"d2dd_hh\" : %s [D2D-D(HOST->FPGA0->FPGA1->HOST)]\n", tp_model_name[TP_D2D_D_HOST_HOST]);
	printf("  --chid <num> : channel IDs. valid IDs is 0 to %d.\n", CH_NUM_MAX-1);
	printf("                 separate multiple IDs with commas. separate consecutive IDs with a hyphen.\n");
	printf("                 (e.g., --chid 0,3,5-7,9,10,12-15)\n");
	printf("  -f <num> : FPS.\n");
	printf("  -r <num> : number of send frames.\n");
	printf("  --deqtcore <num> : set cpu core id used by dequeue (1-64).\n");
	printf("\n");
	printf(" [Optional]\n");
	printf("  -m : enable tester measurement mode.\n");
	printf("  --ppms : enable ppm file output of send data.\n");
	printf("  --ppmr : enable ppm file output of receive data.\n");
	printf("  -p : enable performance measurement.\n");
	printf("\n");
}

static int32_t check_chid_args(const char *str, uint32_t *ch_id_p)
{
	int64_t val = 0;
	if (stoi(str, &val) < 0) {
		// Error if string cannot be converted to integer
		rslt2file("parse chid args Error: Cannot to convert string \"%s\" to an integer.\n", str);
		return -1;
	} else {
		if (val < 0 || val > CH_NUM_MAX - 1) {
			rslt2file("parse chid args Error: invalid channel id \"%ld\".\n", val);
			return -1;
		}
	}

	*ch_id_p = (uint32_t)val;

	return 0;
}


static int32_t parse_chid_string(char *string)
{
	assert(string != NULL);

	for (size_t i=0; i < CH_NUM_MAX; i++) {
		g_ch_en[i] = false;
	}
	const uint32_t snum = 5;
	char kdiv[CH_NUM_MAX][snum + 1];
	char hdiv[2][snum + 1];
	uint32_t kdivcnt = 0;

	// Separated by comma ","
	char *kdivp = strtok(string, ",");
	while (kdivp != NULL) {
		kdivcnt++;

		if (kdivcnt > CH_NUM_MAX) {
			// error if comma separated value exceeds maximum number of channels
			rslt2file("parse chid args Error: Num of comma separated(%u). > MAX num of channels(%u).\n", kdivcnt, CH_NUM_MAX);
			return -1;
		}

		if (strlen(kdivp) > snum) {
			// Error if number of characters after comma separator exceeds snum value
			rslt2file("parse chid args Error: chid characters \"%s\". > MAX num of characters(%u).\n", kdivp, snum);
			return -1;
		} else {
			// comma delimited string
			strcpy(kdiv[kdivcnt - 1], kdivp);
		}

		kdivp = strtok(NULL, ",");
	}

	for (size_t i=0; i < kdivcnt; i++) {
		// Does it contain the hyphen "-"?
		char *hp = strchr(kdiv[i], '-');
		if (hp != NULL) {
			uint32_t hdivcnt = 0;
			// separated by hyphen "-"
			char *hdivp = strtok(kdiv[i], "-");
			while (hdivp != NULL) {
				hdivcnt++;

				if (hdivcnt > 2) {
					// an error occurs if the number of hyphen separators exceeds 2
					rslt2file("parse chid args Error: Num of hyphen separated(%u). > MAX num of 2.\n", hdivcnt);
					return -1;
				} else {
					// store character string delimited by hyphen
					strcpy(hdiv[hdivcnt - 1], hdivp);
				}

				hdivp = strtok(NULL, "-");
			}

			if (hdivcnt != 2) {
				// The number of hyphen separators is not 2. Error.
				rslt2file("parse chid args Error: Num of hyphen separated(%u). Correct num of 2.\n", hdivcnt);
				return -1;
			}

			// get continuous channel ID specification
			uint32_t chid_pair[2];
			for (size_t j=0; j < 2; j++) {
				if(check_chid_args(hdiv[j], &chid_pair[j]) < 0) {
					return -1;
				}
			}
			uint32_t start_chid = chid_pair[0];
			uint32_t end_chid = chid_pair[1];
			if (start_chid > end_chid) {
				rslt2file("parse chid args Error: invalid channel id range \"%u-%u\".\n", start_chid, end_chid);
				return -1;
			} else {
				for (size_t ch_id = start_chid; ch_id <= end_chid; ch_id++) {
					g_ch_en[ch_id] = true;
				}
			}
		} else {
			// If the hyphen "-" is not included, the channel ID is obtained without change.
			uint32_t ch_id = 0;
			if(check_chid_args(kdiv[i], &ch_id) < 0) {
				return -1;
			} else {
				g_ch_en[ch_id] = true;
			}
		}
	}
	return 0;
}

int32_t parse_app_args_func(int argc, char **argv)
{
	int32_t opt;
	int32_t idx = 0;

	while ((opt = getopt_long(argc, argv, app_short_options, app_long_options, &idx)) != EOF) {
		switch (opt) {
			case 1: {
				char *string = strdup(optarg);
				if (string == NULL) {
					rslt2file("parse app args Error: Cannot allocate memory for chid string.\n");
					return -1;
				} else {
					if (parse_chid_string(string) < 0) {
						rslt2file("parse app args Error: \"--chid %s\".\n", optarg);
						free(string);
						print_usage();
						return -1;
					}
					free(string);
				}
				for (size_t i=0; i < LANE_NUM_MAX; i++) {
					g_ch_num[i] = 0;
					uint32_t lch_idx = i * (CH_NUM_MAX / LANE_NUM_MAX);
					for (size_t j=0; j < (CH_NUM_MAX / LANE_NUM_MAX); j++) {
						uint32_t ch_id = j + lch_idx;
						if (g_ch_en[ch_id])
							g_ch_num[i]++;
					}
				}
				break;
			}
			case 2:
				if (strcmp(optarg, "hh") == 0) {
					g_tp_model = TP_HOST_HOST;
					g_is_send_data = true;
					g_is_receive_data = true;
				} else if (strcmp(optarg, "d2dh_hh") == 0) {
					g_tp_model = TP_D2D_H_HOST_HOST;
					g_is_send_data = true;
					g_is_receive_data = true;
				} else if (strcmp(optarg, "d2dd_hh") == 0) {
					g_tp_model = TP_D2D_D_HOST_HOST;
					g_is_send_data = true;
					g_is_receive_data = true;
				} else {
					rslt2file("parse app args Error: \"--tp %s\".\n", optarg);
					print_usage();
					return -1;
				}
				break;
			case 3:
				g_loglevel = atoi(optarg);
				break;
			case 4:
				g_core = atoi(optarg);
				break;
			case 5:
				g_is_outppm_send_data = true; 
				break;
			case 6:
				g_is_outppm_receive_data = true;
				break;
			case 'f':
				g_fps = atoi(optarg);
				break;
			case 'r':
				g_frame_num = atoi(optarg);
				g_enq_num = atoi(optarg);
				if (g_enq_num > SHMEMALLOC_NUM_MAX) {
					g_shmalloc_num = SHMEMALLOC_NUM_MAX;
				} else {
					g_shmalloc_num = g_enq_num;
				}
				break;
			case 'm':
				g_tester_meas_mode = true;
				break;
			case 'p':
				g_is_performance_meas = true;
				break;
			default:
				rslt2file("parse app args Error: unknown argument.\n");
				print_usage();
				return -1;
		}
	}

	return 0;
}

int32_t check_options(void)
{
	int32_t ret = 0;

	if (g_tp_model == TP_UNKNOWN) {
		rslt2file("option Error: invalid test model.\n");
		ret = -1;
	}

	if (g_frame_num < 1) {
		rslt2file("option Error: invalid frame num(%d).\n", g_frame_num);
		ret = -1;
	}

	if (g_loglevel < LOG_TRACE || g_loglevel > LOG_FORCE) {
		ret = -1;
	}
	logfile(LOG_FORCE, "loglevel= %d\n", g_loglevel);

	if (g_core < 1 || g_core > CORE_NUM_MAX)
		ret = -1;

	g_divide_que.que_num = g_enq_num;
	g_divide_que.que_num_rem = 0;
	g_divide_que.div_num = 1;

	// print argument options
	rslt2file("\nArgument options...\n");

	rslt2file("  Test model: %s\n", tp_model_name[g_tp_model]);

	for (size_t i=0; i < LANE_NUM_MAX; i++) {
		uint32_t lch_idx = i * (CH_NUM_MAX / LANE_NUM_MAX);
		rslt2file("  Num of channels in FPGA lane%zu: %d ", i, g_ch_num[i]);
		if (g_ch_num[i] > 0) {
			bool f = true;
			rslt2file("[");
			for (size_t j=0; j < (CH_NUM_MAX / LANE_NUM_MAX); j++) {
				uint32_t ch_id = j + lch_idx;
				if (g_ch_en[ch_id]) {
					if (f) {
						rslt2file("CH%zu", ch_id);
						f = false;
					} else {
						rslt2file(",CH%zu", ch_id);
					}
				}
			}
			rslt2file("]");
		}
		rslt2file("\n");
	}

	rslt2file("  Num of frames: %d\n", g_frame_num);

	if (g_fps > 0) {
		rslt2file("  Tester FPS: %d\n", g_fps);
	} else {
		rslt2file("  Tester FPS: none\n");
	}

	rslt2file("  Dequeue thread's CPU core No.: ");
	bool f = true;
	for (size_t i=0; i < CH_NUM_MAX; i++) {
		uint32_t ch_id = i;
		if (g_ch_en[ch_id]) {
			if (f) {
				rslt2file("%d", g_core + ch_id);
				f = false;
			} else {
				rslt2file(",%d", g_core + ch_id);
			}
		}
	}
	rslt2file("\n");

	rslt2file("  enable tester measurement mode: %s\n", bool2string(g_tester_meas_mode));

	rslt2file("  enable ppm file output of send data: %s\n", bool2string(g_is_outppm_send_data));
	rslt2file("  enable ppm file output of receive data: %s\n", bool2string(g_is_outppm_receive_data));
	rslt2file("  enable performance measurement: %s\n", bool2string(g_is_performance_meas));

	rslt2file("\n");

	if (ret < 0) {
		print_usage();
	}

	return ret;
}

void set_cmdname(const char *cmd)
{
	if (cmd == NULL || cmd == "") {
		printf("set cmdname failed.\n");
	} else {
		g_cmdname = cmd;
	}
}

tp_model_t getopt_tp_model(void)
{
	return g_tp_model;
}
bool getopt_ch_en(uint32_t i)
{
	return g_ch_en[i];
}
uint32_t getopt_ch_num(uint32_t i)
{
	return g_ch_num[i];
}
uint32_t getopt_fps(void)
{
	return g_fps;
}
uint32_t getopt_frame_num(void)
{
	return g_frame_num;
}
uint32_t getopt_enq_num(void)
{
	return g_enq_num;
}
uint32_t getopt_shmalloc_num(void)
{
	return g_shmalloc_num;
}
bool getopt_is_send_data(void)
{
	return g_is_send_data;
}
bool getopt_is_receive_data(void)
{
	return g_is_receive_data;
}
bool getopt_is_outppm_send_data(void)
{
	return g_is_outppm_send_data ;
}
bool getopt_is_outppm_receive_data(void)
{
	return g_is_outppm_receive_data ;
}
bool getopt_tester_meas_mode(void)
{
	return g_tester_meas_mode;
}
bool getopt_is_performance_meas(void)
{
	return g_is_performance_meas;
}

//dequeue Thread Core No
uint32_t getopt_core(void)
{
	return g_core;
}

//for debugging
int32_t getopt_loglevel(void)
{
	return g_loglevel;
}

//--------------------------
//  allocate shared memory
//--------------------------
static inline uint32_t getbufsize_d2d(uint32_t size)
{
	//D2D buffer size constraint: 4K * 2^N
	uint32_t bufsize = (size + (DATA_SIZE_4KB - 1)) & ~(DATA_SIZE_4KB - 1); //4KB alignment
	bufsize = nextPow2(bufsize); //rounds up to the nearest power of 2

	return bufsize;
}

int32_t shmem_malloc(shmem_mode_t mode, mngque_t* p, uint32_t ch_id)
{
	logfile(LOG_DEBUG, "CH(%d) shmem_malloc..(%p)\n", ch_id, p);

	if (p == NULL) {
		logfile(LOG_ERROR, " input error shmem_malloc..nil(%p)\n", p);
		return -1;
	}

	uint32_t enq_num = getopt_enq_num();
	uint32_t shmalloc_num = getopt_shmalloc_num();
	uint32_t *in_dev_id = get_dev_id(0);
	uint32_t in_index = dev_id_to_index(*in_dev_id);
	uint32_t imgsize_src = getparam_frame_height_in(in_index, ch_id) * getparam_frame_width_in(in_index, ch_id) * 3;
	uint32_t *out_dev_id = get_dev_id(fpga_get_num() - 1);
	uint32_t out_index = dev_id_to_index(*out_dev_id);
	uint32_t imgsize_dst1 = getparam_frame_height_out(out_index, ch_id) * getparam_frame_width_out(out_index, ch_id) * 3;
	uint32_t imgsize_dst2 = getparam_frame_height_out(out_index, ch_id) * getparam_frame_width_out(out_index, ch_id) * 3;
	uint32_t headsize = sizeof(frameheader_t);
	uint32_t bufsize_src = imgsize_src + headsize;
	uint32_t bufsize_dst1 = imgsize_dst1 + headsize;
	uint32_t bufsize_dst2 = imgsize_dst2 + headsize;
	uint32_t bufsize_d2d = 8 * DATA_SIZE_1KB * DATA_SIZE_1KB; //D2D buffer size fixed at 8 MB

	//alloc mem for queue
	logfile(LOG_DEBUG, "--- shmem alloc ---\n");
	uint32_t ss = 0;
	uint32_t ss_d2d = 0;
	p->enq_num = enq_num;
	p->srcdsize = 0;
	p->dst1dsize = 0;
	p->dst2dsize = 0;
	p->d2ddsize = 0;
	if (mode == SHMEM_SRC) {
		logfile(LOG_DEBUG, " enq_num(%d), headsize(%d), imgsize_src(%d), bufsize_src(%d), shmalloc_num(%d)\n", enq_num, headsize, imgsize_src, bufsize_src, shmalloc_num);
		p->srcdsize = bufsize_src;
		ss = bufsize_src + 0x10000;                //extra for 4KB alignment
	} else if (mode == SHMEM_D2D_SRC) {
		logfile(LOG_DEBUG, " enq_num(%d), headsize(%d), imgsize_src(%d), bufsize_src(%d), shmalloc_num(%d), bufsize_d2d(%d)\n", enq_num, headsize, imgsize_src, bufsize_src, shmalloc_num, bufsize_d2d);
		p->srcdsize = bufsize_src;
		p->d2ddsize = bufsize_dst1;
		ss = bufsize_src + 0x10000;                //extra for 4KB alignment
		ss_d2d = bufsize_d2d;
	} else if (mode == SHMEM_DST) {
		logfile(LOG_DEBUG, " enq_num(%d), headsize(%d), imgsize_dst1(%d), bufsize_dst1(%d), shmalloc_num(%d)\n", enq_num, headsize, imgsize_dst1, bufsize_dst1, shmalloc_num);
		p->dst1dsize = bufsize_dst1;
		ss = bufsize_dst1  + 0x10000;              //extra for 4KB alignment
	} else if (mode == SHMEM_D2D_DST) {
		logfile(LOG_DEBUG, " enq_num(%d), headsize(%d), imgsize_dst1(%d), bufsize_dst1(%d), shmalloc_num(%d), bufsize_d2d(%d)\n", enq_num, headsize, imgsize_dst1, bufsize_dst1, shmalloc_num, bufsize_d2d);
		p->dst1dsize = bufsize_dst1;
		p->d2ddsize = bufsize_dst1;
		ss = bufsize_dst1  + 0x10000;              //extra for 4KB alignment
		ss_d2d = bufsize_d2d;
	} else if (mode == SHMEM_SRC_DST) {
		logfile(LOG_DEBUG, " enq_num(%d), headsize(%d), imgsize_src(%d), bufsize_src(%d), imgsize_dst1(%d), bufsize_dst1(%d), shmalloc_num(%d)\n", enq_num, headsize, imgsize_src, bufsize_src, imgsize_dst1, bufsize_dst1, shmalloc_num);
		p->srcdsize = bufsize_src;
		p->dst1dsize = bufsize_dst1;
		//ss = bufsize_src + bufsize_dst1;         //buf(src->dst1)
		ss = bufsize_src + bufsize_dst1 + 0x10000; //extra for 4KB alignment
	} else if (mode == SHMEM_D2D_SRC_DST) {
		logfile(LOG_DEBUG, " enq_num(%d), headsize(%d), imgsize_src(%d), bufsize_src(%d), imgsize_dst1(%d), bufsize_dst1(%d), shmalloc_num(%d), bufsize_d2d(%d)\n", enq_num, headsize, imgsize_src, bufsize_src, imgsize_dst1, bufsize_dst1, shmalloc_num, bufsize_d2d);
		p->srcdsize = bufsize_src;
		p->dst1dsize = bufsize_dst1;
		p->d2ddsize = bufsize_dst1;
		//ss = bufsize_src + bufsize_dst1;         //buf(src->dst1)
		ss = bufsize_src + bufsize_dst1 + 0x10000; //extra for 4KB alignment
		ss_d2d = bufsize_d2d;
	} else if (mode == SHMEM_DST1_DST2) {
		logfile(LOG_DEBUG, " enq_num(%d), headsize(%d), imgsize_dst1(%d), bufsize_dst1(%d), imgsize_dst2(%d), bufsize_dst2(%d), shmalloc_num(%d)\n", enq_num, headsize, imgsize_dst1,  bufsize_dst1, imgsize_dst2, bufsize_dst2, shmalloc_num);
		p->dst1dsize = bufsize_dst1;
		p->dst2dsize = bufsize_dst2;
		ss = bufsize_dst1 + bufsize_dst2 + 0x10000; //extra for 4KB alignment
	} else if (mode == SHMEM_D2D) {
		logfile(LOG_DEBUG, " enq_num(%d), headsize(%d), imgsize_dst1(%d), bufsize_dst1(%d), bufsize_d2d(%d)\n", enq_num, headsize, imgsize_dst1, bufsize_dst1, bufsize_d2d);
		p->d2ddsize = bufsize_dst1;
		ss_d2d = bufsize_d2d;
	} else {
		logfile(LOG_DEBUG, " enq_num(%d), headsize(%d), imgsize_src(%d), bufsize_src(%d), imgsize_dst1(%d), bufsize_dst1(%d), imgsize_dst2(%d), bufsize_dst2(%d), shmalloc_num(%d)\n", enq_num, headsize, imgsize_src, bufsize_src, imgsize_dst1,  bufsize_dst1, imgsize_dst2, bufsize_dst2, shmalloc_num);
		p->srcdsize = bufsize_src;
		p->dst1dsize = bufsize_dst1;
		p->dst2dsize = bufsize_dst2;
		ss = bufsize_src + bufsize_dst1 + bufsize_dst2 + 0x10000; //extra for 4KB alignment
	}

	memset(&gmm[ch_id][0], 0, sizeof(gmm[ch_id]));
	logfile(LOG_DEBUG, "alloc..\n");
 	for (size_t i=0; i < shmalloc_num; i++) {
		if (ss != 0) {
			logfile(LOG_DEBUG, "shmem alloc..(%d)\n",ss);
			gmm[ch_id][i] = fpga_shmem_alloc(ss);
			if (gmm[ch_id][i] == NULL) {
				logfile(LOG_ERROR, "shmemlloc error(%d)!\n",i);
				p->enqbuf[i].srcbufp = NULL;
				p->enqbuf[i].dst1bufp = NULL;
				p->enqbuf[i].dst2bufp = NULL;
				return -1;
			}

			if (mode == SHMEM_SRC || mode == SHMEM_D2D_SRC) {
				p->enqbuf[i].srcbufp = (void*)(((uint64_t)gmm[ch_id][i] & ~0xfff)+0x1000); // debug
				p->enqbuf[i].dst1bufp = NULL;
				p->enqbuf[i].dst2bufp = NULL;
			} else if (mode == SHMEM_DST || mode == SHMEM_D2D_DST) {
				p->enqbuf[i].srcbufp = NULL;
				p->enqbuf[i].dst1bufp = (void*)(((uint64_t)gmm[ch_id][i] & ~0xfff)+0x1000); // debug
				p->enqbuf[i].dst2bufp = NULL;
			} else if (mode == SHMEM_SRC_DST || mode == SHMEM_D2D_SRC_DST) {
				p->enqbuf[i].srcbufp = (void*)(((uint64_t)gmm[ch_id][i] & ~0xfff)+0x1000); // debug
				void* dst1bufp = (void*)((uint64_t)p->enqbuf[i].srcbufp + (uint64_t)bufsize_src); // dst1bufp start address
				p->enqbuf[i].dst1bufp = (void*)(((uint64_t)dst1bufp + (SHMEM_BOUNDARY_SIZE - 1)) & ~(SHMEM_BOUNDARY_SIZE - 1)); // dst1bufp top address incremented by 1024
				p->enqbuf[i].dst2bufp = NULL;
			} else if (mode == SHMEM_DST1_DST2) {
				p->enqbuf[i].srcbufp = NULL;
				p->enqbuf[i].dst1bufp = (void*)(((uint64_t)gmm[ch_id][i] & ~0xfff)+0x1000); // debug
				void* dst2bufp = (void*)((uint64_t)p->enqbuf[i].dst1bufp + (uint64_t)bufsize_dst1); // dst2bufp start address
				p->enqbuf[i].dst2bufp = (void*)(((uint64_t)dst2bufp + (SHMEM_BOUNDARY_SIZE - 1)) & ~(SHMEM_BOUNDARY_SIZE - 1)); // dst1bufp top address incremented by 1024
			} else {
				p->enqbuf[i].srcbufp = (void*)(((uint64_t)gmm[ch_id][i] & ~0xfff)+0x1000); // debug
				void* dst1bufp = (void*)((uint64_t)p->enqbuf[i].srcbufp + (uint64_t)bufsize_src); // dst1bufp start address
				p->enqbuf[i].dst1bufp = (void*)(((uint64_t)dst1bufp + (SHMEM_BOUNDARY_SIZE - 1)) & ~(SHMEM_BOUNDARY_SIZE - 1)); // dst1bufp top address incremented by 1024
				void* dst2bufp = (void*)((uint64_t)p->enqbuf[i].dst1bufp + (uint64_t)bufsize_dst1); // dst2bufp start address
				p->enqbuf[i].dst2bufp = (void*)(((uint64_t)dst2bufp + (SHMEM_BOUNDARY_SIZE - 1)) & ~(SHMEM_BOUNDARY_SIZE - 1)); // dst1bufp top address incremented by 1024
			}

			logfile(LOG_DEBUG, "srcbufp(%p), dst1bufp(%p), dst2bufp(%p)\n", p->enqbuf[i].srcbufp, p->enqbuf[i].dst1bufp, p->enqbuf[i].dst2bufp);
		} else {
			p->enqbuf[i].srcbufp = NULL;
			p->enqbuf[i].dst1bufp = NULL;
			p->enqbuf[i].dst2bufp = NULL;
		}
	}

	//alloc mem for D2D
	memset(&gmmd2d[ch_id], 0, sizeof(gmmd2d[ch_id]));
	if (ss_d2d != 0) {
		logfile(LOG_DEBUG, "shmem d2d alloc..(%d)\n",ss_d2d);
		gmmd2d[ch_id] = fpga_shmem_aligned_alloc(ss_d2d);
		if (gmmd2d[ch_id] == NULL) {
			logfile(LOG_ERROR, "shmemlloc d2d error!\n");
			p->d2dbufp = NULL;
			return -1;
		}
		p->d2dbufp = gmmd2d[ch_id];
		p->d2dbuflen = bufsize_d2d;

		logfile(LOG_DEBUG, "d2dbufp(%p)\n", p->d2dbufp);
	} else {
		p->d2dbufp = NULL;
		p->d2dbuflen = 0;
	}

	//initialize data memory area
	for (size_t i=0; i < shmalloc_num; i++) {
		if (p->enqbuf[i].srcbufp != NULL) {
			init_data((uint8_t*)p->enqbuf[i].srcbufp, bufsize_src, 1); //0xff
		}
		if (p->enqbuf[i].dst1bufp != NULL) {
			init_data((uint8_t*)p->enqbuf[i].dst1bufp, bufsize_dst1, 1); //0xff
		}
		if (p->enqbuf[i].dst2bufp != NULL) {
			init_data((uint8_t*)p->enqbuf[i].dst2bufp, bufsize_dst2, 1); //0xff
		}
	}
	if (p->d2dbufp != NULL) {
		init_data((uint8_t*)p->d2dbufp, bufsize_d2d, 0); //0x00
	}

	if (p->srcdsize != 0) {
		if (bufsize_src < DATA_SIZE_1KB) {
			// enqueue src_len header + payload set to 1KB if less than 1KB
			p->srcbuflen = DATA_SIZE_1KB;
		} else {
			// enqueue src_len alignment(ALIGN_SRC_LEN byte)
			p->srcbuflen = (bufsize_src + (ALIGN_SRC_LEN - 1)) & ~(ALIGN_SRC_LEN - 1);
		}
	}
	if (p->dst1dsize != 0) {
		if (bufsize_dst1 < DATA_SIZE_1KB) {
			// set 1KB if the dequeue dst_len header + payload is less than 1KB
			p->dst1buflen = DATA_SIZE_1KB;
		} else {
			// dequeue dst_len alignment(ALIGN_DST_LEN byte)
			p->dst1buflen = (bufsize_dst1 + (ALIGN_DST_LEN - 1)) & ~(ALIGN_DST_LEN - 1);
		}
	}
	if (p->dst2dsize != 0) {
		if (bufsize_dst2 < DATA_SIZE_1KB) {
			// set 1KB if the dequeue dst_len header + payload is less than 1KB
			p->dst2buflen = DATA_SIZE_1KB;
		} else {
			// dequeue dst_len alignment(ALIGN_DST_LEN byte)
			p->dst2buflen = (bufsize_dst2 + (ALIGN_DST_LEN - 1)) & ~(ALIGN_DST_LEN - 1);
		}
	}

	return 0;
}

int32_t shmem_free(const mngque_t* p, uint32_t ch_id)
{
	logfile(LOG_DEBUG, "CH(%u) shmem_free...\n", ch_id);

	uint32_t shmalloc_num = getopt_shmalloc_num();

	if (p != NULL) {
		logfile(LOG_DEBUG, "shmem_free(%p)\n", p);
		for (size_t i=0; i < shmalloc_num; i++) {
			if (p->enqbuf[i].srcbufp) {
				logfile(LOG_DEBUG, "shmemfree..(%p)\n",p->enqbuf[i].srcbufp);
				fpga_shmem_free((void*)gmm[ch_id][i]);
			} else if (p->enqbuf[i].dst1bufp) {
				logfile(LOG_DEBUG, "shmemfree..(%p)\n",p->enqbuf[i].dst1bufp);
				fpga_shmem_free((void*)gmm[ch_id][i]);
			}
		}

		if (p->d2dbufp) {
			logfile(LOG_DEBUG, "shmemfree d2d..(%p)\n",p->d2dbufp);
			fpga_shmem_free((void*)gmmd2d[ch_id]);
		}
	}

	return 0;
}

int32_t deq_shmem_free(const mngque_t* p, uint32_t ch_id)
{
	logfile(LOG_DEBUG, "CH(%u) deq_shmem_free...\n", ch_id);

	uint32_t shmalloc_num = getopt_shmalloc_num();

	if (p != NULL) {
		logfile(LOG_DEBUG, "deq_shmem_free(%p)\n", p);
		for (size_t i=0; i < shmalloc_num; i++) {
			if (p->enqbuf[i].dst1bufp) {
				logfile(LOG_DEBUG, "shmemfree..(%p)\n",p->enqbuf[i].dst1bufp);
				fpga_shmem_free((void*)gmm[ch_id][i]);
			}
		}
	}

	return 0;
}

bool* get_deq_shmstate(uint32_t ch_id)
{
	bool *p = &gdeqshmstate[ch_id][0];

	return p;
}

int64_t* get_deq_receivep(uint32_t ch_id)
{
	int64_t *p = &gdeqreceivep[ch_id];

	return p;
}

//--------------------------
// device id
//--------------------------
static uint32_t dev_id_list[FPGA_MAX_DEVICES];
static bool set_dev_id_state = false;

int32_t set_dev_id_list()
{
	int32_t ret = 0;

	// get device list
	char **device_list;
	ret = fpga_get_device_list(&device_list);
	if (ret < 0) {
		rslt2file("fpga_get_device_list error!!\n");
		logfile(LOG_ERROR, "fpga_get_device_list:ret(%d) error!!\n", ret);
		return -1;
	}
	logfile(LOG_DEBUG, "fpga_get_device_list:ret(%d)\n", ret);

	// get dev_id from device list
	for (size_t i=0; i < fpga_get_num(); i++ ) {
		char str[64];
		sprintf(str, "%s%s", FPGA_DEVICE_PREFIX, device_list[i]);
		ret = fpga_get_dev_id(str, &dev_id_list[i]);
		if (ret < 0) {
			rslt2file("fpga_get_dev_id error!!\n");
			logfile(LOG_ERROR, "fpga_get_dev_id:ret(%d) error!!\n", ret);
			return -1;
		}
		logfile(LOG_DEBUG, "fpga_get_dev_id:ret(%d)\n", ret);
		logfile(LOG_DEBUG, "  %s dev_id(%u)\n", str, dev_id_list[i]);
	}
	set_dev_id_state = true;

	// release device list
	ret = fpga_release_device_list(device_list);
	if (ret < 0) {
		rslt2file("fpga_release_device_list error!!\n");
		logfile(LOG_ERROR, "fpga_release_device_list:ret(%d) error!!\n", ret);
		return -1;
	}
	logfile(LOG_DEBUG, "fpga_release_device_list:ret(%d)\n", ret);

	return 0;
}

uint32_t* get_dev_id(uint32_t index)
{
	assert(set_dev_id_state);

	uint32_t *p = &dev_id_list[index];

	return p;
}

uint32_t dev_id_to_index(uint32_t dev_id)
{
	assert(set_dev_id_state);

	uint32_t index = 0;

	for (size_t i=0; i < fpga_get_num(); i++) {
		if (dev_id == dev_id_list[i]) {
			index = i;
			break;
		}
	}

	return index;
}

//--------------------------
// D2D FPGA connect info
//--------------------------
static fpga_lldma_connect_t connectinfo[CH_NUM_MAX];

fpga_lldma_connect_t* get_connectinfo(uint32_t ch_id)
{
	fpga_lldma_connect_t* p;

	p = &connectinfo[ch_id];

	return p;
}

//--------------------------
// queue info
//--------------------------
static dma_info_t enqdmainfo_channel[FPGA_MAX_DEVICES][CH_NUM_MAX];
static dma_info_t enqdmainfo[FPGA_MAX_DEVICES][CH_NUM_MAX];
static dma_info_t deqdmainfo_channel[FPGA_MAX_DEVICES][CH_NUM_MAX];
static dma_info_t deqdmainfo[FPGA_MAX_DEVICES][CH_NUM_MAX];
static dmacmd_info_t **enqdmacmdinfo = NULL;
static dmacmd_info_t **deqdmacmdinfo = NULL;

int32_t dmacmdinfo_malloc()
{
	logfile(LOG_DEBUG, "dmacmdinfo_malloc...\n");

	uint32_t enq_num = getopt_enq_num();

	// for enqueue
	enqdmacmdinfo = (dmacmd_info_t**)malloc(sizeof(dmacmd_info_t) * CH_NUM_MAX);
	if (enqdmacmdinfo == NULL) {
		logfile(LOG_ERROR, "enqdmacmdinfo malloc error!\n");
		return -1;
	}
	for (size_t i=0; i < CH_NUM_MAX; i++) {
		enqdmacmdinfo[i] = (dmacmd_info_t*)malloc(sizeof(dmacmd_info_t) * enq_num);
		if (enqdmacmdinfo[i] == NULL) {
			logfile(LOG_ERROR, "enqdmacmdinfo[%zu] malloc error!\n", i);
			return -1;
		}
	}
	logfile(LOG_DEBUG, "  enqdmacmdinfo malloc(%p)\n", enqdmacmdinfo);

	// for dequeue
	deqdmacmdinfo = (dmacmd_info_t**)malloc(sizeof(dmacmd_info_t) * CH_NUM_MAX);
	if (deqdmacmdinfo == NULL) {
		logfile(LOG_ERROR, "deqdmacmdinfo malloc error!\n");
		return -1;
	}
	for (size_t i=0; i < CH_NUM_MAX; i++) {
		deqdmacmdinfo[i] = (dmacmd_info_t*)malloc(sizeof(dmacmd_info_t) * enq_num);
		if (deqdmacmdinfo[i] == NULL) {
			logfile(LOG_ERROR, "deqdmacmdinfo[%zu] malloc error!\n", i);
			return -1;
		}
	}
	logfile(LOG_DEBUG, "  deqdmacmdinfo malloc(%p)\n", deqdmacmdinfo);

	return 0;
}

void dmacmdinfo_free()
{
	logfile(LOG_DEBUG, "dmacmdinfo_free...\n");

	// for enqueue
	if (enqdmacmdinfo != NULL) {
		logfile(LOG_DEBUG, "  enqdmacmdinfo free(%p)\n", enqdmacmdinfo);
		free(enqdmacmdinfo);
	} else {
		logfile(LOG_ERROR, "  enqdmacmdinfo buffer is NULL!\n");
	}

	// for dequeue
	if (deqdmacmdinfo != NULL) {
		logfile(LOG_DEBUG, "  deqdmacmdinfo free(%p)\n", deqdmacmdinfo);
		free(deqdmacmdinfo);
	} else {
		logfile(LOG_ERROR, "  deqdmacmdinfo buffer is NULL!\n");
	}
}

dma_info_t* get_enqdmainfo_channel(uint32_t dev_id, uint32_t ch_id)
{
	dma_info_t* p;

	p = &enqdmainfo_channel[dev_id][ch_id];

	return p;
}

dma_info_t* get_deqdmainfo_channel(uint32_t dev_id, uint32_t ch_id)
{
	dma_info_t* p;

	p = &deqdmainfo_channel[dev_id][ch_id];

	return p;
}

dma_info_t* get_enqdmainfo(uint32_t dev_id, uint32_t ch_id)
{
	dma_info_t* p;

	p = &enqdmainfo[dev_id][ch_id];

	return p;
}

dma_info_t* get_deqdmainfo(uint32_t dev_id, uint32_t ch_id)
{
	dma_info_t* p;

	p = &deqdmainfo[dev_id][ch_id];

	return p;
}

dmacmd_info_t* get_enqdmacmdinfo(uint32_t ch_id, uint32_t enq_id)
{
	dmacmd_info_t* p;

	p = &enqdmacmdinfo[ch_id][enq_id];

	return p;
}

dmacmd_info_t* get_deqdmacmdinfo(uint32_t ch_id, uint32_t enq_id)
{
	dmacmd_info_t* p;

	p = &deqdmacmdinfo[ch_id][enq_id];

	return p;
}

const divide_que_t* get_divide_que(void)
{
	divide_que_t* p;

	p = &g_divide_que;

	return p;
}

//--------------------------
// kernel id
//--------------------------
uint32_t get_chain_krnl_id(uint32_t ch_id)
{
	uint32_t ch_div_unit = CH_NUM_MAX / CHAIN_KRNL_NUM_MAX;
	uint32_t chain_krnl_id = ch_id / ch_div_unit;

	return chain_krnl_id;
}

uint32_t get_conv_krnl_id(uint32_t ch_id)
{
	uint32_t ch_div_unit = CH_NUM_MAX / CONV_KRNL_NUM_MAX;
	uint32_t conv_krnl_id = ch_id / ch_div_unit;

	return conv_krnl_id;
}

uint32_t get_function_krnl_id(uint32_t ch_id)
{
	uint32_t ch_div_unit = CH_NUM_MAX / FUNCTION_KRNL_NUM_MAX;
	uint32_t function_krnl_id = ch_id / ch_div_unit;

	return function_krnl_id;
}

//-----------------------------------------
// movie file
//-----------------------------------------
int32_t open_moviefile(uint32_t ch_id)
{
	const char *moviefile = getparam_moviefile(ch_id);
	logfile(LOG_DEBUG, "CH(%u) movie2cap : movie file (%s)\n", ch_id, moviefile);
	int32_t ret = movie2cap(moviefile, ch_id);
	if (ret < 0) {
		logfile(LOG_ERROR, "  failed to open movie file (%s)!\n", ch_id, moviefile);
		rslt2file("failed to open movie file (%s)!\n", ch_id, moviefile);
		return -1;
	}

	return 0;
}

//-----------------------------------------
// send data
//-----------------------------------------
// sendimg
int32_t sendimg_malloc(uint32_t ch_id)
{
	logfile(LOG_DEBUG, "CH(%u) sendimg_malloc...\n", ch_id);

	size_t gen_frame_num = getopt_frame_num();
	if (getopt_tester_meas_mode()) {
		gen_frame_num = 1;
	}

	uint32_t *dev_id = get_dev_id(0);
	uint32_t index = dev_id_to_index(*dev_id);
	size_t height = getparam_frame_height_in(index, ch_id);
	size_t width = getparam_frame_width_in(index, ch_id);
	size_t dsize = height * width * 3;
	uint8_t *img = (uint8_t*)malloc(sizeof(uint8_t) * dsize * gen_frame_num);
	if (img == NULL) {
		logfile(LOG_ERROR, "  sendimg malloc error!\n");
		return -1;
	}
	init_data(img, sizeof(uint8_t) * dsize * gen_frame_num, 0); //0x00

	gsendimg[ch_id] = img;
	logfile(LOG_DEBUG, "  sendimg malloc(%p)\n", gsendimg[ch_id]);

	return 0;
}

void* get_sendimg_addr(uint32_t ch_id)
{
	void *p = gsendimg[ch_id];

	return p;
}

void sendimg_free(uint32_t ch_id)
{
	logfile(LOG_DEBUG, "CH(%u) sendimg_free...\n", ch_id);

	if (gsendimg[ch_id] != NULL) {
		logfile(LOG_DEBUG, "  sendimg free(%p)\n", gsendimg[ch_id]);
		free(gsendimg[ch_id]);
	} else {
		logfile(LOG_ERROR, "  sendimg buffer is NULL!\n");
	}
}

//-----------------------------------------
// receive data
//-----------------------------------------
// receiveheader
int32_t receiveheader_malloc(uint32_t ch_id)
{
	logfile(LOG_DEBUG, "CH(%u) receiveheader_malloc...\n", ch_id);

	size_t frame_num = getopt_frame_num();

	frameheader_t *h = (frameheader_t*)malloc(sizeof(frameheader_t) * frame_num);
	if (h == NULL) {
		logfile(LOG_ERROR, "  receiveheader malloc error!\n");
		return -1;
	}
	init_data((uint8_t*)h, sizeof(frameheader_t) * frame_num, 1); //0xff

	greceiveheader[ch_id] = (void*)h;
	logfile(LOG_DEBUG, "  receiveheader malloc(%p)\n", greceiveheader[ch_id]);

	return 0;
}

void* get_receiveheader_addr(uint32_t ch_id)
{
	void *p = greceiveheader[ch_id];

	return p;
}

void receiveheader_free(uint32_t ch_id)
{
	logfile(LOG_DEBUG, "CH(%u) receiveheader_free...\n", ch_id);

	if (greceiveheader[ch_id] != NULL) {
		logfile(LOG_DEBUG, "  receiveheader free(%p)\n", greceiveheader[ch_id]);
		free(greceiveheader[ch_id]);
	} else {
		logfile(LOG_ERROR, "  receiveheader buffer is NULL!\n");
	}
}

// receiveimg
int32_t receiveimg_malloc(uint32_t ch_id)
{
	logfile(LOG_DEBUG, "CH(%u) receiveimg_malloc...\n", ch_id);

	size_t frame_num = getopt_frame_num();
	if (frame_num > DUMP_PPM_NUM_MAX) {
		frame_num = DUMP_PPM_NUM_MAX;
	}
	uint32_t *dev_id = get_dev_id(fpga_get_num() - 1);
	uint32_t index = dev_id_to_index(*dev_id);
	size_t height = getparam_frame_height_in(index, ch_id);
	size_t width = getparam_frame_width_in(index, ch_id);
	size_t dsize = height * width * 3;
	uint8_t *img = (uint8_t*)malloc(sizeof(uint8_t) * dsize * frame_num);
	if (img == NULL) {
		logfile(LOG_ERROR, "  receiveimg malloc error!\n");
		return -1;
	}
	init_data(img, sizeof(uint8_t) * dsize * frame_num, 1); //0xff

	greceiveimg[ch_id] = img;
	logfile(LOG_DEBUG, "  receiveimg malloc(%p)\n", greceiveimg[ch_id]);

	return 0;
}

uint8_t* get_receiveimg_addr(uint32_t ch_id)
{
	uint8_t *p = greceiveimg[ch_id];

	return p;
}

void receiveimg_free(uint32_t ch_id)
{
	logfile(LOG_DEBUG, "CH(%u) receiveimg_free...\n", ch_id);

	if (greceiveimg[ch_id] != NULL) {
		logfile(LOG_DEBUG, "  receiveimg free(%p)\n", greceiveimg[ch_id]);
		free(greceiveimg[ch_id]);
	} else {
		logfile(LOG_ERROR, "  receiveimg buffer is NULL!\n");
	}
}

//-----------------------------------------
// for performance measurement
//-----------------------------------------
static timestamp_t **g_timestamp_rx = NULL;
static timestamp_t **g_timestamp_tx = NULL;
static uint64_t **g_timestamp_header_rx = NULL;
static uint64_t **g_timestamp_header_tx = NULL;

//-----------------------------------------
// set send frame
//-----------------------------------------
int32_t set_frame_shmem_src(uint32_t ch_id, uint32_t enq_id)
{
	if (! getopt_is_performance_meas())
		logfile(LOG_DEBUG, "CH(%u) enq(%u) set_frame_shmem_src...\n", ch_id, enq_id);

	dmacmd_info_t *pdmacmdinfo = get_enqdmacmdinfo(ch_id, enq_id);
	if (pdmacmdinfo == NULL) {
		return -1;
	}
	void *data_addr = (void*)pdmacmdinfo->data_addr;

	uint32_t *dev_id = get_dev_id(0);
	uint32_t index = dev_id_to_index(*dev_id);
	size_t height = getparam_frame_height_in(index, ch_id);
	size_t width = getparam_frame_width_in(index, ch_id);
	size_t img_len = height * width * 3;
	size_t head_len = sizeof(frameheader_t);
	
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	uint64_t timestamp_nsec = (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
	//----------------------------------------------
	// frameheader
	//----------------------------------------------
	frameheader_t fh;
	
	fh.marker       = 0xE0FF10AD;
	fh.payload_len  = img_len;
	fh.frame_index  = enq_id + 1;
	fh.local_ts     = timestamp_nsec;
	fh.channel_id   = ch_id;
	fh.h_checksum   = 0x00000000;//future function
	memset(&fh.reserved1, 0x00, sizeof(uint8_t) * 4);
	memset(&fh.reserved2, 0x00, sizeof(uint8_t) * 8);
	memset(&fh.reserved3, 0x00, sizeof(uint8_t) * 8);
	memset(&fh.reserved4, 0x00, sizeof(uint8_t) * 2);
	// shared memory frame header area
	void *head_addr = data_addr;
	// set frame header in shared memory
	memcpy(head_addr, &fh, head_len);

	//----------------------------------------------
	// imagedata
	//----------------------------------------------
	// input image data
	uint8_t *sa = get_sendimg_addr(ch_id);
	uint8_t *simg_addr = sa;
	if (! getopt_tester_meas_mode() ) {
		simg_addr = sa + img_len * enq_id;
	}
	// shared memory payload area
	void *payload_addr = data_addr + head_len;
	// set input image in shared memory
	memcpy(payload_addr, simg_addr, img_len);

	return 0;
}

//-----------------------------------------
// Debug PPM Output
//-----------------------------------------
int32_t outppm_send_data(uint32_t ch_id, uint32_t enq_id)
{
	int32_t ret = 0;

	logfile(LOG_DEBUG, "outppm_send_data...(%u)\n", enq_id);

	uint32_t total_task = enq_id + 1;

	if (enq_id < DUMP_PPM_NUM_MAX) {
		uint32_t *dev_id = get_dev_id(0);
		uint32_t index = dev_id_to_index(*dev_id);
		size_t height = getparam_frame_height_in(index, ch_id);
		size_t width = getparam_frame_width_in(index, ch_id);

		size_t img_len = height * width * 3;
		uint8_t *img_addr = gsendimg[ch_id];
		if (! getopt_tester_meas_mode ()) {
			img_addr += img_len * enq_id;
		}

		char ppm[64];
		sprintf(ppm, "%s/ch%02u_task%u_send.ppm", SEND_DATA_DIR, ch_id, total_task);
		ret = dump_ppm(img_addr, height, width, ppm, 0);
		if (ret < 0) {
			rslt2file("dump_ppm error: (%s)\n", ppm);
			logfile(LOG_ERROR, "dump_ppm error: (%s)!\n", ppm);
			return -1;
		} else {
			rslt2file("dump ppm -> \"%s\"\n", ppm);
			logfile(LOG_DEBUG, "dump ppm...(%s)\n", ppm);
		}
	} else {
		rslt2file("dump ppm -> CH(%u) TASK(%u) Non-target\n", ch_id, total_task);
		logfile(LOG_DEBUG, "  dump ppm... CH(%u) TASK(%u) Non-target\n", ch_id, total_task);
	}

	return 0;
}

//-----------------------------------------
// Device Information
//-----------------------------------------
void pr_device_info()
{
	rslt2file("FPGA device info\n");
	rslt2file("--------------------------------------------------\n");

	for (size_t i=0; i < fpga_get_num(); i++ ) {
		uint32_t *dev_id = get_dev_id(i);
		fpga_device_user_info_t dev_info;
		logfile(LOG_DEBUG, "dev(%u) fpga_get_device_info\n", *dev_id);
		int32_t ret = fpga_get_device_info(*dev_id, &dev_info);
		if (ret < 0) {
			logfile(LOG_ERROR, "  fpga_get_device_info:ret(%d) error!!\n", ret);
			rslt2file("  Device info Error!!\n");
		} else {
			rslt2file("  Device file path    : %s\n", dev_info.device_file_path);
			rslt2file("  Device vendor       : %s\n", dev_info.vendor);
			rslt2file("  Device index        : %d\n", dev_info.device_index);
			rslt2file("  PCIe bus id         : %04x:%02x:%02x.%01x\n", dev_info.pcie_bus.domain, dev_info.pcie_bus.bus, dev_info.pcie_bus.device, dev_info.pcie_bus.function);
			rslt2file("  Bitstream id parent : 0x%08x\n", dev_info.bitstream_id.parent);
			uint16_t parent_type = dev_info.bitstream_id.parent >> 16;
			uint8_t parent_version = dev_info.bitstream_id.parent >> 8;
			uint8_t parent_revision = dev_info.bitstream_id.parent;
			rslt2file("    FPGA type     : 0x%04x\n", parent_type);
			rslt2file("    FPGA version  : 0x%02x\n", parent_version);
			rslt2file("    FPGA revision : 0x%02x\n", parent_revision);
			rslt2file("  Bitstream id child  : 0x%08x\n", dev_info.bitstream_id.child);
			uint16_t child_type = dev_info.bitstream_id.child >> 16;
			uint8_t child_version = dev_info.bitstream_id.child >> 8;
			uint8_t child_revision = dev_info.bitstream_id.child;
			rslt2file("    FPGA type     : 0x%04x\n", child_type);
			rslt2file("    FPGA version  : 0x%02x\n", child_version);
			rslt2file("    FPGA revision : 0x%02x\n", child_revision);
			rslt2file("  ------------------------------------------------\n");
		}
	}

	rslt2file("\n\n");
}


int32_t timestamp_malloc()
{
	logfile(LOG_DEBUG, "timestamp_malloc...\n");

	uint32_t enq_num = getopt_enq_num();

	// RX
	g_timestamp_rx = (timestamp_t**)malloc(sizeof(timestamp_t) * CH_NUM_MAX);
	if (g_timestamp_rx == NULL) {
		logfile(LOG_ERROR, "g_timestamp_rx malloc error!\n");
		return -1;
	}

	for (size_t i=0; i < CH_NUM_MAX; i++) {
		g_timestamp_rx[i] = (timestamp_t*)malloc(sizeof(timestamp_t) * enq_num);
		if (g_timestamp_rx[i] == NULL) {
			logfile(LOG_ERROR, "g_timestamp_rx[%zu] malloc error!\n", i);
			return -1;
		}
	}
	logfile(LOG_DEBUG, "  g_timestamp_rx malloc(%p)\n", g_timestamp_rx);

	// TX
	g_timestamp_tx = (timestamp_t**)malloc(sizeof(timestamp_t) * CH_NUM_MAX);
	if (g_timestamp_tx == NULL) {
		logfile(LOG_ERROR, "g_timestamp_tx malloc error!\n");
		return -1;
	}
	for (size_t i=0; i < CH_NUM_MAX; i++) {
		g_timestamp_tx[i] = (timestamp_t*)malloc(sizeof(timestamp_t) * enq_num);
		if (g_timestamp_tx[i] == NULL) {
			logfile(LOG_ERROR, "g_timestamp_tx[%zu] malloc error!\n", i);
			return -1;
		}
	}
	logfile(LOG_DEBUG, "  g_timestamp_tx malloc(%p)\n", g_timestamp_tx);

	// header
	g_timestamp_header_rx = (uint64_t**)malloc(sizeof(uint64_t) * CH_NUM_MAX);
	if (g_timestamp_header_rx == NULL) {
		logfile(LOG_ERROR, "g_timestamp_header_rx malloc error!\n");
		return -1;
	}
	for (size_t i=0; i < CH_NUM_MAX; i++) {
		g_timestamp_header_rx[i] = (uint64_t*)malloc(sizeof(uint64_t) * enq_num);
		if (g_timestamp_header_rx[i] == NULL) {
			logfile(LOG_ERROR, "g_timestamp_header_rx[%zu] malloc error!\n", i);
			return -1;
		}
	}
	logfile(LOG_DEBUG, "  g_timestamp_header_rx malloc(%p)\n", g_timestamp_header_rx);

	g_timestamp_header_tx = (uint64_t**)malloc(sizeof(uint64_t) * CH_NUM_MAX);
	if (g_timestamp_header_tx == NULL) {
		logfile(LOG_ERROR, "g_timestamp_header_tx malloc error!\n");
		return -1;
	}
	for (size_t i=0; i < CH_NUM_MAX; i++) {
		g_timestamp_header_tx[i] = (uint64_t*)malloc(sizeof(uint64_t) * enq_num);
		if (g_timestamp_header_tx[i] == NULL) {
			logfile(LOG_ERROR, "g_timestamp_header_tx[%zu] malloc error!\n", i);
			return -1;
		}
	}
	logfile(LOG_DEBUG, "  g_timestamp_header_tx malloc(%p)\n", g_timestamp_header_tx);

	return 0;
}

void timestamp_free()
{
	logfile(LOG_DEBUG, "timestamp_free...\n");

	if (g_timestamp_rx != NULL) {
		logfile(LOG_DEBUG, "  g_timestamp_rx free(%p)\n", g_timestamp_rx);
		free(g_timestamp_rx);
	} else {
		logfile(LOG_ERROR, "  g_timestamp_rx buffer is NULL!\n");
	}

	if (g_timestamp_tx != NULL) {
		logfile(LOG_DEBUG, "  g_timestamp_tx free(%p)\n", g_timestamp_tx);
		free(g_timestamp_tx);
	} else {
		logfile(LOG_ERROR, "  g_timestamp_tx buffer is NULL!\n");
	}
	if (g_timestamp_header_rx != NULL) {
		logfile(LOG_DEBUG, "  g_timestamp_header_rx free(%p)\n", g_timestamp_header_rx);
		free(g_timestamp_header_rx);
	} else {
		logfile(LOG_ERROR, "  g_timestamp_header_rx buffer is NULL!\n");
	}

	if (g_timestamp_header_tx != NULL) {
		logfile(LOG_DEBUG, "  g_timestamp_header_tx free(%p)\n", g_timestamp_header_tx);
		free(g_timestamp_header_tx);
	} else {
		logfile(LOG_ERROR, "  g_timestamp_header_tx buffer is NULL!\n");
	}
}

void timer_header_start(uint32_t ch_id, uint32_t enq_id, uint64_t timestamp)
{
	g_timestamp_header_rx[ch_id][enq_id] = timestamp;
}

void timer_rx_start(uint32_t ch_id, uint32_t enq_id)
{
	struct timespec start_time;
	struct timespec end_time;
	timestamp_t *p = &g_timestamp_rx[ch_id][enq_id];

	memset(&end_time, 0, sizeof(end_time));
	p->end_time = end_time;

	// get time
	clock_gettime(CLOCK_MONOTONIC, &start_time);
	p->start_time = start_time;
}

void timer_rx_stop(uint32_t ch_id, uint32_t enq_id)
{
	struct timespec end_time;
	timestamp_t *p = &g_timestamp_rx[ch_id][enq_id];

	// get time
	clock_gettime(CLOCK_MONOTONIC, &end_time);
	p->end_time = end_time;
}

void timer_tx_start(uint32_t ch_id, uint32_t enq_id)
{
	struct timespec start_time;
	struct timespec end_time;
	timestamp_t *p = &g_timestamp_tx[ch_id][enq_id];

	memset(&end_time, 0, sizeof(end_time));
	p->end_time = end_time;

	// get time
	clock_gettime(CLOCK_MONOTONIC, &start_time);
	p->start_time = start_time;
}

void timer_tx_stop(uint32_t ch_id, uint32_t enq_id)
{
	struct timespec end_time;
	timestamp_t *p = &g_timestamp_tx[ch_id][enq_id];

	// get time
	clock_gettime(CLOCK_MONOTONIC, &end_time);
	p->end_time = end_time;
	g_timestamp_header_tx[ch_id][enq_id] = (uint64_t)end_time.tv_sec*1000000000ULL + (uint64_t)end_time.tv_nsec;

}

void pr_perf_normal(void)
{
	uint32_t total_ch_num = 0;
	for (size_t i=0; i < LANE_NUM_MAX; i++) {
		total_ch_num += getopt_ch_num(i);
	}

	uint64_t all_ch_rx_data_len = 0;
	uint64_t all_ch_tx_data_len = 0;
	double all_ch_tx_throughput = 0.0;
	double all_ch_tx_throughput_f = 0.0;

	rslt2file("\n//////////// Performance ////////////\n");

	uint32_t enq_num = getopt_enq_num();
	// ----------------------------------
	// Calculation of performance measurement results for each CH
	// ----------------------------------
	for (size_t i=0; i < CH_NUM_MAX; i++) {
		uint32_t ch_id = i;
		if (getopt_ch_en(ch_id)) {
			uint64_t rx_data_len = 0;
			uint64_t tx_data_len = 0;
			uint64_t rx_total_data_len = 0;
			uint64_t tx_total_data_len = 0;
			uint64_t rxtx_header_latency_ave = 0;
			uint64_t rxtx_header_latency_total = 0;
			double tx_throughput_alltask = 0.0;
			double tx_throughput_alltask_f = 0.0;
			uint64_t tx_latency_total = 0;
			uint64_t tx_latency_alltask = 0;
			double rxtx_header_latency_ave_ms = 0.0;

			if(getopt_is_send_data() || getopt_is_receive_data()) { // Skip if neither send nor receive
				for (size_t j=0; j < enq_num; j++) {
					uint32_t enq_id = j;

					if (getopt_is_send_data()) {
						dmacmd_info_t *penqdmacmdinfo = get_enqdmacmdinfo(ch_id, enq_id);

						// RX Data size
						rx_data_len = penqdmacmdinfo->data_len;
					}

					if (getopt_is_receive_data()) {
						dmacmd_info_t *pdmadmacmdinfo = get_deqdmacmdinfo(ch_id, enq_id);

						// TX Data size
						tx_data_len = pdmadmacmdinfo->data_len;

						// TX latency per task
						uint64_t tx_latency = 0;
						if (getopt_is_send_data()) {
							// for HOST->FPGA-> HOST
							tx_latency = time_duration(&g_timestamp_rx[ch_id][enq_id].end_time, &g_timestamp_tx[ch_id][enq_id].end_time);
						} else {
							tx_latency = time_duration(&g_timestamp_tx[ch_id][enq_id].start_time, &g_timestamp_tx[ch_id][enq_id].end_time);
						}
						tx_latency_total += tx_latency;
					}
					if (getopt_is_receive_data()) {
						// header timestamp used nsec
						uint64_t rxtx_header_latency = g_timestamp_header_tx[ch_id][enq_id] - g_timestamp_header_rx[ch_id][enq_id];
						rxtx_header_latency_total += rxtx_header_latency;
					}

					// Total Data size of this CH
					rx_total_data_len += rx_data_len;
					tx_total_data_len += tx_data_len;
				}

				// Total Data size of all channels
				all_ch_rx_data_len += rx_total_data_len;
				all_ch_tx_data_len += tx_total_data_len;

				if (getopt_is_receive_data()) {
					// TX latency from first task to last task
					if (getopt_is_send_data()) {
						// for HOST->FPGA-> HOST
						tx_latency_alltask = time_duration(&g_timestamp_rx[ch_id][0].end_time, &g_timestamp_tx[ch_id][enq_num - 1].end_time);
					} else {
						tx_latency_alltask = time_duration(&g_timestamp_tx[ch_id][0].start_time, &g_timestamp_tx[ch_id][enq_num - 1].end_time);
					}

					// TX throughput of first task to last task
					tx_throughput_alltask = (double)tx_total_data_len / ((double)tx_latency_alltask / 1000000000L);
					tx_throughput_alltask_f = (double)enq_num / ((double)tx_latency_alltask / 1000000000L);
				}

				// using header timestamp
				if (getopt_is_receive_data()) {
					// average RX (enqueue)->TX (dequeue) latency nsec
					rxtx_header_latency_ave = rxtx_header_latency_total / enq_num;
				}
				rxtx_header_latency_ave_ms = (double)rxtx_header_latency_ave / 1000000L;

				all_ch_tx_throughput += tx_throughput_alltask;
				all_ch_tx_throughput_f += tx_throughput_alltask_f;
			}
			rslt2file("==========================================================\n");
			rslt2file("CH(%u)\n", ch_id);
			rslt2file("--------------------------------------------------------\n");
			rslt2file("  transfer data size\n");
			if (getopt_is_send_data()) {
				rslt2file("         send : %u [frame] %" PRIu64 " [Byte]\n", enq_num, rx_total_data_len);
			} else {
				rslt2file("         send : - [frame] - [Byte]\n");
			}
			if (getopt_is_receive_data()) {
				rslt2file("         recv : %u [frame] %" PRIu64 " [Byte]\n", enq_num, tx_total_data_len);
				rslt2file("  throughput : %.3lf [fps] %.3lf [bps]\n", tx_throughput_alltask_f, tx_throughput_alltask*8.0);
				rslt2file("  latency = %.6lf [msec]\n", rxtx_header_latency_ave_ms);
			} else {
				rslt2file("         recv : - [frame] - [Byte]\n");
				rslt2file("  throughput : - [fps] - [bps]\n");
				rslt2file("  latency = - [msec]\n");
			}
		}
	}

	rslt2file("==========================================================\n");
	rslt2file("ALL CH TOTAL\n");
	rslt2file("--------------------------------------------------------\n");
	rslt2file("  number of ch : %u\n", total_ch_num);
	rslt2file("  transfer data size\n");
	if (getopt_is_send_data()) {
		rslt2file("         send : %" PRIu64 " [frame] %" PRIu64 " [Byte]\n", enq_num * total_ch_num, all_ch_rx_data_len);
	} else {
		rslt2file("         send : - [frame] - [Byte]\n");
	}
	if (getopt_is_receive_data()) {
		rslt2file("         recv : %" PRIu64 " [frame] %" PRIu64 " [Byte]\n", enq_num * total_ch_num, all_ch_tx_data_len);
		rslt2file("  throughput : %.3lf [fps] %.3lf [bps]\n", all_ch_tx_throughput_f, all_ch_tx_throughput*8.0);
	} else {
		rslt2file("         recv : - [frame] - [Byte]\n");
		rslt2file("  throughput : -[fps] - [bps]\n");
	}

	rslt2file("==========================================================\n");
	rslt2file("ALL CH AVE\n");
	rslt2file("--------------------------------------------------------\n");
	if (getopt_is_receive_data()) {
		double allch_ave_tx_throughput_alltask_f = all_ch_tx_throughput_f / (double)total_ch_num;
		double allch_ave_tx_throughput_alltask = (all_ch_tx_throughput * 8.0) / (double)total_ch_num;
		rslt2file("  throughput : %.3lf [fps] %.3lf [bps]\n", allch_ave_tx_throughput_alltask_f, allch_ave_tx_throughput_alltask);
	} else {
		rslt2file("  throughput : - [fps] - [bps]\n");
	}
	rslt2file("==========================================================\n");
}

void pr_perf()
{
	uint32_t total_ch_num = 0;
	for (size_t i=0; i < LANE_NUM_MAX; i++) {
		total_ch_num += getopt_ch_num(i);
	}
	uint32_t enq_num = getopt_enq_num();

	uint64_t all_ch_rx_data_len = 0;
	uint64_t all_ch_tx_data_len = 0;
	uint64_t all_ch_rxtx_data_len = 0;
	uint64_t all_ch_rx_latency = 0;
	uint64_t all_ch_tx_latency = 0;
	uint64_t all_ch_rxtx_latency = 0;
	double all_ch_rx_throughput = 0.0;
	double all_ch_tx_throughput = 0.0;
	double all_ch_rxtx_throughput = 0.0;

	rslt2file("\n//////////// Performance ////////////\n");

	// ----------------------------------
	// Calculation of performance measurement results for each CH
	// ----------------------------------
	for (size_t i=0; i < CH_NUM_MAX; i++) {
		uint32_t ch_id = i;
		if (getopt_ch_en(ch_id)) {
			uint64_t rx_data_len = 0;
			uint64_t tx_data_len = 0;
			uint64_t rxtx_data_len = 0;
			uint64_t rx_total_data_len = 0;
			uint64_t tx_total_data_len = 0;
			uint64_t rxtx_total_data_len = 0;
			uint64_t rx_latency_max = 0;
			uint64_t rx_latency_min = 0;
			uint64_t rx_latency_ave = 0;
			uint64_t rx_latency_total = 0;
			uint64_t rx_latency_alltask = 0;
			uint64_t rx_latency_per_task = 0;
			uint64_t tx_latency_max = 0;
			uint64_t tx_latency_min = 0;
			uint64_t tx_latency_ave = 0;
			uint64_t tx_latency_total = 0;
			uint64_t tx_latency_alltask = 0;
			uint64_t tx_latency_per_task = 0;
			uint64_t rxtx_latency_max = 0;
			uint64_t rxtx_latency_min = 0;
			uint64_t rxtx_latency_ave = 0;
			uint64_t rxtx_latency_total = 0;
			uint64_t rxtx_latency_alltask = 0;
			uint64_t rxtx_latency_per_task = 0;
			uint64_t rxtx_header_latency_max = 0;
			uint64_t rxtx_header_latency_min = 0;
			uint64_t rxtx_header_latency_ave = 0;
			uint64_t rxtx_header_latency_total = 0;
			double rx_throughput_max = 0.0;
			double rx_throughput_min = 0.0;
			double rx_throughput_ave = 0.0;
			double rx_throughput_alltask = 0.0;
			double rx_throughput_per_task = 0.0;
			double tx_throughput_max = 0.0;
			double tx_throughput_min = 0.0;
			double tx_throughput_ave = 0.0;
			double tx_throughput_alltask = 0.0;
			double tx_throughput_per_task = 0.0;
			double rxtx_throughput_max = 0.0;
			double rxtx_throughput_min = 0.0;
			double rxtx_throughput_ave = 0.0;
			double rxtx_throughput_alltask = 0.0;
			double rxtx_throughput_per_task = 0.0;

			for (size_t j=0; j < enq_num; j++) {
				uint32_t enq_id = j;

				if (getopt_is_send_data()) {
					dmacmd_info_t *penqdmacmdinfo = get_enqdmacmdinfo(ch_id, enq_id);

					// RX Data size
					rx_data_len = penqdmacmdinfo->data_len;

					// RX latency per task
					uint64_t rx_latency = time_duration(&g_timestamp_rx[ch_id][enq_id].start_time, &g_timestamp_rx[ch_id][enq_id].end_time);
					if (enq_id == 0) {
						rx_latency_max = rx_latency;
						rx_latency_min = rx_latency;
					} else {
						if (rx_latency > rx_latency_max)
							rx_latency_max = rx_latency;
						if (rx_latency < rx_latency_min)
							rx_latency_min = rx_latency;
					}
					rx_latency_total += rx_latency;

					// RX throughput per task
					double rx_throughput = (double)rx_data_len / ((double)rx_latency / 1000000000L);
					if (enq_id == 0) {
						rx_throughput_max = rx_throughput;
						rx_throughput_min = rx_throughput;
					} else {
						if (rx_throughput > rx_throughput_max)
							rx_throughput_max = rx_throughput;
						if (rx_throughput < rx_throughput_min)
							rx_throughput_min = rx_throughput;
					}
				}

				if (getopt_is_receive_data()) {
					dmacmd_info_t *pdmadmacmdinfo = get_deqdmacmdinfo(ch_id, enq_id);

					// TX Data size
					tx_data_len = pdmadmacmdinfo->data_len;

					// TX latency per task
					uint64_t tx_latency = 0;
					if (getopt_is_send_data()) {
						// for HOST->FPGA-> HOST
						tx_latency = time_duration(&g_timestamp_rx[ch_id][enq_id].end_time, &g_timestamp_tx[ch_id][enq_id].end_time);
					} else {
						tx_latency = time_duration(&g_timestamp_tx[ch_id][enq_id].start_time, &g_timestamp_tx[ch_id][enq_id].end_time);
					}
					if (enq_id == 0) {
						tx_latency_max = tx_latency;
						tx_latency_min = tx_latency;
					} else {
						if (tx_latency > tx_latency_max)
							tx_latency_max = tx_latency;
						if (tx_latency < tx_latency_min)
							tx_latency_min = tx_latency;
					}
					tx_latency_total += tx_latency;


					// TX throughput per task
					double tx_throughput = (double)tx_data_len / ((double)tx_latency / 1000000000L);
					if (enq_id == 0) {
						tx_throughput_max = tx_throughput;
						tx_throughput_min = tx_throughput;
					} else {
						if (tx_throughput > tx_throughput_max)
							tx_throughput_max = tx_throughput;
						if (tx_throughput < tx_throughput_min)
							tx_throughput_min = tx_throughput;
					}
				}

				// RX+TX Data size
				rxtx_data_len = rx_data_len + tx_data_len;

				if (getopt_is_send_data() && getopt_is_receive_data()) {
					// RX (enqueue)->TX (dequeue) latency per task
					uint64_t rxtx_latency = time_duration(&g_timestamp_rx[ch_id][enq_id].start_time, &g_timestamp_tx[ch_id][enq_id].end_time);
					if (enq_id == 0) {
						rxtx_latency_max = rxtx_latency;
						rxtx_latency_min = rxtx_latency;
					} else {
						if (rxtx_latency > rxtx_latency_max)
							rxtx_latency_max = rxtx_latency;
						if (rxtx_latency < rxtx_latency_min)
							rxtx_latency_min = rxtx_latency;
					}
					rxtx_latency_total += rxtx_latency;

					// RX (enqueue)->TX (dequeue) throughput per task
					double rxtx_throughput = ((double)rxtx_data_len / 2L) / ((double)rxtx_latency / 1000000000L);
					if (enq_id == 0) {
						rxtx_throughput_max = rxtx_throughput;
						rxtx_throughput_min = rxtx_throughput;
					} else {
						if (rxtx_throughput > rxtx_throughput_max)
							rxtx_throughput_max = rxtx_throughput;
						if (rxtx_throughput < rxtx_throughput_min)
							rxtx_throughput_min = rxtx_throughput;
					}
				}
				// using header timestamp
				if (getopt_is_receive_data()) {
					uint64_t rxtx_header_latency = g_timestamp_header_tx[ch_id][enq_id] - g_timestamp_header_rx[ch_id][enq_id];
					if (enq_id == 0) {
						rxtx_header_latency_max = rxtx_header_latency;
						rxtx_header_latency_min = rxtx_header_latency;
					} else {
						if (rxtx_header_latency > rxtx_header_latency_max)
							rxtx_header_latency_max = rxtx_header_latency;
						if (rxtx_header_latency < rxtx_header_latency_min)
							rxtx_header_latency_min = rxtx_header_latency;
					}
					rxtx_header_latency_total += rxtx_header_latency;
				}

				// Total Data size of this CH
				rx_total_data_len += rx_data_len;
				tx_total_data_len += tx_data_len;
				rxtx_total_data_len += rxtx_data_len;
			}

			// Total Data size of all channels
			all_ch_rx_data_len += rx_total_data_len;
			all_ch_tx_data_len += tx_total_data_len;
			all_ch_rxtx_data_len += rxtx_total_data_len;

			if (getopt_is_send_data()) {
				// average RX latency
				rx_latency_ave = rx_latency_total / enq_num;

				// average RX throughput
				rx_throughput_ave = (double)rx_data_len / ((double)rx_latency_ave / 1000000000L);

				// RX latency from first task to last task
				rx_latency_alltask = time_duration(&g_timestamp_rx[ch_id][0].start_time, &g_timestamp_rx[ch_id][enq_num - 1].end_time);

				// RX throughput from first task to last task
				rx_throughput_alltask = (double)rx_total_data_len / ((double)rx_latency_alltask / 1000000000L);
				// RX
				rx_latency_per_task = rx_latency_alltask / enq_num;

				// RX
				rx_throughput_per_task = (double)rx_data_len / ((double)rx_latency_per_task / 1000000000L);
			}

			if (getopt_is_receive_data()) {
				// average TX latency
				tx_latency_ave = tx_latency_total / enq_num;

				// average TX throughput
				tx_throughput_ave = (double)tx_data_len / ((double)tx_latency_ave / 1000000000L);

				// TX latency from first task to last task
				if (getopt_is_send_data()) {
					// for HOST->FPGA-> HOST
					tx_latency_alltask = time_duration(&g_timestamp_rx[ch_id][0].end_time, &g_timestamp_tx[ch_id][enq_num - 1].end_time);
				} else {
					tx_latency_alltask = time_duration(&g_timestamp_tx[ch_id][0].start_time, &g_timestamp_tx[ch_id][enq_num - 1].end_time);
				}

				// TX throughput of first task to last task
				tx_throughput_alltask = (double)tx_total_data_len / ((double)tx_latency_alltask / 1000000000L);
				// TX
				tx_latency_per_task = tx_latency_alltask / enq_num;

				// TX
				tx_throughput_per_task = (double)tx_data_len / ((double)tx_latency_per_task / 1000000000L);
			}

			if (getopt_is_send_data() && getopt_is_receive_data()) {
				// average RX (enqueue)->TX (dequeue) latency
				rxtx_latency_ave = rxtx_latency_total / enq_num;

				// average RX (enqueue)->TX (dequeue) throughput
				rxtx_throughput_ave = ((double)rxtx_data_len / 2L) / ((double)rxtx_latency_ave / 1000000000L);

				// latency from RX (enqueue) first task to TX (dequeue) last task
				rxtx_latency_alltask = time_duration(&g_timestamp_rx[ch_id][0].start_time, &g_timestamp_tx[ch_id][enq_num - 1].end_time);
				// Throughput from RX (enqueue) first task to TX (dequeue) last task
				rxtx_throughput_alltask = ((double)rxtx_total_data_len / 2L) / ((double)rxtx_latency_alltask / 1000000000L);
				// RX(enqueue)->TX(dequeue)latency
				rxtx_latency_per_task = rxtx_latency_alltask / enq_num;

				// RX(enqueue)->TX(dequeue)throughput
				rxtx_throughput_per_task = ((double)rxtx_data_len / 2L) / ((double)rxtx_latency_per_task / 1000000000L);
			}
			// using header timestamp
			if (getopt_is_receive_data()) {
				// average RX (enqueue)->TX (dequeue) latency
				rxtx_header_latency_ave = rxtx_header_latency_total / enq_num;
			}

			double rx_data_len_m = (double)rx_data_len / 1000000;
			double rx_data_len_mi = (double)rx_data_len / (DATA_SIZE_1KB*DATA_SIZE_1KB);
			double tx_data_len_m = (double)tx_data_len / 1000000;
			double tx_data_len_mi = (double)tx_data_len / (DATA_SIZE_1KB*DATA_SIZE_1KB);
			double rxtx_data_len_m = (double)rxtx_data_len / 1000000;
			double rxtx_data_len_mi = (double)rxtx_data_len / (DATA_SIZE_1KB*DATA_SIZE_1KB);
			double rx_latency_max_s = (double)rx_latency_max / 1000000000L;
			double rx_latency_ave_s = (double)rx_latency_ave / 1000000000L;
			double rx_latency_min_s = (double)rx_latency_min / 1000000000L;
			double rx_latency_per_task_s = (double)rx_latency_per_task / 1000000000L;
			double tx_latency_max_s = (double)tx_latency_max / 1000000000L;
			double tx_latency_ave_s = (double)tx_latency_ave / 1000000000L;
			double tx_latency_min_s = (double)tx_latency_min / 1000000000L;
			double tx_latency_per_task_s = (double)tx_latency_per_task / 1000000000L;
			double rxtx_latency_max_s = (double)rxtx_latency_max / 1000000000L;
			double rxtx_latency_ave_s = (double)rxtx_latency_ave / 1000000000L;
			double rxtx_latency_min_s = (double)rxtx_latency_min / 1000000000L;
			double rxtx_latency_per_task_s = (double)rxtx_latency_per_task / 1000000000L;
			double rxtx_header_latency_max_ms = (double)rxtx_header_latency_max / 1000000L;
			double rxtx_header_latency_ave_ms = (double)rxtx_header_latency_ave / 1000000L;
			double rxtx_header_latency_min_ms = (double)rxtx_header_latency_min / 1000000L;
			double rx_throughput_max_m = rx_throughput_max / 1000000;
			double rx_throughput_max_mi = rx_throughput_max / (DATA_SIZE_1KB*DATA_SIZE_1KB);
			double rx_throughput_ave_m = rx_throughput_ave / 1000000;
			double rx_throughput_ave_mi = rx_throughput_ave / (DATA_SIZE_1KB*DATA_SIZE_1KB);
			double rx_throughput_min_m = rx_throughput_min / 1000000;
			double rx_throughput_min_mi = rx_throughput_min / (DATA_SIZE_1KB*DATA_SIZE_1KB);
			double rx_throughput_per_task_m = rx_throughput_per_task / 1000000;
			double rx_throughput_per_task_mi = rx_throughput_per_task / (DATA_SIZE_1KB*DATA_SIZE_1KB);
			double tx_throughput_max_m = tx_throughput_max / 1000000;
			double tx_throughput_max_mi = tx_throughput_max / (DATA_SIZE_1KB*DATA_SIZE_1KB);
			double tx_throughput_ave_m = tx_throughput_ave / 1000000;
			double tx_throughput_ave_mi = tx_throughput_ave / (DATA_SIZE_1KB*DATA_SIZE_1KB);
			double tx_throughput_min_m = tx_throughput_min / 1000000;
			double tx_throughput_min_mi = tx_throughput_min / (DATA_SIZE_1KB*DATA_SIZE_1KB);
			double tx_throughput_per_task_m = tx_throughput_per_task / 1000000;
			double tx_throughput_per_task_mi = tx_throughput_per_task / (DATA_SIZE_1KB*DATA_SIZE_1KB);
			double rxtx_throughput_max_m = rxtx_throughput_max / 1000000;
			double rxtx_throughput_max_mi = rxtx_throughput_max / (DATA_SIZE_1KB*DATA_SIZE_1KB);
			double rxtx_throughput_ave_m = rxtx_throughput_ave / 1000000;
			double rxtx_throughput_ave_mi = rxtx_throughput_ave / (DATA_SIZE_1KB*DATA_SIZE_1KB);
			double rxtx_throughput_min_m = rxtx_throughput_min / 1000000;
			double rxtx_throughput_min_mi = rxtx_throughput_min / (DATA_SIZE_1KB*DATA_SIZE_1KB);
			double rxtx_throughput_per_task_m = rxtx_throughput_per_task / 1000000;
			double rxtx_throughput_per_task_mi = rxtx_throughput_per_task / (DATA_SIZE_1KB*DATA_SIZE_1KB);

			double rx_total_data_len_m = (double)rx_total_data_len / 1000000;
			double rx_total_data_len_mi = (double)rx_total_data_len / (DATA_SIZE_1KB*DATA_SIZE_1KB);
			double tx_total_data_len_m = (double)tx_total_data_len / 1000000;
			double tx_total_data_len_mi = (double)tx_total_data_len / (DATA_SIZE_1KB*DATA_SIZE_1KB);
			double rxtx_total_data_len_m = (double)rxtx_total_data_len / 1000000;
			double rxtx_total_data_len_mi = (double)rxtx_total_data_len / (DATA_SIZE_1KB*DATA_SIZE_1KB);
			double rx_latency_alltask_s = (double)rx_latency_alltask / 1000000000L;
			double tx_latency_alltask_s = (double)tx_latency_alltask / 1000000000L;
			double rxtx_latency_alltask_s = (double)rxtx_latency_alltask / 1000000000L;
			double rx_throughput_alltask_m = rx_throughput_alltask / 1000000;
			double rx_throughput_alltask_mi = rx_throughput_alltask / (DATA_SIZE_1KB*DATA_SIZE_1KB);
			double tx_throughput_alltask_m = tx_throughput_alltask / 1000000;
			double tx_throughput_alltask_mi = tx_throughput_alltask / (DATA_SIZE_1KB*DATA_SIZE_1KB);
			double rxtx_throughput_alltask_m = rxtx_throughput_alltask / 1000000;
			double rxtx_throughput_alltask_mi = rxtx_throughput_alltask / (DATA_SIZE_1KB*DATA_SIZE_1KB);

			rslt2file("==========================================================\n");
			rslt2file("CH(%u)\n", ch_id);
			rslt2file("--------------------------------------------------------\n");
			// Performance information per task
			rslt2file("Performance per task\n");
			rslt2file("  transfer data size per task\n");
			rslt2file("         RX : %lu [Byte]  (%.3lf [MB], %.3lf [MiB])\n", rx_data_len, rx_data_len_m, rx_data_len_mi);
			rslt2file("         TX : %lu [Byte]  (%.3lf [MB], %.3lf [MiB])\n", tx_data_len, tx_data_len_m, tx_data_len_mi);
			rslt2file("      RX+TX : %lu [Byte]  (%.3lf [MB], %.3lf [MiB])\n", rxtx_data_len, rxtx_data_len_m, rxtx_data_len_mi);
			rslt2file("  RX latency per task    : %lu [nsec] (%.9lf [sec])\n", rx_latency_per_task, rx_latency_per_task_s);
			rslt2file("  RX throughput per task : %.3lf [Byte/s]  (%.3lf [MB/s], %.3lf [MiB/s])\n", rx_throughput_per_task, rx_throughput_per_task_m, rx_throughput_per_task_mi);
			rslt2file("  TX latency per task    : %lu [nsec]  (%.9lf [sec])\n", tx_latency_per_task, tx_latency_per_task_s);
			rslt2file("  TX throughput per task : %.3lf [Byte/s]  (%.3lf [MB/s], %.3lf [MiB/s])\n", tx_throughput_per_task, tx_throughput_per_task_m, tx_throughput_per_task_mi);
			if (getopt_is_send_data() && getopt_is_receive_data()) {
				rslt2file("  RX(enqueue)->TX(dequeue) latency per task    : %lu [nsec]  (%.9lf [sec])\n", rxtx_latency_per_task, rxtx_latency_per_task_s);
				rslt2file("  RX(enqueue)->TX(dequeue) throughput per task : %.3lf [Byte/s]  (%.3lf [MB/s], %.3lf [MiB/s])\n", rxtx_throughput_per_task, rxtx_throughput_per_task_m ,rxtx_throughput_per_task_mi);
			}
			if (getopt_is_receive_data()) {
				rslt2file("header timestamp->TX(dequeue) latency per task\n");
				rslt2file("latency(ave) = %.6lf [msec]\n", rxtx_header_latency_ave_ms);
				rslt2file("latency(max) = %.6lf [msec]\n", rxtx_header_latency_max_ms);
				rslt2file("latency(min) = %.6lf [msec]\n", rxtx_header_latency_min_ms);
			}
			rslt2file("--------------------------------------------------------\n");
			// Performance information display from the start of the first task to the end of all tasks in this CH
			rslt2file("All task performance\n");
			rslt2file("  number of task : %u\n", enq_num);
			rslt2file("  transfer data size\n");
			rslt2file("         RX : %lu [Byte]  (%.3lf [MB], %.3lf [MiB])\n", rx_total_data_len, rx_total_data_len_m, rx_total_data_len_mi);
			rslt2file("         TX : %lu [Byte]  (%.3lf [MB], %.3lf [MiB])\n", tx_total_data_len, tx_total_data_len_m, tx_total_data_len_mi);
			rslt2file("      RX+TX : %lu [Byte]  (%.3lf [MB], %.3lf [MiB])\n", rxtx_total_data_len, rxtx_total_data_len_m, rxtx_total_data_len_mi);
			rslt2file("  RX latency    : %lu [nsec]  (%.9lf [sec])\n", rx_latency_alltask, rx_latency_alltask_s);
			rslt2file("  RX throughput : %.3lf [Byte/s]  (%.3lf [MB/s], %.3lf [MiB/s])\n", rx_throughput_alltask, rx_throughput_alltask_m, rx_throughput_alltask_mi);
			rslt2file("  TX latency    : %lu [nsec]  (%.9lf [sec])\n", tx_latency_alltask, tx_latency_alltask_s);
			rslt2file("  TX throughput : %.3lf [Byte/s]  (%.3lf [MB/s], %.3lf [MiB/s])\n", tx_throughput_alltask, tx_throughput_alltask_m, tx_throughput_alltask_mi);
			if (getopt_is_send_data() && getopt_is_receive_data()) {
				rslt2file("  RX(task=1 enqueue)->TX(task=%u dequeue) latency    : %lu [nsec]  (%.9lf [sec])\n", enq_num, rxtx_latency_alltask, rxtx_latency_alltask_s);
				rslt2file("  RX(task=1 enqueue)->TX(task=%u dequeue) throughput : %.3lf [Byte/s]  (%.3lf [MB/s], %.3lf [MiB/s])\n", enq_num, rxtx_throughput_alltask, rxtx_throughput_alltask_m, rxtx_throughput_alltask_mi);
			}
			rslt2file("\n");
		}
	}

	// ----------------------------------
	// Calculation of performance measurement results of all CHs
	// ----------------------------------
	struct timespec *all_ch_rx_first_time;
	struct timespec *all_ch_rx_last_time;
	struct timespec *all_ch_tx_first_time;
	struct timespec *all_ch_tx_last_time;
	bool f = true;
	uint64_t enq_num_allch = 0;
	for (size_t i=0; i < CH_NUM_MAX; i++) {
		uint32_t ch_id = i;
		if (getopt_ch_en(ch_id)) {
			enq_num_allch += enq_num;
			// The first CH loop always obtains the time.
			if (f) {
				if (getopt_is_send_data()) {
					// Get first task start time of all CH RX
					all_ch_rx_first_time = &g_timestamp_rx[ch_id][0].start_time;

					// Obtain the last task end time of all CH RX
					all_ch_rx_last_time = &g_timestamp_rx[ch_id][enq_num - 1].end_time;
				}
				if (getopt_is_receive_data()) {
					// Acquisition of the first task start time of all CH TXs
					if (getopt_is_send_data()) {
						// for HOST->FPGA-> HOST
						all_ch_tx_first_time = &g_timestamp_rx[ch_id][0].end_time;
					} else {
						all_ch_tx_first_time = &g_timestamp_tx[ch_id][0].start_time;
					}

					// Obtain the last task end time of all CH TXs
					all_ch_tx_last_time = &g_timestamp_tx[ch_id][enq_num - 1].end_time;
				}
				f = false;
				continue;
			}

			// The second and subsequent CH loops are compared with the acquired time.

			if (getopt_is_send_data()) {
				// Obtain the first task start time of all CH RX (update to the earliest time)
				if (g_timestamp_rx[ch_id][0].start_time.tv_sec < all_ch_rx_first_time->tv_sec) {
					all_ch_rx_first_time = &g_timestamp_rx[ch_id][0].start_time;
					continue;
				} else if (g_timestamp_rx[ch_id][0].start_time.tv_sec == all_ch_rx_first_time->tv_sec) {
					if (g_timestamp_rx[ch_id][0].start_time.tv_nsec < all_ch_rx_first_time->tv_nsec) {
						all_ch_rx_first_time = &g_timestamp_rx[ch_id][0].start_time;
						continue;
					}
				}

				// Obtain the last task end time for all CH RX (update to the latest time)
				if (g_timestamp_rx[ch_id][enq_num - 1].end_time.tv_sec > all_ch_rx_last_time->tv_sec) {
					all_ch_rx_last_time = &g_timestamp_rx[ch_id][enq_num - 1].end_time;
					continue;
				} else if (g_timestamp_rx[ch_id][enq_num - 1].end_time.tv_sec == all_ch_rx_last_time->tv_sec) {
					if (g_timestamp_rx[ch_id][enq_num - 1].end_time.tv_nsec > all_ch_rx_last_time->tv_nsec) {
						all_ch_rx_last_time = &g_timestamp_rx[ch_id][enq_num - 1].end_time;
						continue;
					}
				}
			}

			if (getopt_is_receive_data()) {
				// Acquisition of the first task start time of all CH TXs (updated to the earliest time)
				if (getopt_is_send_data()) {
					// for HOST->FPGA-> HOST
					if (g_timestamp_rx[ch_id][0].end_time.tv_sec < all_ch_tx_first_time->tv_sec) {
						all_ch_tx_first_time = &g_timestamp_rx[ch_id][0].end_time;
						continue;
					} else if (g_timestamp_rx[ch_id][0].end_time.tv_sec == all_ch_tx_first_time->tv_sec) {
						if (g_timestamp_rx[ch_id][0].end_time.tv_nsec < all_ch_tx_first_time->tv_nsec) {
							all_ch_tx_first_time = &g_timestamp_rx[ch_id][0].end_time;
							continue;
						}
					}
				} else {
					if (g_timestamp_tx[ch_id][0].start_time.tv_sec < all_ch_tx_first_time->tv_sec) {
						all_ch_tx_first_time = &g_timestamp_tx[ch_id][0].start_time;
						continue;
					} else if (g_timestamp_tx[ch_id][0].start_time.tv_sec == all_ch_tx_first_time->tv_sec) {
						if (g_timestamp_tx[ch_id][0].start_time.tv_nsec < all_ch_tx_first_time->tv_nsec) {
							all_ch_tx_first_time = &g_timestamp_tx[ch_id][0].start_time;
							continue;
						}
					}
				}

				// Obtain the last task end time of all CH TXs (update to the latest time)
				if (g_timestamp_tx[ch_id][enq_num - 1].end_time.tv_sec > all_ch_tx_last_time->tv_sec) {
					all_ch_tx_last_time = &g_timestamp_tx[ch_id][enq_num - 1].end_time;
					continue;
				} else if (g_timestamp_tx[ch_id][enq_num - 1].end_time.tv_sec == all_ch_tx_last_time->tv_sec) {
					if (g_timestamp_tx[ch_id][enq_num - 1].end_time.tv_nsec > all_ch_tx_last_time->tv_nsec) {
						all_ch_tx_last_time = &g_timestamp_tx[ch_id][enq_num - 1].end_time;
						continue;
					}
				}
			}
		}
	}

	double all_ch_rx_data_len_m = (double)all_ch_rx_data_len / (DATA_SIZE_1KB*DATA_SIZE_1KB);
	double all_ch_tx_data_len_m = (double)all_ch_tx_data_len / (DATA_SIZE_1KB*DATA_SIZE_1KB);
	double all_ch_rxtx_data_len_m = (double)all_ch_rxtx_data_len / (DATA_SIZE_1KB*DATA_SIZE_1KB);

	if (getopt_is_send_data()) {
		// all CH RX latencies from first task to last task
		all_ch_rx_latency = time_duration(all_ch_rx_first_time, all_ch_rx_last_time);
	}
	if (getopt_is_receive_data()) {
		// Latency of all CH TX first task to last task
		all_ch_tx_latency = time_duration(all_ch_tx_first_time, all_ch_tx_last_time);
	}
	double all_ch_rx_latency_s = (double)all_ch_rx_latency / 1000000000L;
	double all_ch_tx_latency_s = (double)all_ch_tx_latency / 1000000000L;

	if (getopt_is_send_data()) {
		// throughput of all CH RX first task to last task
		all_ch_rx_throughput = (double)all_ch_rx_data_len / all_ch_rx_latency_s;
	}
	if (getopt_is_receive_data()) {
		// throughput of all CH TX first task to last task
		all_ch_tx_throughput = (double)all_ch_tx_data_len / all_ch_tx_latency_s;
	}
	double all_ch_rx_throughput_m = all_ch_rx_throughput / (DATA_SIZE_1KB*DATA_SIZE_1KB);
	double all_ch_tx_throughput_m = all_ch_tx_throughput / (DATA_SIZE_1KB*DATA_SIZE_1KB);

	if (getopt_is_send_data() && getopt_is_receive_data()) {
		// Latency of all CH RX (enqueue) first task to TX (dequeue) last task
		all_ch_rxtx_latency = time_duration(all_ch_rx_first_time, all_ch_tx_last_time);

		// Throughput of all CH RX (enqueue) first task to TX (dequeue) last task
		all_ch_rxtx_throughput = ((double)all_ch_rxtx_data_len / 2L) / ((double)all_ch_rxtx_latency / 1000000000L);
	}
	double all_ch_rxtx_latency_s = (double)all_ch_rxtx_latency / 1000000000L;
	double all_ch_rxtx_throughput_m = all_ch_rxtx_throughput / (DATA_SIZE_1KB*DATA_SIZE_1KB);

	// Performance information display from the start of the first task to the end of all tasks in all CHs
	rslt2file("==========================================================\n");
	rslt2file("ALL CH TOTAL\n");
	rslt2file("--------------------------------------------------------\n");
	rslt2file("  number of ch : %u\n", total_ch_num);
	rslt2file("  number of all task : %lu\n", enq_num_allch);//enq_num * total_ch_num);
	rslt2file("  transfer data size\n");
	rslt2file("         RX : %lu [Byte]  (%.3lf [MB])\n", all_ch_rx_data_len, all_ch_rx_data_len_m);
	rslt2file("         TX : %lu [Byte]  (%.3lf [MB])\n", all_ch_tx_data_len, all_ch_tx_data_len_m);
	rslt2file("      RX+TX : %lu [Byte]  (%.3lf [MB])\n", all_ch_rxtx_data_len, all_ch_rxtx_data_len_m);
	rslt2file("  RX latency    : %lu [nsec]  (%.9lf [sec])\n", all_ch_rx_latency, all_ch_rx_latency_s);
	rslt2file("  RX throughput : %.3lf [Byte/s]  (%.3lf [MB/s])\n", all_ch_rx_throughput, all_ch_rx_throughput_m);
	rslt2file("  TX latency    : %lu [nsec]  (%.9lf [sec])\n", all_ch_tx_latency, all_ch_tx_latency_s);
	rslt2file("  TX throughput : %.3lf [Byte/s]  (%.3lf [MB/s])\n", all_ch_tx_throughput, all_ch_tx_throughput_m);
	if (getopt_is_send_data() && getopt_is_receive_data()) {
		rslt2file("  RX(task=1 enqueue)->TX(task=%lu dequeue) latency    : %lu [nsec]  (%.9lf [sec])\n", enq_num_allch, all_ch_rxtx_latency, all_ch_rxtx_latency_s);//enq_num * total_ch_num, all_ch_rxtx_latency, all_ch_rxtx_latency_s);
		rslt2file("  RX(task=1 enqueue)->TX(task=%lu dequeue) throughput : %.3lf [Byte/s]  (%.3lf [MB/s])\n", enq_num_allch, all_ch_rxtx_throughput, all_ch_rxtx_throughput_m);//enq_num * total_ch_num, all_ch_rxtx_throughput, all_ch_rxtx_throughput_m);
	}
	rslt2file("\n");
	rslt2file("--------------------------------------------------------\n");

}

//-----------------------------------------
// for debug log display
//-----------------------------------------
void prlog_mngque(const mngque_t *p, uint32_t ch_id)
{
	logfile(LOG_DEBUG, "pr_mngque...\n");

	if (p != NULL) {
		logfile(LOG_DEBUG, "pr_mngque(%p)\n",p);
		logfile(LOG_DEBUG, "  enq_num(%d)\n",p->enq_num);
		logfile(LOG_DEBUG, "  srcdsize(0x%x)\n",p->srcdsize);
		logfile(LOG_DEBUG, "  dst1dsize(0x%x)\n",p->dst1dsize);
		logfile(LOG_DEBUG, "  dst2dsize(0x%x)\n",p->dst2dsize);
		logfile(LOG_DEBUG, "  d2ddsize(0x%x)\n",p->d2ddsize);
		logfile(LOG_DEBUG, "  srcbuflen(0x%x)\n",p->srcbuflen);
		logfile(LOG_DEBUG, "  dst1buflen(0x%x)\n",p->dst1buflen);
		logfile(LOG_DEBUG, "  dst2buflen(0x%x)\n",p->dst2buflen);
		logfile(LOG_DEBUG, "  d2dbuflen(0x%x)\n",p->d2dbuflen);
		logfile(LOG_DEBUG, "  d2dbufp(%p)\n",p->d2dbufp);
		for (size_t i=0; i < getopt_shmalloc_num(); i++) {
			logfile(LOG_DEBUG, "  [%zu] srcbufp(%p)\n", i, p->enqbuf[i].srcbufp);
			logfile(LOG_DEBUG, "  [%zu] dst1bufp(%p)\n", i, p->enqbuf[i].dst1bufp);
			logfile(LOG_DEBUG, "  [%zu] dst2bufp(%p)\n", i, p->enqbuf[i].dst2bufp);
		}
	}
}

void prlog_divide_que(const divide_que_t *p)
{
	logfile(LOG_DEBUG, "pr_divide_que...\n");

	if (p != NULL) {
		logfile(LOG_DEBUG, "pr_divide_que(%p)\n",p);
		logfile(LOG_DEBUG, "  enq_num(%u)\n",p->que_num);
		logfile(LOG_DEBUG, "  que_num_rem(%u)\n",p->que_num_rem);
		logfile(LOG_DEBUG, "  div_num(%u)\n",p->div_num);
	}
}

int32_t prlog_dma_info(const dma_info_t *p, uint32_t ch_id)
{
	if (p == NULL) {
		return -1;
	}
	logfile(LOG_DEBUG, "CH(%u) pr_dma_info(%p)\n", ch_id, p);
	logfile(LOG_DEBUG, "  CH(%u) dev_id(0x%x)\n", ch_id, p->dev_id);
	logfile(LOG_DEBUG, "  CH(%u) dir(%d)\n", ch_id, p->dir);
	logfile(LOG_DEBUG, "  CH(%u) chid(0x%x)\n", ch_id, p->chid);
	logfile(LOG_DEBUG, "  CH(%u) queue_addr(%p)\n", ch_id, p->queue_addr);
	logfile(LOG_DEBUG, "  CH(%u) queue_size(%u)\n", ch_id, p->queue_size);
	if (p->connector_id == NULL) {
		logfile(LOG_DEBUG, "  CH(%u) connector_id(%s)\n", ch_id, "");
	} else {
		logfile(LOG_DEBUG, "  CH(%u) connector_id(%s)\n", ch_id, p->connector_id);
	}

	return 0;
}

int32_t prlog_dmacmd_info(const dmacmd_info_t *p, uint32_t ch_id, uint32_t enq_id)
{
	if (p == NULL) {
		return -1;
	}
	logfile(LOG_DEBUG, "CH(%u) ENQ(%u) pr_dmacmd_info(%p)\n", ch_id, enq_id, p);
	logfile(LOG_DEBUG, "  CH(%u) ENQ(%u) task_id(0x%x)\n", ch_id, enq_id, p->task_id);
	logfile(LOG_DEBUG, "  CH(%u) ENQ(%u) data_len(0x%x)\n", ch_id, enq_id, p->data_len);
	logfile(LOG_DEBUG, "  CH(%u) ENQ(%u) data_addr(%p)\n", ch_id, enq_id, p->data_addr);
	logfile(LOG_DEBUG, "  CH(%u) ENQ(%u) desc_addr(%p)\n", ch_id, enq_id, p->desc_addr);
	logfile(LOG_DEBUG, "  CH(%u) ENQ(%u) result_status(%u)\n", ch_id, enq_id, p->result_status);
	logfile(LOG_DEBUG, "  CH(%u) ENQ(%u) result_task_id(0x%x)\n", ch_id, enq_id, p->result_task_id);
	logfile(LOG_DEBUG, "  CH(%u) ENQ(%u) result_data_len(0x%x)\n", ch_id, enq_id, p->result_data_len);
	logfile(LOG_DEBUG, "  CH(%u) ENQ(%u) result_data_addr(%p)\n", ch_id, enq_id, p->result_data_addr);

	return 0;
}

int32_t prlog_connect_info(const fpga_lldma_connect_t *p, uint32_t ch_id)
{
	if (p == NULL) {
		return -1;
	}
	logfile(LOG_DEBUG, "CH(%u) pr_connect_info(%p)\n", ch_id, p);
	logfile(LOG_DEBUG, "  CH(%u) tx_dev_id(0x%x)\n", ch_id, p->tx_dev_id);
	logfile(LOG_DEBUG, "  CH(%u) tx_chid(0x%x)\n", ch_id, p->tx_chid);
	logfile(LOG_DEBUG, "  CH(%u) rx_dev_id(0x%x)\n", ch_id, p->rx_dev_id);
	logfile(LOG_DEBUG, "  CH(%u) rx_chid(0x%x)\n", ch_id, p->rx_chid);
	logfile(LOG_DEBUG, "  CH(%u) buf_size(%u)\n", ch_id, p->buf_size);
	logfile(LOG_DEBUG, "  CH(%u) buf_addr(%p)\n", ch_id, p->buf_addr);
	if (p->connector_id == NULL) {
		logfile(LOG_DEBUG, "  CH(%u) connector_id(%s)\n", ch_id, "");
	} else {
		logfile(LOG_DEBUG, "  CH(%u) connector_id(%s)\n", ch_id, p->connector_id);
	}

	return 0;
}
