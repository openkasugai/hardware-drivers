/*************************************************
* Copyright 2024 NTT Corporation, FUJITSU LIMITED
* Licensed under the 3-Clause BSD License, see LICENSE for details.
* SPDX-License-Identifier: BSD-3-Clause
*************************************************/
/**
 * @file libfpgacommon_internal.h
 * @brief Header file for internal definition of common process
 */

#ifndef LIBFPGA_INCLUDE_LIBFPGA_INTERNAL_LIBFPGACOMMON_INTERNAL_H_
#define LIBFPGA_INCLUDE_LIBFPGA_INTERNAL_LIBFPGACOMMON_INTERNAL_H_

#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Function which poll by callback function
 * @param[in] timeout Timeout for polling
 * @param[in] interval Interval for polling
 * @param[in] clb Callback function
 * @param[in] arg Arguments for callback function
 * @retval 0
 *   Success
 * @retval -INVALID_ARGUMENT
 *   clb is null
 * @return
 *   clb's retval when timeout happened.
 * @details
 *   Callback function should return 0 when succeed to poll status,
 *    and should return non-zero value when failed to poll status.
 */
int __fpga_common_polling(
        const struct timeval *timeout,
        const struct timeval *interval,
        int (*clb)(void*),
        void *arg);

#ifdef __cplusplus
}
#endif

#endif  // LIBFPGA_INCLUDE_LIBFPGA_INTERNAL_LIBFPGACOMMON_INTERNAL_H_
