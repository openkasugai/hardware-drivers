/*************************************************
* Copyright 2024 NTT Corporation, FUJITSU LIMITED
* Licensed under the 3-Clause BSD License, see LICENSE for details.
* SPDX-License-Identifier: BSD-3-Clause
*************************************************/

#include <libfpgactl.h>
#include <liblogging.h>

#include <libfpga_internal/libfpgactl_internal.h>
#include <libfpga_internal/libfpgautil.h>
#include <libfpga_internal/libfpga_json.h>

#include <pciaccess.h>  // libpciaccess-dev

#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <dirent.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>


// LogLibFpga
#undef FPGA_LOGGER_LIBNAME
#define FPGA_LOGGER_LIBNAME LIBFPGACTL


/**
 * static global variable: long option table for fpga_init()
 */
static const struct option fpga_long_options[] = {
    { "device", required_argument, NULL, 'd' },
    { NULL, 0, 0, 0 },
};

/**
 * static global variable: short option table for fpga_init()
 */
static const char fpga_short_options[] = {
    "d:"    /* specify device file */
};

/**
 * static global variable: table corresponding card_id with card_name.
 */
static const struct fpga_card_table_elem fpga_available_card_table[] = {
  // { card_id, card_name }
  {FPGA_CARD_U250, CARD_NAME_ALVEO_U250},
  {FPGA_CARD_U250, CARD_NAME_ALVEO_U250_ACT},
  {FPGA_CARD_U280, CARD_NAME_ALVEO_U280}
};

/**
 * static global variable: device management list
 */
static fpga_device_t devices[FPGA_MAX_DEVICES];

/**
 * static global variable: num of opening devices
 */
static int devices_num = 0;

/**
 * static global variable: temporary variable used in fpga_init()
 */
static char **device_name = NULL;

/**
 * static global variable: file path used in fpga_get_device_config
 */
static char fpga_bitstream_config_table_file[FPGA_FILE_PATH_MAX] = FPGA_CONFIG_JSON_PATH;


/**
 * @brief Initialize device management list(`devices`) at first once.
 */
static void __libfpgactl_init(void) {
  static bool init_once = true;
  if (init_once) {
    memset(devices, 0, sizeof(devices));
    init_once = !init_once;
  }
}


/**
 * @brief Get the smallest free index from device management list
 */
static int __fpga_get_smallest_devices_index(void) {
  for (int index = 0; index < FPGA_MAX_DEVICES; index++) {
    if (!devices[index].name) {
      return index;
    }
  }
  return -1;
}


/**
 * @brief Function which initialize FPGA
 */
