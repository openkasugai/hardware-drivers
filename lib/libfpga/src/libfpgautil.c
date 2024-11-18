/*************************************************
* Copyright 2024 NTT Corporation, FUJITSU LIMITED
* Licensed under the 3-Clause BSD License, see LICENSE for details.
* SPDX-License-Identifier: BSD-3-Clause
*************************************************/

#include <libfpga_internal/libfpgautil.h>

#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <string.h>

int fpgautil_open(const char *pathname, int flags) {
    return open(pathname , flags);
}

int fpgautil_close(int fd) {
    return close(fd);
}

int fpgautil_ioctl(int fd, unsigned long request1, void * request2) {  //NOLINT
    return ioctl(fd , request1, request2);
}

void* fpgautil_mmap(void *addr, size_t length, int prot, int flags, int fd, off_t offset) {
    return mmap(addr, length, prot, flags, fd, offset);
}

int fpgautil_munmap(void *addr, size_t length) {
    return munmap(addr, length);
}

ssize_t fpgautil_read(int fd, void *buf, size_t count) {
    return read(fd, buf, count);
}

int fpgautil_strcmp(const char *s1, const char *s2) {
    return strcmp(s1, s2);
}
