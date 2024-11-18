/*************************************************
* Copyright 2024 NTT Corporation, FUJITSU LIMITED
* Licensed under the 3-Clause BSD License, see LICENSE for details.
* SPDX-License-Identifier: BSD-3-Clause
*************************************************/
/**
 * @file libfpgautil.h
 * @brief Header file for functions for wrap system calls
 */

#ifndef LIBFPGA_INCLUDE_LIBFPGA_INTERNAL_LIBFPGAUTIL_H_
#define LIBFPGA_INCLUDE_LIBFPGA_INTERNAL_LIBFPGAUTIL_H_

#include <unistd.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Function which wrap open() for google mock
 * @details See open().
 */
int fpgautil_open(const char *pathname, int flags);

/**
 * @brief Function which wrap close() for google mock
 * @details See close().
 */
int fpgautil_close(int fd);

/**
 * @brief Function which wrap ioctl() for google mock
 * @details See ioctl().
 * @details This library use ioctl only with 3 arguments.
 */
int fpgautil_ioctl(int fd, unsigned long request, void * request2);  //NOLINT

/**
 * @brief Function which wrap mmap() for google mock
 * @details See mmap().
 */
void* fpgautil_mmap(void *addr, size_t length, int prot, int flags, int fd, off_t offset);

/**
 * @brief Function which wrap munmap() for google mock
 * @details See munmap().
 */
int fpgautil_munmap(void *addr, size_t length);

/**
 * @brief Function which wrap read() for google mock
 * @details See read().
 */
ssize_t fpgautil_read(int fd, void *buf, size_t count);

#ifdef __cplusplus
}
#endif

#endif  // LIBFPGA_INCLUDE_LIBFPGA_INTERNAL_LIBFPGAUTIL_H_
