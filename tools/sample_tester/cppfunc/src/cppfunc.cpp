/*************************************************
* Copyright 2024 NTT Corporation, FUJITSU LIMITED
* Licensed under the 3-Clause BSD License, see LICENSE for details.
* SPDX-License-Identifier: BSD-3-Clause
*************************************************/
#include <iostream>
#include <cstring>
#include <opencv2/opencv.hpp>
#include "cppfunc.h"

struct FrameHeader {
    uint32_t marker;
    uint32_t payload_len;
    uint8_t reserved1[4];
    uint32_t frame_index;//sequence_num
    uint8_t reserved2[8];
    double local_ts;
    uint32_t channel_id;//data_id
    uint8_t reserved3[8];
	uint16_t h_checksum;//future function
    uint8_t reserved4[2];
};

static std::string g_cap_pipeline("qtdemux ! video/x-h264 ! h264parse ! openh264dec ! queue ! videoconvert ! appsink");
static std::vector<cv::VideoCapture> g_cap(CPPF_CH_NUM_MAX);
static std::vector<size_t> g_cap_prop_pos_frames(CPPF_CH_NUM_MAX, 0);

int32_t readimg(const char *p, uint8_t *memp)
{
	if (memp == NULL) {
		std::cout << "readimg failed. memp is NULL." << std::endl;
		return -1;
	}

	cv::Mat image = cv::imread(p);
	if (image.empty()) {
		std::cout << "imread failed (" << p << ")." << std::endl;
		return -1;
	}

	const size_t height  = image.rows;
	const size_t width   = image.cols;
	const size_t channel = image.channels();

	uint8_t *src_p = image.ptr();
	const size_t size = height * width * channel;
	std::memcpy(memp, src_p, size);

	return 0;
}

int32_t dump_ppm(const uint8_t *memp, size_t height, size_t width, const char *ppm, size_t mode)
{
	if (mode != 0 && mode != 1) {
		std::cout << "dump_ppm Error: ppm mode \"" << mode << "\" is invalid. only [0:ascii/1:binary]." << std::endl;
		std::cout << "dump_ppm Error: can't output to \"" << ppm << "\"" << std::endl;
		return -1;
	}

	const cv::Mat image(cv::Size(width, height), CV_8UC3, (void*)memp);

	std::vector<int32_t> params(2);
	params[0] = CV_IMWRITE_PXM_BINARY;
	params[1] = mode; // 0:ascii, 1:binary
	if ( ! cv::imwrite(ppm, image, params) ) {
		std::cout << "dump_ppm Error: can't output to \"" << ppm << "\"" << std::endl;
		return -1;
	}

	return 0;
}

int32_t movie2image(const char *movie, uint32_t ch_id, size_t height, size_t width, size_t frame_num, uint8_t *outp, size_t *r_frame_num)
{
	if (outp == NULL) {
		std::cout << "movie2image failed. outp is NULL." << std::endl;
		return -1;
	}

	*r_frame_num = 0;

	// Movie File Open
	std::string movie_stdstr(movie);
	std::string videosrc = "filesrc location=" + movie_stdstr + " ! " + g_cap_pipeline;
	cv::VideoCapture cap;
	cap.open(videosrc, cv::CAP_GSTREAMER);
	if ( ! cap.isOpened() ) {
		std::cout << "VideoCapture failed to open movie file \"" << movie << "\"." << std::endl;
		return -1;
	}
	// Start Frame Position Set
	//cap.set(cv::CAP_PROP_POS_FRAMES, g_cap_prop_pos_frames[ch_id]);

	size_t frame_cnt = 0;

	while (true) {
		// Get Video Frame
		cv::Mat input_mat;
		if (!cap.read(input_mat)) {
			if (input_mat.empty()) {
				// Return to first frame when last frame is reached
				//cap.set(cv::CAP_PROP_POS_FRAMES, 0);
				g_cap_prop_pos_frames[ch_id] = 0;
				continue;
			} else {
				std::cout << "Failed to get frame cv::VideoCapture" << std::endl;
				break;
			}
		}

		if (frame_cnt >= frame_num) {
			break;
		}

		frame_cnt++;

		// get current frame position
		g_cap_prop_pos_frames[ch_id] = cap.get(cv::CAP_PROP_POS_FRAMES);

		
		// image data output
		const size_t channel = input_mat.channels();
		uint8_t *src_p = nullptr;
		uint8_t *dst_p = outp + (height * width * channel * (frame_cnt - 1));
		const size_t size = height * width * channel;
		cv::Mat resize_mat;
		if (height == static_cast<size_t>(input_mat.rows) && width == static_cast<size_t>(input_mat.cols)) {
			src_p = input_mat.ptr();
		} else {
			// resize
			cv::resize(input_mat, resize_mat, cv::Size(width, height), 0, 0, cv::INTER_LINEAR);
			src_p = resize_mat.ptr();
		}
		std::memcpy(dst_p, src_p, size);

		// release
		input_mat.release();
		resize_mat.release();
	}

	// Number of generated movie frames
	*r_frame_num = frame_cnt;

	cap.release();

	return 0;
}

int32_t movie2cap(const char *movie, uint32_t ch_id)
{
	// Movie File Open
	std::string movie_stdstr(movie);
	std::string videosrc = "filesrc location=" + movie_stdstr + " ! " + g_cap_pipeline;
	g_cap[ch_id].open(videosrc, cv::CAP_GSTREAMER);
	if ( ! g_cap[ch_id].isOpened() ) {
		std::cout << "VideoCapture failed to open movie file \"" << movie << "\"." << std::endl;
		return -1;
	}

	return 0;
}

int32_t movie2sendppm(const char *movie, uint32_t ch_id, size_t frame_num, size_t *r_frame_num, const char *ppmdir, uint32_t dump_ppm_num_max)
{
	*r_frame_num = 0;

	// Movie File Open
	std::string movie_stdstr(movie);
	std::string videosrc = "filesrc location=" + movie_stdstr + " ! " + g_cap_pipeline;
	cv::VideoCapture cap;
	cap.open(videosrc, cv::CAP_GSTREAMER);
	if ( ! cap.isOpened() ) {
		std::cout << "VideoCapture failed to open movie file \"" << movie << "\"." << std::endl;
		return -1;
	}

	size_t frame_cnt = 0;

	while (true) {
		if (frame_cnt >= frame_num) {
			break;
		}

		// Get Video Frame
		cv::Mat input_mat;
		if (!cap.read(input_mat)) {
			if (input_mat.empty()) {
				break;
			} else {
				std::cout << "Failed to get frame cv::VideoCapture" << std::endl;
				break;
			}
		}

		frame_cnt++;

		// ppm
		if (frame_cnt < dump_ppm_num_max + 1) {
			char ppm[256];
			sprintf(ppm, "%s/ch%02u_task%zu_send.ppm", ppmdir, ch_id, frame_cnt);
			std::vector<int32_t> params(2);
			params[0] = CV_IMWRITE_PXM_BINARY;
			params[1] = 0; // 0:ascii, 1:binary
			if ( ! cv::imwrite(ppm, input_mat, params) ) {
				std::cout << "dump_ppm Error: can't output to \"" << ppm << "\"" << std::endl;
				return -1;
			}
		}

		// release
		input_mat.release();
	}

	// Number of generated movie frames
	*r_frame_num = frame_cnt;

	return 0;
}
