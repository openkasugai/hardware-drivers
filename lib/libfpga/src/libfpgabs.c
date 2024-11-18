/*************************************************
* Copyright 2024 NTT Corporation, FUJITSU LIMITED
* Licensed under the 3-Clause BSD License, see LICENSE for details.
* SPDX-License-Identifier: BSD-3-Clause
*************************************************/

#include <libfpgabs.h>
#include <liblogging.h>

#include <libfpga_internal/libfpgautil.h>

#include <liblldma.h>
#include <libpower.h>

#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>


// LogLibFpga
#undef FPGA_LOGGER_LIBNAME
#define FPGA_LOGGER_LIBNAME LIBFPGABS


/**
 * static global variable : The name of directory for searching configuration file
 *                          in fpga_config_region().
 */
static char configure_file_dir_path[LIBFPGABS_RECONFIG_FILE_DIR_LEN] = "";


/**
 * @brief Function which convert region information between driver and library
 */
static xpcie_region_t __fpga_get_xpcie_region(
  fpga_region_t region
) {
  switch (region) {
  case FPGA_MODULE_REGION_ALL:
    return XPCIE_DEV_REGION_ALL;
  default:
    return XPCIE_DEV_REGION_MAX;
  }
}


static int __fpga_ref_control(
  uint32_t dev_id,
  fpga_region_t region,
  xpcie_refcount_cmd_t cmd,
  int *refcount
) {
  fpga_device_t *dev = fpga_get_device(dev_id);
  xpcie_region_t xpcie_region = __fpga_get_xpcie_region(region);
  if (!dev || xpcie_region >= XPCIE_DEV_REGION_MAX || cmd >= XPCIE_DEV_REFCOUNT_MAX) {
    llf_err(INVALID_ARGUMENT, "%s(dev_id(%u), region(%d->%d), cmd(%d))\n",
      __func__, dev_id, region, xpcie_region, cmd);
    return -INVALID_ARGUMENT;
  }

  fpga_ioctl_refcount_t ioctl_cmd = {
    .cmd = cmd,
    .region = xpcie_region
  };

  unsigned long request = XPCIE_DEV_DRIVER_SET_REFCOUNT; // NOLINT
  if (refcount)
    request = XPCIE_DEV_DRIVER_GET_REFCOUNT;

  // control refcount
  if (fpgautil_ioctl(dev->fd, request, &ioctl_cmd)) {
    int err = errno;
    if (err == XPCIE_DEV_REFCOUNT_WRITING) {
      llf_err(INVALID_OPERATION, "Invalid operation: FPGA[dev_id:%u, name:%s, region:%d] is now being written.\n",
        dev_id, dev->name, xpcie_region);
      return -INVALID_OPERATION;
    } else if (err == XPCIE_DEV_REFCOUNT_USING) {
      llf_err(INVALID_OPERATION, "Invalid operation: FPGA[dev_id:%u, name:%s, region:%d] is now being used.\n",
        dev_id, dev->name, xpcie_region);
      return -INVALID_OPERATION;
    } else {
      llf_err(FAILURE_IOCTL, "Failed to ioctl %s(errno:%d)\n",
        request == XPCIE_DEV_DRIVER_SET_REFCOUNT ? "XPCIE_DEV_DRIVER_SET_REFCOUNT"
                                                 : "XPCIE_DEV_DRIVER_GET_REFCOUNT", err);
      return -FAILURE_IOCTL;
    }
  }
  if (refcount)
    *refcount = ioctl_cmd.refcount;
  return 0;
}


int fpga_refcount_region_acquire(
  uint32_t dev_id,
  fpga_region_t region
) {
  llf_dbg("%s(dev_id(%u), region(%d))\n", __func__, dev_id, region);

  return __fpga_ref_control(dev_id, region, XPCIE_DEV_REFCOUNT_INC, NULL);
}


