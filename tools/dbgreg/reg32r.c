/*************************************************
* Copyright 2024 NTT Corporation, FUJITSU LIMITED
* Licensed under the 3-Clause BSD License, see LICENSE for details.
* SPDX-License-Identifier: BSD-3-Clause
*************************************************/
#include <libfpgactl.h>
#include <liblogging.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <limits.h> //UINT_MAX

#define READ_MAX_SIZE   0x4000
#define WRITE_MAX_SIZE  64
#define REG_ACCESS_MAX  0x140000

#ifndef APP_VERSION
#define APP_VERSION "<invalid_version>"
#endif

static void print_usage(void) {
  printf("reg32r (build.%s)\n", APP_VERSION);
  printf("  Usage: reg32r <device_file_name|serial_id> <address(hex:0-%#x)> [size(dec)]\n",
    REG_ACCESS_MAX - 1);
}
static int reg_read(int fd, uint32_t addr, uint32_t *value);

/* ******** *
 * * main * *
 * ******** */
int main(int argc, char **argv)
{
  char *device_file = NULL;
  uint32_t dev_id;
  fpga_device_t *dev;

  int i;
  int setting_data_num;

  int fd;
  uint32_t addr;
  uint32_t data;
  uint32_t size = 4;  // default read size is 4byte

  // suppress create libfpga log-file
  libfpga_log_set_level(LIBFPGA_LOG_NOTHING);

  //parameter check
  if (argc != 4 && argc != 3){
    // should be 3 or 4 argument
    print_usage();
    return -1;
  }
  if (argc - 3 > READ_MAX_SIZE ){
    // should be smaller than READ_MAX_SIZE(0x4000byte)
    printf(" data size error!\n");
    print_usage();
    return -1;
  }

  // get device file name
  device_file = argv[1];
  if (fpga_dev_init(device_file, &dev_id)) {
    printf(" Failure open %s\n", device_file);
    return -1;
  }
  if (fpga_enable_regrw(dev_id)) {
    printf("something wrong...\n");
    goto finish;
  }
  dev = fpga_get_device(dev_id);
  if (!dev) {
    printf("something wrong...\n");
    goto finish;
  }
  fd = dev->fd;

  // get address from cmdline as hex
  addr = strtoul(argv[2], NULL, 16);

  // get read size from cmdline as dec
  if (argc == 4) {
    size = strtoul(argv[3], NULL, 10);
    if (size < 4)
      size = 4;
  }

  // print header
  if (size > 12){
    printf("offset:        0        4        8       12\n");
  } else if (size > 8){
    printf("offset:        0        4        8\n");
  } else if (size > 4){
    printf("offset:        0        4\n");
  } else {
    printf("offset:        0\n");
  }

  //read
  addr = (addr/4)*4;
  printf(" %04X : ", addr);
  for (i=0; i<size/4; i++) {
    if (reg_read(fd, addr, &data)) {
      printf("ERROR at reg_read()! (address)=(%#010x)\n", addr);
      goto finish;
    }
    if ( (i > 0) && ((i % 4) == 0) ) {
      printf("\n");
      printf(" %04X : ", addr);
    }
    addr += 4;
    printf("%08x ", data);
  }
  printf("\n");


finish:
  fpga_finish();

  return 0;
}


//---------------------------------------
//  FPGA register access
//---------------------------------------
static int reg_read(int fd, uint32_t addr, uint32_t *value)
{
  if (addr > REG_ACCESS_MAX){
    printf(" address error(%#010X)\n",addr);
    return -1;
  }
  int n = pread(fd, value, sizeof(uint32_t), addr);
  if (n != sizeof(uint32_t)) {
      return -1;
  }

  return 0;
}