static int __fpga_dev_init(
  const char *name,
  uint32_t *dev_id
) {
  // clean devices[] array at first
  __libfpgactl_init();

  if (!name)
    return -INVALID_ARGUMENT;

  // Get/Create device file name
  char *tmp_filename = NULL;
  if (*name == '/') {
    // When the first charcter of <name> is '/', open file by <name>
    tmp_filename = strdup(name);
    if (!tmp_filename) {
      llf_err(FAILURE_MEMORY_ALLOC, "Failed to allocate memory for temporary filename.\n");
      return -FAILURE_MEMORY_ALLOC;
    }
  } else {
    // When the first charcter of <name> is NOT '/', open file by <PREFIX><name>
    tmp_filename = (char*)malloc(sizeof(char) * FILENAME_MAX); //NOLINT
    if (!tmp_filename) {
      llf_err(FAILURE_MEMORY_ALLOC, "Failed to allocate memory for temporary filename.\n");
      return -FAILURE_MEMORY_ALLOC;
    }
    snprintf(tmp_filename, FILENAME_MAX, "%s%s", FPGA_DEVICE_PREFIX, name);
  }

  // When the same name device is already opened, return 0 and print warning log
  for (int cnt = 0; cnt < FPGA_MAX_DEVICES; cnt++) {
    if (devices[cnt].name) {
      if (0 == strcmp(&tmp_filename[strlen(FPGA_DEVICE_PREFIX)], devices[cnt].name)) {
        llf_warn(ALREADY_INITIALIZED, "Detect %s is already opening.\n", tmp_filename);
        free(tmp_filename);
        return 0;
      }
    }
  }

  fpga_device_t *dev = NULL;
  uint32_t minor;
  int fd;
  int index;
  int ret = 0;
  static uint32_t xpcie_driver_version = -1;

  // Open device file
  fd = fpgautil_open(tmp_filename, O_RDWR);
  if (fd < 0) {
    llf_err(FAILURE_DEVICE_OPEN, "Failed to open device file:%s\n", tmp_filename);
    free(tmp_filename);
    return -FAILURE_DEVICE_OPEN;
  }

  // Get free index of devices[]
  index = __fpga_get_smallest_devices_index();
  if (index < 0) {
    // There is no free index in devices[]
    ret = -FULL_ELEMENT;
    llf_err(FULL_ELEMENT, "Invalid operation: availabe FPGA num is %d\n", FPGA_MAX_DEVICES);
    // Dump file names
    for (index = 0; index < FPGA_MAX_DEVICES; index++)
      llf_err(FULL_ELEMENT, "  name[%02d]:%s\n",
        index, devices[index].name ? devices[index].name : "<null?>");
    goto err_out;
  }

  dev = &devices[index];

  if (xpcie_driver_version == -1) {
    // Driver and libfpgactl will never change when using, so get/print once
    // Print library version
    llf_info("Library Name           : %s\n", LIBRARY_NAME);
    llf_info("Library type           : %#04x\n", LIBRARY_TYPE);
    llf_info("Library version(major) : %#04x\n", LIBRARY_VERSION1);
    llf_info("Library version(minor) : %#04x\n", LIBRARY_VERSION2);
    llf_info("Library revision       : %#04x%02x\n", LIBRARY_REVISION_H, LIBRARY_REVISION_L);

    // Get driver version
    if (fpgautil_ioctl(fd, XPCIE_DEV_DRIVER_GET_VERSION, &xpcie_driver_version) < 0) {
      int err = errno;
      llf_err(FAILURE_IOCTL, "Failed to ioctl XPCIE_DEV_DRIVER_GET_VERSION(errno:%d)\n", err);
      ret = -FAILURE_IOCTL;
      goto err_out;
    }
    llf_info("Driver version(major) : %#04x\n", xpcie_driver_version >> 24);
    llf_info("Driver version(minor) : %#04x\n", (xpcie_driver_version >> 16) & 0xFF);
    llf_info("Driver revision       : %#06x\n", xpcie_driver_version & 0xFFFF);
  }

  // Get FPGA MINOR number
  if (fpgautil_ioctl(fd, XPCIE_DEV_DRIVER_GET_DEVICE_ID, &minor) < 0) {
    int err = errno;
    llf_err(FAILURE_IOCTL, "Failed to ioctl XPCIE_DEV_DRIVER_GET_DEVICE_ID(errno:%d)\n", err);
    ret = -FAILURE_IOCTL;
    goto err_out;
  }

  // Set data into devices[]
  dev->name = strdup(tmp_filename+strlen(FPGA_DEVICE_PREFIX));
  if (dev->name == NULL) {
    llf_err(FAILURE_MEMORY_ALLOC, "Failed to allocate memory for device serial_id.\n");
    ret = -FAILURE_MEMORY_ALLOC;
    goto err_out;
  }
  dev->fd = fd;
  dev->dev_id = minor;
  dev->task_id = 1;

  // Get FPGA's data from driver
  ret = fpga_update_info(index);
  if (ret) {
    llf_err(-ret, "Failed to get information of %s\n", dev->name);
    goto err_out;
  }

  // Increment opening device num
  devices_num++;

  free(tmp_filename);

  if (dev_id)
    *dev_id = index;

  return ret;

err_out:
  if (tmp_filename)
    free(tmp_filename);
  if (dev && dev->name)
    free(dev->name);
  if (dev)
    memset(dev, 0, sizeof(fpga_device_t));
  fpgautil_close(fd);

  return ret;
}


