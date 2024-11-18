/*************************************************
* Copyright 2024 NTT Corporation, FUJITSU LIMITED
* Licensed under the 3-Clause BSD License, see LICENSE for details.
* SPDX-License-Identifier: BSD-3-Clause
*************************************************/
#ifndef __CPPFUNC_H__
#define __CPPFUNC_H__

#ifdef __cplusplus
extern "C" {
#endif
#define CPPF_CH_NUM_MAX	32
#define IMG_WIDTH_MAX           3840
#define IMG_HEIGHT_MAX          2160
#define IMG_DATA_SIZE_MAX       (IMG_WIDTH_MAX * IMG_HEIGHT_MAX * 3)

int32_t readimg(const char *p, uint8_t *memp);
int32_t dump_ppm(const uint8_t *memp, size_t height, size_t width, const char *ppm, size_t mode);
int32_t movie2image(const char *movie, uint32_t ch_id, size_t height, size_t width, size_t frame_num, uint8_t *outp, size_t *r_frame_num);
int32_t movie2cap(const char *movie, uint32_t ch_id);
int32_t movie2sendppm(const char *movie, uint32_t ch_id, size_t frame_num, size_t *r_frame_num, const char *ppmdir, uint32_t dump_ppm_num_max);

#ifdef __cplusplus
}
#endif

#endif /* __CPPFUNC_H__ */
