/*************************************************
* Copyright 2024 NTT Corporation, FUJITSU LIMITED
* Licensed under the 3-Clause BSD License, see LICENSE for details.
* SPDX-License-Identifier: BSD-3-Clause
*************************************************/

#include <liblogging.h>
#include <libfpga_internal/libfpgacommon_internal.h>

#include <stdint.h>
#include <stdbool.h>
#include <time.h>
#include <errno.h>

#undef FPGA_LOGGER_LIBNAME
#define FPGA_LOGGER_LIBNAME "libcommon:  "


static struct timespec __convert_time_val2spec(
  const struct timeval *val
) {
  return (struct timespec){
    .tv_sec = val ? val->tv_sec
                      : 0,
    .tv_nsec = val ? (val->tv_usec * 1000)
                       : 0
  };  // NOLINT
}


static int64_t __get_remain_ns(
  const struct timespec *spec_elapsed,
  const struct timespec *spec_target
) {
  return (spec_target->tv_sec  - spec_elapsed->tv_sec) * 1000L * 1000L * 1000L
       + (spec_target->tv_nsec - spec_elapsed->tv_nsec);
}


int __fpga_common_polling(
  const struct timeval *timeout,
  const struct timeval *interval,
  int (*clb)(void*),
  void *arg
) {
  if (!clb) {
    llf_err(INVALID_ARGUMENT, "%s(timeout(%#lx), interval(%#lx), clb(%#lx), arg(%#lx))\n",
      (uintptr_t)timeout, (uintptr_t)interval, (uintptr_t)clb, (uintptr_t)arg);
    return -INVALID_ARGUMENT;
  }
  if (timeout)
    llf_dbg(" timeout[s]  : %ld.%06ld\n", timeout->tv_sec, timeout->tv_usec);
  else
    llf_dbg(" timeout[s]  : %ld.%06ld\n", 0, 0);
  if (interval)
    llf_dbg(" interval[s] : %ld.%06ld\n", interval->tv_sec, interval->tv_usec);
  else
    llf_dbg(" interval[s] : %ld.%06ld\n", 0, 0);

  int ret;
  struct timespec to = __convert_time_val2spec(timeout);
  struct timespec iv = __convert_time_val2spec(interval);
  struct timespec req, rem;
  struct timespec elapsed_time = {.tv_sec = 0, .tv_nsec = 0};
  struct timespec t1, t2;

  while (1) {
    // Execute Callback Function
    ret = clb(arg);
    if (ret <= 0)
      break;

    // Timeout
    if (__get_remain_ns(&elapsed_time, &to) <= 0) {
      llf_dbg(" Timeout of polling...\n");
      llf_dbg("  elapsed time[sec]=%ld.%09ld\n", elapsed_time.tv_sec, elapsed_time.tv_nsec);
      break;
    }

    // Sleep during the interval
    req = iv;
    clock_gettime(CLOCK_REALTIME, &t1);
    while (clock_nanosleep(CLOCK_REALTIME, 0, &req, &rem) == EINTR) {
      // When there is interruption, continue to sleep
      req.tv_sec = rem.tv_sec;
      req.tv_nsec = rem.tv_nsec;
    }
    clock_gettime(CLOCK_REALTIME, &t2);
    int64_t up_ns = elapsed_time.tv_nsec + (t2.tv_nsec - t1.tv_nsec);
    elapsed_time.tv_sec += (t2.tv_sec - t1.tv_sec) + (up_ns / (1000L * 1000L * 1000L));
    elapsed_time.tv_nsec = up_ns % (1000L * 1000L * 1000L);
  }

  return ret;
}