int fpga_dev_init(
  const char *name,
  uint32_t *dev_id
) {
  if (!name || !dev_id) {
    llf_err(INVALID_ARGUMENT, "%s(%s, %#x)\n",
      __func__, name ? name : "<null>", (uintptr_t)dev_id);
    return -INVALID_ARGUMENT;
  }
  llf_dbg("%s(name(%s), dev_id(%#x))\n", __func__, name, (uintptr_t)dev_id);

  return __fpga_dev_init(name, dev_id);
}


int fpga_dev_simple_init(
  const char *name
) {
  if (!name) {
    llf_err(INVALID_ARGUMENT, "%s(name(<null>))\n", __func__);
    return -INVALID_ARGUMENT;
  }

  llf_dbg("%s(name(%s))\n", __func__, name);

  return __fpga_dev_init(name, NULL);
}


/**
 * @brief Parse argument using delimiter(',')
 */
static void __parse_devname_string(
  char *string
) {
  char *name;
  int string_len = strlen(string);
  int i;

  for (i = 0; i < string_len; i++) {
    if (string[i] == ',')
      string[i] = '\0';
  }

  if (strlen(string) > 0) {
    name = strdup(string);
    if (name)
      device_name[devices_num++] = name;
  }

  for (i = 0; i < string_len; i++) {
    if (string[i] == '\0') {
      if (strlen(&string[i + 1]) > 0) {
        name = strdup(&string[i + 1]);
        if (name)
          device_name[devices_num++] = name;
      }
    }
  }
}


/**
 * @brief Parse command line arguments and handle options
 */