int fpga_refcount_region_release(
  uint32_t dev_id,
  fpga_region_t region
) {
  llf_dbg("%s(dev_id(%u), region(%d))\n", __func__, dev_id, region);

  return __fpga_ref_control(dev_id, region, XPCIE_DEV_REFCOUNT_DEC, NULL);
}


int fpga_refcount_region_cleanup(
  uint32_t dev_id,
  fpga_region_t region
) {
  llf_dbg("%s(dev_id(%u), region(%d))\n", __func__, dev_id, region);

  return __fpga_ref_control(dev_id, region, XPCIE_DEV_REFCOUNT_RST, NULL);
}


int fpga_refcount_region_get(
  uint32_t dev_id,
  fpga_region_t region,
  int *refcount
) {
  llf_dbg("%s(dev_id(%u), region(%d))\n", __func__, dev_id, region);

  return __fpga_ref_control(dev_id, region, XPCIE_DEV_REFCOUNT_GET, refcount);
}


int fpga_refcount_acquire(
  uint32_t dev_id
) {
  llf_dbg("%s(dev_id(%u))\n", __func__, dev_id);

  return __fpga_ref_control(dev_id, FPGA_MODULE_REGION_ALL, XPCIE_DEV_REFCOUNT_INC, NULL);
}


int fpga_refcount_release(
  uint32_t dev_id
) {
  llf_dbg("%s(dev_id(%u))\n", __func__, dev_id);

  return __fpga_ref_control(dev_id, FPGA_MODULE_REGION_ALL, XPCIE_DEV_REFCOUNT_DEC, NULL);
}


int fpga_refcount_cleanup(
  uint32_t dev_id
) {
  llf_dbg("%s(dev_id(%u))\n", __func__, dev_id);

  return __fpga_ref_control(dev_id, FPGA_MODULE_REGION_ALL, XPCIE_DEV_REFCOUNT_RST, NULL);
}


int fpga_ref_acquire(
  uint32_t dev_id
) {
  return fpga_refcount_acquire(dev_id);
}


int fpga_ref_release(
  uint32_t dev_id
) {
  return fpga_refcount_release(dev_id);
}


int fpga_ref_cleanup(
  uint32_t dev_id
) {
  return fpga_refcount_cleanup(dev_id);
}


/**
 * @brief Function which execute FPGA configuration
 */
