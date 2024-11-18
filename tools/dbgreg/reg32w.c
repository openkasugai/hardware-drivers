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
  printf("reg32w (build.%s)\n", APP_VERSION);
  printf("  Usage: reg32w <device_file_name|serial_id> <address(hex:0-%#x)> <data(hex)> [data(hex)]...\n",
    REG_ACCESS_MAX - 1);
}
static int reg_write(int fd, uint32_t addr, uint32_t value);

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
  uint32_t data[WRITE_MAX_SIZE];

  // suppress create libfpga log-file
  libfpga_log_set_level(LIBFPGA_LOG_NOTHING);

  //parameter check
  if (argc < 4){
    // should be larger than 4
    print_usage();
    return -1;
  }
  if (argc - 3 > WRITE_MAX_SIZE ){
    // should be smaller than WRITE_MAX_SIZE(possible value: WRITE_MAX_SIZE*4byte)
    printf(" data num over error!\n");
    print_usage();
    return -1;
  }

  // get device file name and open ang get fd
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

  // get setting value from cmdline as hex
  setting_data_num = argc -3;
  for (i = 0; i < setting_data_num ; i++){
    data[i] = strtoul(argv[i+3], NULL, 16);
  }

  // read
  addr = (addr/4)*4;
  for (i = 0; i < setting_data_num ; i++){
    //write
    if (reg_write(fd, addr, data[i])) { 
      printf("ERROR at reg_write()! (address,data)=(%#010x,%#010x)\n", addr,data[i]);
      goto finish;
    }
    addr += 4;
  }


finish:
  fpga_finish();

  return 0;
}


//---------------------------------------
//  FPGA register access
//---------------------------------------
static int reg_write(int fd, uint32_t addr, uint32_t value)
{
  if (addr > REG_ACCESS_MAX){
    printf(" address error(%#010X)\n",addr);
    return -1;
  }
  int n = pwrite(fd, &value, sizeof(uint32_t), addr);
  if (n != sizeof(uint32_t)) {
      return -1;
  }

  return 0;
}


