/*************************************************
* Copyright 2024 NTT Corporation, FUJITSU LIMITED
* Licensed under the 3-Clause BSD License, see LICENSE for details.
* SPDX-License-Identifier: BSD-3-Clause
*************************************************/
#include <libfpgactl.h>
#include <liblogging.h>

#include <libpower.h>

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <getopt.h>
#include <time.h>

#ifndef APP_VERSION
#define APP_VERSION "x.x.x"
#endif

#define PRINT_COLS      \
"Timestamp, "           \
"Elapsed time[ms], "    \
"pcie_12V_voltage[V], " \
"pcie_12V_current[A], " \
"pcie_12V_power[W], "   \
"aux_12V_voltage[V], "  \
"aux_12V_current[A], "  \
"aux_12V_power[W], "    \
"total_power(12V)[W], " \
"PEX_3V3_voltage[V], "  \
"PEX_3V3_current[A], "  \
"PEX_3V3_power[W], "    \
"AUX_3V3_voltage[V], "  \
"AUX_3V3_current[A], "  \
"VCCINT_voltage[V], "   \
"VCCINT_current[A]"     \
"\n"

#define PRINT_FMT       \
"%04d/%02d/%02d "       \
"%02d:%02d:%02d.%03ld, "\
"%ld, "                 \
"%.3f, "                \
"%.3f, "                \
"%.3f, "                \
"%.3f, "                \
"%.3f, "                \
"%.3f, "                \
"%.3f, "                \
"%.3f, "                \
"%.3f, "                \
"%.3f, "                \
"%.3f, "                \
"%.3f, "                \
"%.3f, "                \
"%.3f"                  \
"\n"


static unsigned int m_interval = 100;


static void print_usage(void) {
  printf("get_fpga_power: version %s\n", APP_VERSION);
  printf("usage: ./get_fpga_power -d <device file> [-- -i <interval>]\n");
  printf("interval:default=100[ms]\n");
  printf("\n");
}

static const struct option long_options[] = {
    { "interval", required_argument, NULL, 'i' },
    { "help", no_argument, NULL, 'h' },
    { NULL, 0, 0, 0 },
};

static const char short_options[] = {
    "i:h"
};

static int parse_args(
  int argc,
  char **argv
) {
  int opt, ret;
  char **argvopt;
  int option_index;
  char *prgname = argv[0];
  const int old_optind = optind;
  const int old_optopt = optopt;
  char * const old_optarg = optarg;

  argvopt = argv;
  optind = 1;
  opterr = 0;

  while ((opt = getopt_long(argc, argvopt, short_options,
                long_options, &option_index)) != EOF
  ) {
    switch (opt) {
    case 'h':
      print_usage();
      exit(0);
    case 'i': {
      m_interval = atoi(optarg);
      break;
    }
    default:
      printf("Cannot parse option : %s\n", argvopt[optind - 1]);
      ret = -1;
      goto out;
    }
  }

  if (optind >= 0)
    argv[optind - 1] = prgname;
  ret = optind - 1;

out:
  optind = old_optind;
  optopt = old_optopt;
  optarg = old_optarg;

  return ret;
}

/* ******** *
 * * main * *
 * ******** */
int main(int argc, char **argv)
{
  // set log
  libfpga_log_set_output_stdout();
  libfpga_log_quit_timestamp();
  libfpga_log_set_level(LIBFPGA_LOG_NOTHING);

  // init fpga
  int ret;
  ret = fpga_init(argc, argv);
  if (ret <= 0) {
    printf("Error happened at fpga_init(): ret=%d\n", ret);
    print_usage();
    return -1;
  }
  argc -= ret;
  argv += ret;

  // parse options
  ret = parse_args(argc, argv);
  if (ret < 0) {
    printf("Failed to parse options...\n");
    fpga_finish();
    return -1;
  }

  // init fpga
  if ((ret = fpga_get_num()) != 1) {
    printf("FPGA num(%d) is invalid...\n", ret);
    print_usage();
    return -1;
  }
  uint32_t dev_id = 0;

  // reset cms
  if((ret = fpga_set_cms_unrest(dev_id))) {
    printf("Error happened at fpga_set_cms_unrest(): ret=%d\n", ret);
    fpga_finish();
    return -1;
  }

  unsigned int u_interval = m_interval * 1000;
  long elapsed_msec;
  struct timespec ts;
  struct tm tm;
  struct timespec old_ts;
  struct tm old_tm;

  double pcie_12V_voltage;
  double pcie_12V_current;
  double aux_12V_voltage;
  double aux_12V_current;
  double pex_3V3_voltage;
  double pex_3V3_current;
  double pex_3V3_power;
  double aux_3V3_voltage;
  double aux_3V3_current;
  double vccint_voltage;
  double vccint_current;
  double pcie_12V_power;
  double aux_12V_power;
  double total_power_12V;

  fpga_power_info_t power_info;

  // get first time
  clock_gettime(CLOCK_REALTIME, &old_ts);
  localtime_r( &old_ts.tv_sec, &old_tm);

  // Print header
  printf(PRINT_COLS);

  while (1) {
    if((ret = fpga_get_power(dev_id, &power_info))) {
      printf("Error happened at fpga_get_power(): ret=%d\n", ret);
      fpga_finish();
      return -1;
    }

    // get time
    clock_gettime(CLOCK_REALTIME, &ts);
    localtime_r( &ts.tv_sec, &tm);
    elapsed_msec = (ts.tv_sec - old_ts.tv_sec) * 1000
                 + (ts.tv_nsec - old_ts.tv_nsec) / 1000000;

    // get power
    pcie_12V_voltage  = (double)(power_info.pcie_12V_voltage) / 1000;
    pcie_12V_current  = (double)(power_info.pcie_12V_current) / 1000;
    pcie_12V_power    = pcie_12V_voltage * pcie_12V_current;
    aux_12V_voltage   = (double)(power_info.aux_12V_voltage)  / 1000;
    aux_12V_current   = (double)(power_info.aux_12V_current)  / 1000;
    aux_12V_power     = aux_12V_voltage * aux_12V_current;
    total_power_12V   = pcie_12V_power + aux_12V_power;
    pex_3V3_voltage   = (double)(power_info.pex_3V3_voltage)  / 1000;
    pex_3V3_current   = (double)(power_info.pex_3V3_current)  / 1000;
    pex_3V3_power     = (double)(power_info.pex_3V3_power)    / 1000;
    aux_3V3_voltage   = (double)(power_info.aux_3V3_voltage)  / 1000;
    aux_3V3_current   = (double)(power_info.aux_3V3_current)  / 1000;
    vccint_voltage    = (double)(power_info.vccint_voltage)   / 1000;
    vccint_current    = (double)(power_info.vccint_current)   / 1000;

    printf(PRINT_FMT,
      tm.tm_year + 1900,
      tm.tm_mon + 1,
      tm.tm_mday,
      tm.tm_hour,
      tm.tm_min,
      tm.tm_sec,
      ts.tv_nsec / 1000000,
      elapsed_msec,
      pcie_12V_voltage,
      pcie_12V_current,
      pcie_12V_power,
      aux_12V_voltage,
      aux_12V_current,
      aux_12V_power,
      total_power_12V,
      pex_3V3_voltage,
      pex_3V3_current,
      pex_3V3_power,
      aux_3V3_voltage,
      aux_3V3_current,
      vccint_voltage,
      vccint_current
    );

    usleep(u_interval);
  }

  return 0;
}