static int __fpga_config_region(
  uint32_t dev_id,
  const char *file_path,
  fpga_region_t region
) {
  fpga_device_t *dev = fpga_get_device(dev_id);

  // Convert region from User Space into Kernel Space
  xpcie_region_t xpcie_region = __fpga_get_xpcie_region(region);

  // Check input
  if (!dev || !file_path || xpcie_region >= XPCIE_DEV_REGION_MAX) {
    llf_err(INVALID_ARGUMENT, "%s(dev_id(%u), file_path(%s), region(%d))\n",
      __func__, dev_id, file_path ? file_path : "<null>", xpcie_region);
    return -INVALID_ARGUMENT;
  }
  for (const char *c = file_path; *c != '\0'; c++) {
    if (*c == ';') {
      llf_err(INVALID_ARGUMENT,
        "Invalid operation: file_path includes invalid character(';').\n");
      return -INVALID_ARGUMENT;
    }
  }

  // Check if command processer exists or not
  if (system(NULL) == 0) {
    llf_err(LIBFPGA_FATAL_ERROR, "Fatal error: Command processer Not Exists...\n");
    return -LIBFPGA_FATAL_ERROR;
  }

  int ret = 0;

  // Change refcount status to writing mode
  ret = __fpga_ref_control(dev_id, region, XPCIE_DEV_REFCOUNT_WRITE, NULL);
  if (ret) {
    llf_err(-ret, "Failed to Set Writing Mode.\n");
    return ret;
  }
  // Write configuration file by mcap
  char cmd_system[1024];
  memset(cmd_system, 0, sizeof(cmd_system));
  if (region == FPGA_MODULE_REGION_ALL) {
    // tandem reconfig
    snprintf(cmd_system, sizeof(cmd_system),
      "%s -E -s %02x:%02x.%x -x %x -p %s",
      LIBFPGABS_MCAP_EXEC_FILE,
      dev->info.pci_bus,
      dev->info.pci_dev,
      dev->info.pci_func,
      dev->info.pci_device_id,
      file_path);
    llf_info(" Execute command: %s\n", cmd_system);
    ret = system(cmd_system);
    int ret_stat = WIFEXITED(ret);
    if (ret_stat) {
      ret_stat = WEXITSTATUS(ret);
      if (ret_stat == 0) {
        llf_info("Succeed to execute command\n");
      } else {
        llf_err(LIBFPGA_FATAL_ERROR,
          "Failed system in exit_status:%d(ret:%#x)\n", ret_stat, ret);
        ret = -LIBFPGA_FATAL_ERROR;
      }
    } else {
      llf_err(LIBFPGA_FATAL_ERROR, "Failed system(ret:%#x)\n", ret);
      ret = -LIBFPGA_FATAL_ERROR;
    }
  } else {
    llf_err(INVALID_OPERATION,
      "Invalid operation: The region(%d) not support...\n", region);
    ret = -INVALID_OPERATION;
  }
  if (ret) {
    llf_err(-ret, "Failed to write bitstream\n");
    goto finish;
  }

  if (region == FPGA_MODULE_REGION_ALL) {
    if (!ret) {
      // Update FPGA's control type
      if ((ret = fpga_update_bitstream_info(dev_id))) {
        llf_err(-ret, "Failed to update FPGA information in Driver.\n");
        goto finish;
      }
      // Set LLDMA's buffer address and length
      if ((ret = fpga_lldma_setup_buffer(dev_id))) {
        llf_err(-ret, "Failed to LLDMA setup.\n");
        goto finish;
      }
      // Reset CMS
      if ((ret = fpga_set_cms_unrest(dev_id))) {
        llf_err(-ret, "Failed to reset CMS.\n");
        goto finish;
      }
    }
  }

finish:

  // Change refcount status from writing mode to free mode
  __fpga_ref_control(dev_id, region, XPCIE_DEV_REFCOUNT_CLEAR, NULL);

  return ret;
}


// cppcheck-suppress unusedFunction
int fpga_write_bitstream(
  uint32_t dev_id,
  uint32_t lane,
  const char* file_path
) {
  if (!fpga_get_device(dev_id) || !file_path) {
    llf_err(INVALID_ARGUMENT, "%s(dev_id(%u), file_path(%s))\n",
      __func__, dev_id, file_path ? file_path : "<null>");
    return -INVALID_ARGUMENT;
  }
  llf_dbg("%s(dev_id(%u), file_path(%s))\n", __func__, dev_id, file_path);

  return __fpga_config_region(
    dev_id,
    file_path,
    FPGA_MODULE_REGION_ALL);
}


/**
 * @brief Function which check if target DMA channel is available or not
 */
static char *__get_file_path(
  const char *file_body,
  fpga_region_t region
) {
  // Check input
  if (!file_body)
    return NULL;

  // Allocate memory for file_path
  char* file_path = (char*)malloc(FPGA_FILE_PATH_MAX);  // NOLINT
  if (!file_path) {
    int err = errno;
    llf_err(FAILURE_MEMORY_ALLOC, "Failed to allocate Memory for Configure file name(errno:%d)\n", err);
    return NULL;
  }

  // Create file_path from region and file_body
  switch (region) {
  case FPGA_MODULE_REGION_ALL:
    snprintf(file_path, FPGA_FILE_PATH_MAX, "%s%s%s%s",
      configure_file_dir_path,
      LIBFPGABS_RECONFIG_TANDEM_PREFIX,
      file_body,
      LIBFPGABS_RECONFIG_SUFFIX);
    break;
  default:
    free(file_path);
    return NULL;
  }

  // Check exist
  FILE *fp = fopen(file_path, "r");
  if (!fp) {
    int err = errno;
    if (err == ENOENT)
      llf_err(INVALID_OPERATION, "Invalid operation: BitstreamFile(%s) Not Exist...\n", file_path);
    else
      llf_err(FAILURE_OPEN, "Failed to open %s(errno:%d)\n", file_path, err);
    free(file_path);
    return NULL;
  }
  fclose(fp);

  llf_dbg(" Convert BitstreamFile: '%s'->'%s'\n", file_body, file_path);
  return file_path;
}