static int __parse_args(
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

  while ((opt = getopt_long(argc, argvopt, fpga_short_options,
                fpga_long_options, &option_index)) != EOF) {
    switch (opt) {
    case 'd': {
      char *string = strdup(optarg);
      if (string == NULL) {
        llf_err(FAILURE_MEMORY_ALLOC, "Failed to allocate memory for device name\n");
        ret = -FAILURE_MEMORY_ALLOC;
        goto out;
      }
      __parse_devname_string(string);
      free(string);
      break;
    }
    default:
      llf_err(INVALID_ARGUMENT, "Invalid operation: unable to parse option[%s].\n", argvopt[optind - 1]);
      ret = -INVALID_ARGUMENT;
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


int fpga_init(
  int argc,
  char **argv
) {
  log_libfpga_cmdline_arg(LIBFPGA_LOG_DEBUG, argc, argv, LIBFPGACTL "%s", __func__);

  int ret;
  int devcount = 0;
  int i;

  if (devices_num > 0) {
    llf_warn(ALREADY_INITIALIZED, "Already initialized FPGA. devices_num:%d\n", devices_num);
    return -ALREADY_INITIALIZED;
  }

  // Allocate memory to store temporary file name
  device_name = (char**)malloc(sizeof(char *) * FPGA_MAX_DEVICES);  // NOLINT
  if (device_name == NULL) {
    llf_err(FAILURE_MEMORY_ALLOC, "Failed to allocate memory for device name list\n");
    return -FAILURE_MEMORY_ALLOC;
  }

  // Parse options
  ret = __parse_args(argc, argv);
  if (ret < 0) {
    llf_err(INVALID_ARGUMENT, "Failed to parse LIBFPGA log options\n");
    free(device_name);
    return -INVALID_ARGUMENT;
  }

  // devices_num is stored the num of parsed names
  devcount = devices_num;
  devices_num = 0;
  for (i = 0; i < devcount; i++) {
    // Initialize FPGA with parsed names
    // In fpga_dev_simple_init(), devices_num will be incremented if succeed to be initialized
    fpga_dev_simple_init(device_name[i]);
    free(device_name[i]);
  }

  free(device_name);
  device_name = NULL;

  // Only when there are no FPGAs succeeded to be initialized, return error.
  // When the num of parsed device name is 2, and the num of opened device is 1, return success.
  if (devices_num == 0) {
    llf_err(NO_DEVICES, "No FPGA available.\n");
    return -NO_DEVICES;
  }

  // Return the num of parsed options.
  return ret;
}


static int __fpga_get_dev_id(
  const char *name,
  uint32_t *dev_id
) {
  if (!name || !dev_id) {
    llf_err(INVALID_ARGUMENT, "%s(name(%s), dev_id(%#lx))\n",
      __func__, name ? name : "<null>", (uintptr_t)dev_id);
    return -INVALID_ARGUMENT;
  }
  for (int index = 0; index < FPGA_MAX_DEVICES; index++) {
    if (devices[index].name) {
      if (*name == '/') {
        int len_prefix = strlen(FPGA_DEVICE_PREFIX);
        int len_name = strlen(name);
        if (len_name <= len_prefix)
          break;
        if (!strcmp(devices[index].name, &name[len_prefix])) {
          *dev_id = index;
          return 0;
        }
      } else {
        if (!strcmp(devices[index].name, name)) {
          *dev_id = index;
          return 0;
        }
      }
    }
  }
  return -NOT_INITIALIZED;
}


int fpga_get_dev_id(
  const char *name,
  uint32_t *dev_id
) {
  if (!name || !dev_id) {
    llf_err(INVALID_ARGUMENT, "%s(name(%s), dev_id(%#lx))\n",
      __func__, name ? name : "<null>", (uintptr_t)dev_id);
    return -INVALID_ARGUMENT;
  }

  int ret = __fpga_get_dev_id(name, dev_id);

  if (ret)
    llf_err(-ret, "Failed to get dev_id.\n");

  return ret;
}


int fpga_finish(void) {
  llf_dbg("%s()\n", __func__);

  if (devices_num == 0) {
    llf_err(NOT_INITIALIZED, "Invalid operation: No FPGA initialized.\n");
    return -NOT_INITIALIZED;
  }

  for (int i = 0; i < FPGA_MAX_DEVICES; i++)
    if (devices[i].name)
      fpga_dev_finish(i);

  return 0;
}


int fpga_dev_finish(
  uint32_t dev_id
) {
  fpga_device_t *dev = fpga_get_device(dev_id);
  if (!dev) {
    llf_err(INVALID_ARGUMENT, "%s(dev_id(%u))\n", __func__, dev_id);
    return -INVALID_ARGUMENT;
  }
  llf_dbg("%s(dev_id(%u))\n", __func__, dev_id);

  free(dev->name);
  fpgautil_close(dev->fd);
  memset(dev, 0, sizeof(fpga_device_t));

  // Decrement opening device num
  devices_num--;

  return 0;
}


/**
 * @brief Function which get FPGA device management information for libfpga(not for user)
 * @details In future, fpga_device_t will be typedefed with void* type, as is often the case in common libraries.
 */
fpga_device_t *fpga_get_device(
  uint32_t dev_id
) {
  if (dev_id >= FPGA_MAX_DEVICES) {
    return NULL;
  }
  if (!devices[dev_id].name) {
    return NULL;
  }
  return &devices[dev_id];
}


int fpga_get_num(void) {
  return devices_num;
}


int fpga_update_info(
  uint32_t dev_id
) {
  fpga_device_t *dev = fpga_get_device(dev_id);
  if (!dev) {
    llf_err(INVALID_ARGUMENT, "%s(dev_id(%u))\n", __func__, dev_id);
    return -INVALID_ARGUMENT;
  }
  llf_dbg("%s(dev_id(%u))\n", __func__, dev_id);

  fpga_card_info_t *info = &dev->info;

  // Update child bitstream information
  if (fpgautil_ioctl(dev->fd, XPCIE_DEV_GLOBAL_UPDATE_MAJOR_VERSION, NULL)) {
    int err = errno;
    llf_err(FAILURE_IOCTL, "Failed to ioctl XPCIE_DEV_GLOBAL_UPDATE_MAJOR_VERSION(errno:%d)\n", err);
    return -FAILURE_IOCTL;
  }

  // Get information as PCI device
  if (fpgautil_ioctl(dev->fd, XPCIE_DEV_DRIVER_GET_DEVICE_INFO, info)) {
    int err = errno;
    llf_err(FAILURE_IOCTL, "Failed to ioctl XPCIE_DEV_DRIVER_GET_DEVICE_INFO(errno:%d)\n", err);
    return -FAILURE_IOCTL;
  }

  // Get information of FPGA type
  if (fpgautil_ioctl(dev->fd, XPCIE_DEV_DRIVER_GET_FPGA_TYPE, &dev->info.ctrl_type)) {
    int err = errno;
    llf_err(FAILURE_IOCTL, "Failed to ioctl XPCIE_DEV_DRIVER_GET_FPGA_TYPE(errno:%d)\n", err);
    return -FAILURE_IOCTL;
  }

  // Get information of FPGA address map
  if (fpgautil_ioctl(dev->fd, XPCIE_DEV_DRIVER_GET_FPGA_ADDR_MAP, &dev->map)) {
    int err = errno;
    llf_err(FAILURE_IOCTL, "Failed to ioctl XPCIE_DEV_DRIVER_GET_FPGA_ADDR_MAP(errno:%d)\n", err);
    return -FAILURE_IOCTL;
  }

  // Print information
  llf_info("FPGA[%02u] BSID(p)  : %08x\n", dev_id, info->bitstream_id.parent);
  llf_info("FPGA[%02u] BSID(c)  : %08x\n", dev_id, info->bitstream_id.child);

  llf_info("FPGA[%02u] device   : %#06x\n", dev_id, info->pci_device_id);
  llf_info("FPGA[%02u] vendor   : %#06x\n", dev_id, info->pci_vendor_id);

  llf_info("FPGA[%02u] bus_id   : %04x:%02x:%02x.%01x\n", dev_id,
    info->pci_domain, info->pci_bus, info->pci_dev, info->pci_func);

  llf_info("FPGA[%02u] card_name: %s\n", dev_id, info->card_name);

  return 0;
}


int fpga_scan_devices(void) {
  llf_dbg("%s()\n", __func__);

  int scan_devices = 0;

  // The file name is xpcie<id> or xpcie_<serial_id>
  // A macro determines which names can be supported when library is built.
  int file_prefix_len = strlen(FPGA_DEVICE_PREFIX) - strlen(FPGA_DEVICE_DIR);
  const char *file_name = FPGA_DEVICE_PREFIX + strlen(FPGA_DEVICE_DIR);

  // Open the directory(FPGA_DEVICE_DIR)
  DIR *dir = opendir(FPGA_DEVICE_DIR);
  struct dirent *ent;
  if (!dir) {
    int err = errno;
    llf_err(FAILURE_OPEN, "Failed to open directory %s(errno:%d)\n", FPGA_DEVICE_DIR, err);
    return -FAILURE_OPEN;
  }

  // Scan all files in the FPGA_DEVICE_DIR
  for (ent = readdir(dir); ent; ent = readdir(dir)) {
    int is_exist = !strncmp(ent->d_name, file_name, file_prefix_len);
    if (is_exist) {
      uint32_t get_dev_id = (uint32_t)-1;
      int is_open = __fpga_get_dev_id(&ent->d_name[file_prefix_len], &get_dev_id);
      if (!is_open) {
        // When the device is already opened, only update device.
        fpga_update_info(get_dev_id);
      } else {
        // When the device is not opened, initialize FPGA
        is_open = __fpga_dev_init(&ent->d_name[file_prefix_len], NULL);
        if (is_open) {
          // Skip incrementing scan_devices when failed to initialize FPGA
          continue;
        }
      }
      scan_devices++;
    }
  }

  closedir(dir);

  if (scan_devices == 0) {
    llf_pr("Invalid operation: Maybe No FPGA or No xpcie driver.\n");
  }

  if (scan_devices != devices_num) {
    llf_pr("Fatal error: Something ERROR is happening.\n");
    return -LIBFPGA_FATAL_ERROR;
  }

  return scan_devices;
}


int fpga_get_device_list(
  char ***device_list
) {
  int ret;

  if (!device_list) {
    llf_err(INVALID_ARGUMENT, "%s(device_list(%#lx))\n", __func__, (uintptr_t)device_list);
    return -INVALID_ARGUMENT;
  }
  llf_dbg("%s(device_list(%#lx))\n", __func__, (uintptr_t)device_list);

  if (devices_num == 0) {
    llf_err(NOT_INITIALIZED, "Invalid operation: No FPGA is initialized\n");
    return -NOT_INITIALIZED;
  }

  // Allocate memory for setntinel and devices
  int list_length = sizeof(char*) * devices_num + 1;
  char **tmp_dev_list = (char**)malloc(list_length);  // NOLINT
  if (!tmp_dev_list) {
    llf_err(FAILURE_MEMORY_ALLOC, "Failed to allocate memory for device list\n");
    return -FAILURE_MEMORY_ALLOC;
  }
  memset(tmp_dev_list, 0, list_length);  // The sentinel is NULL

  int list_index = 0;
  for (int index = 0; index < FPGA_MAX_DEVICES; index++) {
    if (devices[index].name) {
      char *tmp_dev = strdup(devices[index].name);
      if (!tmp_dev) {
        llf_err(FAILURE_MEMORY_ALLOC, "Failed to allocate memory for device name.\n");
        ret = -FAILURE_MEMORY_ALLOC;
        goto failed;
      }
      tmp_dev_list[list_index++] = tmp_dev;
    }
  }

  if (list_index != devices_num)
    llf_warn(LIBFPGA_FATAL_ERROR, "Fatal error: Something ERROR is happening.\n");

  *device_list = tmp_dev_list;

  return 0;

failed:
  for (int index = 0; index < list_index; index++)
    if (tmp_dev_list[index])
      free(tmp_dev_list[index]);

  free(tmp_dev_list);
  return ret;
}


// cppcheck-suppress unusedFunction
int fpga_release_device_list(
  char **device_list
) {
  if (!device_list) {
    llf_err(INVALID_ARGUMENT, "%s(device_list(%#lx))\n", __func__, (uintptr_t)device_list);
    return -INVALID_ARGUMENT;
  }
  llf_dbg("%s(device_list(%#lx))\n", __func__, (uintptr_t)device_list);

  for (int index = 0; device_list[index]; index++)
    free(device_list[index]);

  free(device_list);

  return 0;
}


// cppcheck-suppress unusedFunction
int fpga_get_device_info(
  uint32_t dev_id,
  fpga_device_user_info_t *info
) {
  // input check
  fpga_device_t *dev = fpga_get_device(dev_id);
  if (!dev || !info) {
    llf_err(INVALID_ARGUMENT, "%s(dev_id(%u), info(%#lx))\n",
      __func__, dev_id, (uintptr_t)info);
    return -INVALID_ARGUMENT;
  }
  llf_dbg("%s(dev_id(%u), info(%#lx))\n", __func__, dev_id, (uintptr_t)info);

  // init data
  fpga_device_user_info_t tmp_info;
  memset(&tmp_info, 0, sizeof(tmp_info));

  // create device_file_path
  snprintf(tmp_info.device_file_path, FPGA_FILE_PATH_MAX,
    "%s%s", FPGA_DEVICE_PREFIX, dev->name);

  // create device_index
  tmp_info.device_index = dev->dev_id;

  // create vendor name( or get vendor id value as string when failed)
  // libpciaccess
  int ret = pci_system_init();
  if (!ret) {
    struct pci_device *pdev = pci_device_find_by_slot(
      dev->info.pci_domain,
      dev->info.pci_bus,
      dev->info.pci_dev,
      dev->info.pci_func);
    if (pdev) {
      const char *vendor_name = pci_device_get_vendor_name(pdev);
      if (vendor_name)
        strncpy(
          tmp_info.vendor,
          vendor_name,
          sizeof(tmp_info.vendor) - 1);
    }
    pci_system_cleanup();
  }
  if (!tmp_info.vendor[0])
    snprintf(tmp_info.vendor, sizeof(tmp_info.vendor), "%x", dev->info.pci_vendor_id);

  // create device_type
  strcpy(tmp_info.device_type, dev->info.card_name);

  // create pcie_bus
  tmp_info.pcie_bus.domain  = (int)dev->info.pci_domain;  // NOLINT
  tmp_info.pcie_bus.bus     = (int)dev->info.pci_bus;     // NOLINT
  tmp_info.pcie_bus.device  = (int)dev->info.pci_dev;     // NOLINT
  tmp_info.pcie_bus.function= (int)dev->info.pci_func;    // NOLINT

  // create bitstream_id
  tmp_info.bitstream_id.parent = dev->info.bitstream_id.parent;
  tmp_info.bitstream_id.child  = dev->info.bitstream_id.child;

  // copy the created data into user argument
  memcpy(info, &tmp_info, sizeof(tmp_info));

  return 0;
}


// cppcheck-suppress unusedFunction
int fpga_get_device_config(
  const char *name,
  char **config_json
) {
  int ret;

  if (!name || !config_json) {
    llf_err(INVALID_ARGUMENT, "%s(name(%s), config_json(%#lx))\n",
      __func__, name ? name : "<null>", (uintptr_t)config_json);
    return -INVALID_ARGUMENT;
  }
  llf_dbg("%s(name(%s), config_json(%#lx))\n", __func__, name, (uintptr_t)config_json);

  uint32_t dev_id;
  ret = fpga_get_dev_id(name, &dev_id);
  if (ret) {
    llf_err(-ret, "Invalid operation: %s is not initailized.\n", name);
    return ret;
  }

  // Create bitstream_id string
  fpga_device_t *dev = fpga_get_device(dev_id);
  char bitstream_id[9];
  snprintf(bitstream_id, sizeof(bitstream_id), "%08x", dev->info.bitstream_id.parent);

  // Parse data and get config whose bitstream_id is match
  ret = __fpga_json_get_device_config(fpga_bitstream_config_table_file, bitstream_id, config_json);
  if (ret) {
    llf_err(-ret, "Failed to get parameter.\n");
    return ret;
  }

  return 0;
}


// cppcheck-suppress unusedFunction
int fpga_release_device_config(
  char *config_json
) {
  if (!config_json) {
    llf_err(INVALID_ARGUMENT, "%s(config_json(%s))\n", __func__, "<null>");
    return -INVALID_ARGUMENT;
  }
  llf_dbg("%s(config_json(%s))\n", __func__, config_json);

  free(config_json);

  return 0;
}


int fpga_set_device_config_path(
  const char *file_path
) {
  if (!file_path || strlen(file_path) >= FPGA_FILE_PATH_MAX) {
    llf_err(INVALID_ARGUMENT, "%s(file_path(%s))\n", __func__, file_path ? file_path : "<null>");
    return -INVALID_ARGUMENT;
  }
  llf_dbg("%s(file_path(%s))\n", __func__, file_path);

  memset(fpga_bitstream_config_table_file, 0, sizeof(fpga_bitstream_config_table_file));
  strncpy(fpga_bitstream_config_table_file, file_path, FPGA_FILE_PATH_MAX - 1);

  return 0;
}


int fpga_get_device_config_path(
  char **file_path
) {
  if (!file_path) {
    llf_err(INVALID_ARGUMENT, "%s(file_path(<null>))\n", __func__);
    return -INVALID_ARGUMENT;
  }

  *file_path = strdup(fpga_bitstream_config_table_file);
  if (!(*file_path)) {
    llf_err(FAILURE_MEMORY_ALLOC, "Failed to allocate memory for Bitstrea-Config table file's path\n");
    return -FAILURE_MEMORY_ALLOC;
  }

  return 0;
}


int fpga_soft_reset(
  uint32_t dev_id
) {
  llf_dbg("%s()\n", __func__);

  fpga_device_t *dev = fpga_get_device(dev_id);
  if (!dev) {
    llf_err(INVALID_ARGUMENT,
      "%s(dev_id(%u))\n", __func__, dev_id);
    return -INVALID_ARGUMENT;
  }

  if (fpgautil_ioctl(dev->fd, XPCIE_DEV_GLOBAL_CTRL_SOFT_RST, NULL) < 0) {
    int err = errno;
    llf_err(FAILURE_IOCTL, "Failed to ioctl XPCIE_DEV_GLOBAL_CTRL_SOFT_RST(errno:%d)\n", err);
    return -FAILURE_IOCTL;
  }
  return 0;
}


int __fpga_get_device_card_id(
  uint32_t dev_id
) {
  fpga_device_t *dev = fpga_get_device(dev_id);
  if (!dev) {
    llf_err(INVALID_ARGUMENT, "%s(dev_id(%u))\n", __func__, dev_id);
    return -INVALID_ARGUMENT;
  }

  int max_index = sizeof(fpga_available_card_table)/sizeof(fpga_available_card_table[0]);
  for (int index = 0; index < max_index; index++) {
    if (!strcmp(dev->info.card_name, fpga_available_card_table[index].card_name))
      return fpga_available_card_table[index].card_id;
  }

  llf_err(INVALID_PARAMETER, "%s(device_type(%s) is not supported.)\n", __func__, dev->info.card_name);
  return -INVALID_PARAMETER;
}


int fpga_enable_regrw(
  uint32_t dev_id
) {
  fpga_device_t *dev = fpga_get_device(dev_id);
  if (!dev) {
    llf_err(INVALID_ARGUMENT, "%s(dev_id(%u))\n", __func__, dev_id);
    return -INVALID_ARGUMENT;
  }
  llf_dbg("%s(dev_id(%u))\n", __func__, dev_id);

  uint32_t flag = XPCIE_DEV_REG_ENABLE;
  int ret = fpgautil_ioctl(dev->fd, XPCIE_DEV_DRIVER_SET_REG_LOCK, &flag);
  if (ret) {
    int err = errno;
    llf_err(FAILURE_IOCTL, "Failed to ioctl XPCIE_DEV_DRIVER_SET_REG_LOCK(errno:%d)\n", err);
    return -FAILURE_IOCTL;
  }

  return 0;
}


int fpga_disable_regrw(
  uint32_t dev_id
) {
  fpga_device_t *dev = fpga_get_device(dev_id);
  if (!dev) {
    llf_err(INVALID_ARGUMENT, "%s(dev_id(%u))\n", __func__, dev_id);
    return -INVALID_ARGUMENT;
  }
  llf_dbg("%s(dev_id(%u))\n", __func__, dev_id);

  uint32_t flag = XPCIE_DEV_REG_DISABLE;
  int ret = fpgautil_ioctl(dev->fd, XPCIE_DEV_DRIVER_SET_REG_LOCK, &flag);
  if (ret) {
    int err = errno;
    llf_err(FAILURE_IOCTL, "Failed to ioctl XPCIE_DEV_DRIVER_SET_REG_LOCK(errno:%d)\n", err);
    return -FAILURE_IOCTL;
  }

  return 0;
}


// cppcheck-suppress unusedFunction
int fpga_enable_regrw_all(void) {
  llf_dbg("%s()\n", __func__);
  int failed_device = 0;
  for (uint32_t dev_id = 0; dev_id < FPGA_MAX_DEVICES; dev_id++) {
    if (fpga_get_device(dev_id))
      if (fpga_enable_regrw(dev_id))
        failed_device++;
  }
  if (failed_device)
    return -FAILURE_IOCTL;

  return 0;
}


// cppcheck-suppress unusedFunction
int fpga_disable_regrw_all(void) {
  llf_dbg("%s()\n", __func__);
  int failed_device = 0;
  for (uint32_t dev_id = 0; dev_id < FPGA_MAX_DEVICES; dev_id++) {
    if (fpga_get_device(dev_id))
      if (fpga_disable_regrw(dev_id))
        failed_device++;
  }
  if (failed_device)
    return -FAILURE_IOCTL;

  return 0;
}