int fpga_config_region(
  uint32_t dev_id,
  const char *file_body,
  fpga_region_t region
) {
  // Check input
  if (!fpga_get_device(dev_id) || !file_body || region >= FPGA_MODULE_REGION_MAX) {
    llf_err(INVALID_ARGUMENT, "%s(dev_id(%u), file_body(%s), region(%d))\n",
      __func__, dev_id, file_body ? file_body : "<null>", region);
    return -INVALID_ARGUMENT;
  }
  llf_dbg("%s(dev_id(%u), file_body(%s), region(%d))\n",
    __func__, dev_id, file_body, region);

  // Create BistreamFile's name
  char *file_path = __get_file_path(file_body, region);

  // Execute reconfiguration
  int ret = __fpga_config_region(
    dev_id,
    file_path,
    region);

  if (file_path)
    free(file_path);

  return ret;
}


int fpga_set_config_file_dir(
  const char *dir_path
) {
  // Check if dir_path is null
  if (!dir_path)
    goto invalid_arg;

  // Check if dir_path is too long(strlen(dir_path + '/' + '\0') should be in array)
  int len = strlen(dir_path);
  int slash_padding = 0;
  if (len && dir_path[len - 1] != '/')
    slash_padding = 1;
  if (len + slash_padding >= sizeof(configure_file_dir_path))
    goto invalid_arg;

  llf_dbg("%s(dir_path(%s))\n", __func__, dir_path);

  // init array
  memset(configure_file_dir_path, 0, sizeof(configure_file_dir_path));

  // if text is nothing, last slash is unnecessary
  if (len) {
    memcpy(configure_file_dir_path, dir_path, len);
    configure_file_dir_path[len - 1 + slash_padding] = '/';
  }

  return 0;

invalid_arg:
  llf_err(INVALID_ARGUMENT, "%s(dir_path(%s))\n", __func__, dir_path ? dir_path : "<null>");
  return -INVALID_ARGUMENT;
}


int fpga_get_config_file_dir(
  char **dir_path
) {
  if (!dir_path) {
    llf_err(INVALID_ARGUMENT, "%s(dir_path(<null>))\n", __func__);
    return -INVALID_ARGUMENT;
  }

  *dir_path = strdup(configure_file_dir_path);
  if (!(*dir_path)) {
    llf_err(FAILURE_MEMORY_ALLOC, "Failed to allocate memory for ConfigFile's Directory\n");
    return -FAILURE_MEMORY_ALLOC;
  }

  return 0;
}


int fpga_update_bitstream_info(
  uint32_t dev_id
) {
  fpga_device_t *dev = fpga_get_device(dev_id);
  if (!dev) {
    llf_err(INVALID_ARGUMENT, "%s(dev_id(%u))\n", __func__, dev_id);
    return -INVALID_ARGUMENT;
  }
  llf_dbg("%s(dev_id(%u))\n", __func__, dev_id);

  // Update FPGA's control type
  if (fpgautil_ioctl(dev->fd, XPCIE_DEV_DRIVER_SET_FPGA_UPDATE, NULL)) {
    int err = errno;
    llf_err(FAILURE_IOCTL, "Failure to ioctl XPCIE_DEV_DRIVER_SET_FPGA_UPDATE(errno:%d)\n", err);
    return -FAILURE_IOCTL;
  }
  return 0;
}
