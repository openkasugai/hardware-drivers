/*************************************************
* Copyright 2024 NTT Corporation, FUJITSU LIMITED
* Licensed under the 3-Clause BSD License, see LICENSE for details.
* SPDX-License-Identifier: BSD-3-Clause
*************************************************/

#include <libshmem.h>
#include <liblogging.h>

#include <libfpga_internal/libdpdkutil.h>
#include <libfpga_internal/libshmem_internal.h>

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <pthread.h>


// LogLibFpga
#undef FPGA_LOGGER_LIBNAME
#define FPGA_LOGGER_LIBNAME LIBSHMEM


/**
 * Minimum size to access register for DMA
 */
#define DMA_WORD_LINE_MASK  (0x3)


/**
 * static global variable: Num of Numa Node
 */
static int fpga_shmem_socket_num = 2;

/**
 * static global variable: Max Page of Hugepage
 */
static int fpga_shmem_hugepage_limit = SHMEM_MAX_HUGEPAGES;

/**
 * static global variable: Max Page of Hugepage per a Node
 */
static int fpga_shmem_socket_limit[SHMEM_MAX_NUMA_NODE] = {8, 8};

/**
 * static global variable: Max Num of lcore
 */
static int fpga_shmem_lcore_num = 112;

/**
 * static global variable: Mutex for alloc memory
 */
static pthread_mutex_t region_mutex = PTHREAD_MUTEX_INITIALIZER;

/**
 * static global variable: fd to notice finish DPDK in right way to shmem manager
 */
static int shmem_lock_file_fd = -1;

/**
 * static global variable: File prefix which succeed to init by this process
 */
static char shmem_file_prefix[SHMEM_MAX_FILE_NAME_LEN] = {'\0'};


/**
 * @enum SHMEM_VERSION_FILE_OPS
 * @brief Enumeration for flags to switch operation for DPDK's version file
 */
enum SHMEM_VERSION_FILE_OPS {
  SHMEM_VERSION_FILE_CREATE,  /**< Create version file */
  SHMEM_VERSION_FILE_COMPARE, /**< Compare version file */
  SHMEM_VERSION_FILE_DELETE,  /**< Delete version file */
};


/**
 * @brief Call rte_eal_init()
 */
static int __fpga_shmem_init(
  const char *file_prefix,
  const char *huge_dir,
  const uint32_t socket_limit[],
  const bool lcore_mask[],
  const char *proc_type,
  int rte_log_flag
) {
  int ret;
  int argc = 0;
  char socket_cmd[256];
  char lcore_cmd[256];
  char temp[16];
  char **argv = NULL;
  char *arg_prgname = NULL;
  char *arg_proc_type = NULL;
  char *arg_file_prefix = NULL;
  char *arg_huge_dir = NULL;

  ret = __fpga_shmem_init_host_info();
  if (ret)
    return ret;

  argv = (char**)malloc(sizeof(char*) * 16); // NOLINT
  if (!argv) {
    int err = errno;
    llf_err(FAILURE_MEMORY_ALLOC, "Failed to allocate memory for argv(%d).\n", err);
    ret = -FAILURE_MEMORY_ALLOC;
    goto finish;
  }

  // dummy(prgname)
  arg_prgname = strdup(__func__);
  if (!arg_prgname) {
    int err = errno;
    llf_err(FAILURE_MEMORY_ALLOC, "Failed to allocate memory for prgname(%d).\n", err);
    ret = -FAILURE_MEMORY_ALLOC;
    goto finish;
  }
  *(argv + (argc++)) = arg_prgname;

  // proc_type
  arg_proc_type = strdup(proc_type);
  if (!arg_proc_type) {
    int err = errno;
    llf_err(FAILURE_MEMORY_ALLOC, "Failed to allocate memory for proc_type(%d).\n", err);
    ret = -FAILURE_MEMORY_ALLOC;
    goto finish;
  }
  *(argv + (argc++)) = "--proc-type";
  *(argv + (argc++)) = arg_proc_type;

  // file_prefix
  if (file_prefix) {
    arg_file_prefix = strdup(file_prefix);
    if (!arg_file_prefix) {
      int err = errno;
      llf_err(FAILURE_MEMORY_ALLOC, "Failed to allocate memory for file_prefix(%d).\n", err);
      ret = -FAILURE_MEMORY_ALLOC;
      goto finish;
    }
    *(argv + (argc++)) = "--file-prefix";
    *(argv + (argc++)) = arg_file_prefix;
  }

  // huge_dir
  if (huge_dir) {
    arg_huge_dir = strdup(huge_dir);
    if (!arg_huge_dir) {
      int err = errno;
      llf_err(FAILURE_MEMORY_ALLOC, "Failed to allocate memory for huge_dir(%d).\n", err);
      ret = -FAILURE_MEMORY_ALLOC;
      goto finish;
    }
    *(argv + (argc++)) = "--huge-dir";
    *(argv + (argc++)) = arg_huge_dir;
  }

  // socket_limit
  if (socket_limit != NULL) {
    sprintf(socket_cmd, "%d", socket_limit[0] * 1024 + 1);  // NOLINT
    for (int i = 1; i < fpga_shmem_socket_num; i++) {
      sprintf(temp, ",%d", socket_limit[i] * 1024 + 1);  // NOLINT
      strcat(socket_cmd, temp); // NOLINT
    }
    *(argv + (argc++)) = "--socket-limit";
    *(argv + (argc++)) = socket_cmd;
  }

  // logical core
  if (lcore_mask != NULL) {
    int core_max = fpga_shmem_lcore_num;
    uint32_t core_mask;
    int alignment = core_max - (core_max % 4);

    sprintf(lcore_cmd, "0x");  // NOLINT
    for (int i = alignment; i > 0; i -= 4) {
      core_mask = 0;
      for (int j = 1; j <= 4; j++) {
        core_mask <<= 1;
        core_mask += (lcore_mask[i - j]);
      }
      sprintf(temp, "%x", core_mask);  // NOLINT
      strcat(lcore_cmd, temp); // NOLINT
    }
    if (alignment != core_max) {
      core_mask = 0;
      for (int i = core_max - 1; i >= alignment; i--) {
        core_mask <<= 1;
        core_mask += (lcore_mask[i]);
      }
      sprintf(temp, "%x", core_mask);  // NOLINT
      strcat(lcore_cmd, temp); // NOLINT
    }

    *(argv + (argc++)) = "-c";
    *(argv + (argc++)) = lcore_cmd;
  }

  // Set DPDK log level
  if (rte_log_flag) {
    *(argv + (argc++)) = "--log-level=lib.eal:debug";
  }

  *(argv + (argc++)) = "--";

  // Call rte_eal_init
  log_libfpga_cmdline_arg(LIBFPGA_LOG_DEBUG, argc, argv, LIBSHMEM "%s", "rte_eal_init");
  ret = rte_eal_init(argc, argv);

finish:
  if (arg_file_prefix)
    free(arg_file_prefix);
  if (arg_huge_dir)
    free(arg_huge_dir);
  if (arg_proc_type)
    free(arg_proc_type);
  if (arg_prgname)
    free(arg_prgname);
  if (argv)
    free(argv);

  if (ret < 0) {
    llf_err(-ret, "EAL initialization failed.\n");
    return ret;
  }

  return 0;
}


/**
 * @brief Function which act operations for a version file
 */
static int __fpga_shmem_ops_version_file(
  const char *file_prefix,
  enum SHMEM_VERSION_FILE_OPS flag_version_file_ops
) {
  if (!file_prefix) {
    llf_err(INVALID_ARGUMENT, "%s(file_prefix(%s), flag_version_file_ops(%d))\n",
      __func__, file_prefix, flag_version_file_ops);
    return -INVALID_ARGUMENT;
  }
  // When file_prefix is "", skip this process.
  if (strlen(file_prefix) == 0)
    return 0;

  char shmem_version_file_name[SHMEM_MAX_FILE_NAME_LEN];
  memset(shmem_version_file_name, 0, sizeof(shmem_version_file_name));
  snprintf(shmem_version_file_name, SHMEM_MAX_FILE_NAME_LEN, SHMEM_FMT_VERSION_FILE, file_prefix);

  int ret = 0;
  FILE *fp = NULL;
  int64_t size;
  char *buffer = NULL;

  switch (flag_version_file_ops) {
  case SHMEM_VERSION_FILE_CREATE:
    // Create version file
    fp = fopen(shmem_version_file_name, "w");
    if (!fp) {
      int err = errno;
      llf_err(FAILURE_OPEN, "  Failed to create version file(%s)(errno:%d)\n",
        shmem_version_file_name, err);
      ret = -FAILURE_OPEN;
      break;
    }
    // Set primary process's DPDK-version
    if (fprintf(fp, "%s", rte_version()) <= 0) {
      int err = errno;
      llf_err(FAILURE_WRITE, "  Failed to write version file(%s)(errno:%d)\n",
        shmem_version_file_name, err);
      ret = -FAILURE_WRITE;
      // Delete files
      if (unlink(shmem_version_file_name)) {
        err = errno;
        llf_err(FAILURE_WRITE, "  Failed to delete version file(%s)(errno:%d)\n",
          shmem_version_file_name, err);
      }
      break;
    }
    break;

  case SHMEM_VERSION_FILE_DELETE:
    // Delete version file
    if (unlink(shmem_version_file_name)) {
      int err = errno;
      llf_err(INVALID_OPERATION, "  Failed to delete version file(%s)(errno:%d)\n",
        shmem_version_file_name, err);
      ret = -INVALID_OPERATION;
      break;
    }
    break;

  case SHMEM_VERSION_FILE_COMPARE:
    // Get DPDK-version of primary process from version file
    fp = fopen(shmem_version_file_name, "r");
    if (!fp) {
      int err = errno;
      llf_err(FAILURE_OPEN, "  Failed to open version file(%s)(errno:%d)\n",
        shmem_version_file_name, err);
      ret = -FAILURE_OPEN;
      break;
    }

    // Get file size
    if (fseek(fp, 0, SEEK_END)) {
      int err = errno;
      llf_err(INVALID_OPERATION, "  Failed to seek_end version file(%s)(errno:%d)\n",
        shmem_version_file_name, err);
      ret = -INVALID_OPERATION;
      break;
    }
    size = ftell(fp);
    if (fseek(fp, 0, SEEK_SET)) {
      int err = errno;
      llf_err(INVALID_OPERATION, "  Failed to seek_set version file(%s)(errno:%d)\n",
        shmem_version_file_name, err);
      ret = -INVALID_OPERATION;
      break;
    }
    if (size <= 0) {
      llf_err(INVALID_PARAMETER, "  The size(%d) of version file(%s) is invalid...\n",
        size, shmem_version_file_name);
      ret = -INVALID_PARAMETER;
      break;
    }

    // Allocate buffer to read file
    buffer = (char*)malloc(size + 1);  // NOLINT
    if (!buffer) {
      int err = errno;
      llf_err(FAILURE_MEMORY_ALLOC, "  Failed to allocate memory for version file buffer(%d).\n", err);
      ret = -FAILURE_MEMORY_ALLOC;
      break;
    }
    memset(buffer, 0, size + 1);

    // Get primary process's DPDK-version
    if (fread(buffer, size, 1, fp) != 1) {
      int err = errno;
      llf_err(FAILURE_READ, "  Failed to read version file(%s)(errno:%d)\n",
        shmem_version_file_name, err);
      ret = -FAILURE_READ;
      break;
    }

    // Compare with primary process's DPDK-version
    if (strcmp(buffer, rte_version())) {
      llf_err(INVALID_DATA, "  Mismatch DPDK's version between primary and this process.\n");
      llf_err(INVALID_DATA, "   * primary process version: %s\n", buffer);
      llf_err(INVALID_DATA, "   * this process version: %s\n", rte_version());
      ret = -INVALID_DATA;
      break;
    }
    break;

  default:
    llf_err(UNKNOWN_EXCEPTION, "  This log will not print.\n", buffer);
    break;
  }

  if (fp)
    fclose(fp);
  if (buffer)
    free(buffer);

  return ret;
}


/**
 * @brief Create a lock file and lock it with fd managemented as global variable
 */
static int __fpga_shmem_create_lock_file(
  const char *file_prefix
) {
  llf_dbg("%s(file_prefix(%s))\n", __func__, file_prefix);

  char shmem_lock_file_name[SHMEM_MAX_FILE_NAME_LEN];
  memset(shmem_lock_file_name, 0, sizeof(shmem_lock_file_name));
  snprintf(shmem_lock_file_name, SHMEM_MAX_FILE_NAME_LEN, SHMEM_FMT_FLOCK_FILE, file_prefix);

  // Get fd by open() for flock()
  shmem_lock_file_fd = open(shmem_lock_file_name, O_RDWR | O_CREAT, 0644);
  if (shmem_lock_file_fd < 0) {
    llf_warn(FAILURE_OPEN, "  Failed to create lock file(%s)\n", shmem_lock_file_name);
    return -FAILURE_OPEN;
  }
  if (flock(shmem_lock_file_fd, LOCK_SH) < 0) {
    llf_warn(FAILURE_OPEN, "  Failed to lock file(%s)\n", shmem_lock_file_name);
    return -FAILURE_OPEN;
  }

  return 0;
}


/**
 * @brief Unlock the lock file
 */
static void __fpga_shmem_unlock_lock_file(
  const char *file_prefix
) {
  llf_dbg("%s(%s)\n", __func__, file_prefix);

  if (flock(shmem_lock_file_fd, LOCK_UN) < 0) {
    llf_warn(LIBFPGA_FATAL_ERROR, "  Failed to unlock file(%s)\n", file_prefix);
  }
}


int fpga_shmem_init(
  const char *file_prefix,
  const bool lcore_mask[],
  int rte_log_flag
) {
  llf_dbg("%s(file_prefix(%s), lcore_mask(%#llx), flag(%d))\n",
    __func__, file_prefix, (uint64_t)lcore_mask, rte_log_flag);

  int ret;

  const char *file_prefix_alt = file_prefix ? file_prefix
                                            : SHMEM_DPDK_DEFAULT_PREFIX;

  // Check Primary process's DPDK version
  ret = __fpga_shmem_ops_version_file(
    file_prefix_alt,
    SHMEM_VERSION_FILE_COMPARE);
  if (ret)
    return ret;

  // Initialize as secondary
  ret = __fpga_shmem_init(
    file_prefix_alt,
    NULL,
    NULL,
    lcore_mask,
    "secondary",
    rte_log_flag);
  if (ret)
    return ret;

  // Create lock file
  if (__fpga_shmem_create_lock_file(file_prefix_alt)) {
    fpga_shmem_finish();
    return -FAILURE_INITIALIZE;
  }

  memset(shmem_file_prefix, 0, sizeof(shmem_file_prefix));
  snprintf(shmem_file_prefix, sizeof(shmem_file_prefix), "%s", file_prefix_alt);

  return 0;
}


int fpga_shmem_init_sys(
  const char *file_prefix,
  const char *huge_dir,
  const uint32_t socket_limit[],
  const bool lcore_mask[],
  int rte_log_flag
) {
  llf_dbg("%s(file_prefix(%s), huge_dir(%s), socket_limit(%#llx), lcore_mask(%#llx))\n",
    __func__, file_prefix, huge_dir, (uint64_t)socket_limit, (uint64_t)lcore_mask);

  int ret;

  const char *file_prefix_alt = file_prefix ? file_prefix
                                            : SHMEM_DPDK_DEFAULT_PREFIX;

  // Initialize DPDK as primary process
  ret = __fpga_shmem_init(
    file_prefix_alt,
    huge_dir,
    socket_limit,
    lcore_mask,
    "primary",
    rte_log_flag);
  if (ret)
    return ret;

  // Create Primary process's DPDK version file
  ret = __fpga_shmem_ops_version_file(
    file_prefix_alt,
    SHMEM_VERSION_FILE_CREATE);
  if (ret) {
    rte_eal_cleanup();
    return ret;
  }

  // Store file_prefix
  memset(shmem_file_prefix, 0, sizeof(shmem_file_prefix));
  snprintf(shmem_file_prefix, sizeof(shmem_file_prefix), "%s", file_prefix_alt);

  return ret;
}


int fpga_shmem_finish(void) {
  llf_dbg("%s()\n", __func__);

  fpga_shmem_unregister_all();

  int ret = rte_eal_cleanup();

  if (shmem_lock_file_fd >= 0) {
    // secondary process
    int len, buf = 1;
    len = write(shmem_lock_file_fd, &buf, sizeof(buf));
    llf_dbg("%dbyte written\n", len);
    __fpga_shmem_unlock_lock_file(shmem_file_prefix);
  } else {
    // primary process
    __fpga_shmem_ops_version_file(shmem_file_prefix, SHMEM_VERSION_FILE_DELETE);
  }

  memset(shmem_file_prefix, 0, sizeof(shmem_file_prefix));

  return ret;
}


/**
 * @brief Register memory into memory map
 */
static inline int __add_new_region(
  void *va,
  uint64_t len
) {
  llf_dbg("%s(va(%#llx))\n", __func__, (uintptr_t)va);

  struct rte_memseg_list *msl;
  struct rte_memseg *ms;

  msl = rte_mem_virt2memseg_list(va);
  if (!msl) {
    llf_warn(FAILURE_MEMORY_ALLOC, "  memseg list is full.\n");
    return -1;
  }
  ms  = rte_mem_virt2memseg(va, msl);
  if (!ms) {
    llf_warn(INVALID_ADDRESS, "  memseg is empty.\n");
    return -1;
  }

  // fpga_shmem_register((void*)ms->addr_64, rte_mem_virt2phy(ms->addr), ms->hugepage_sz);
  fpga_shmem_register(va, rte_mem_virt2phy(va), len);

  return 0;
}


/**
 * @brief Update memory in the memory map
 */
static inline int __remap_region(
  void *va,
  uint64_t len
) {
  llf_dbg("%s(va(%#llx))\n", __func__, (uintptr_t)va);

  struct rte_memseg_list *msl;
  struct rte_memseg *ms;

  if (__fpga_shmem_register_check(va)) {
    llf_err(LIBFPGA_FATAL_ERROR, "  There are no valid regions for %#llx\n", (uintptr_t)va);
    return -1;
  }

  msl = rte_mem_virt2memseg_list(va);
  if (!msl) {
    llf_warn(FAILURE_MEMORY_ALLOC, "  memseg list is full.\n");
    return -1;
  }
  ms  = rte_mem_virt2memseg(va, msl);
  if (!ms) {
    llf_warn(INVALID_ADDRESS, "  memseg is empty.\n");
    return -1;
  }

  // fpga_shmem_register_update((void*)ms->addr_64, rte_mem_virt2phy(ms->addr), ms->hugepage_sz);
  fpga_shmem_register_update(va, rte_mem_virt2phy(va), len);

  return 0;
}


// cppcheck-suppress unusedFunction
void *fpga_shmem_alloc(
  size_t length
) {
  llf_dbg("%s(length(%#llx))\n", __func__, length);

  uint64_t pa, chklen;
  void *va;

  // allocate memory from hugepages with 64-bytes cache align
  va = rte_malloc("data", length, RTE_CACHE_LINE_SIZE);
  if (va == NULL) {
    llf_err(FAILURE_MEMORY_ALLOC, "  Failed to allocate HUGEPAGE.\n");
    return NULL;
  }

  chklen = length;
  pthread_mutex_lock(&region_mutex);
  do {
    pa = __dma_pa_from_va(va, &chklen);
    if (pa == 0) {
      if (__add_new_region(va, chklen) < 0)
        goto err_out;
    } else if (pa != rte_mem_virt2phy(va)) {
      if (__remap_region(va, chklen) < 0)
        goto err_out;
    }
  } while (pa == 0);

  if (length != chklen) {
    llf_warn(INVALID_DATA, "  Cannot allocate memory in a hugepage.\n");
    goto err_out;
  }
  pthread_mutex_unlock(&region_mutex);

  return va;

err_out:
  pthread_mutex_unlock(&region_mutex);
  rte_free(va);

  llf_err(FAILURE_MEMORY_ALLOC, "  Failed to get virt-phys map.\n");

  return NULL;
}


// cppcheck-suppress unusedFunction
void *fpga_shmem_aligned_alloc(
  size_t length
) {
  llf_dbg("%s(length(%#llx))\n", __func__, length);

  uint64_t pa, chklen;
  void *va;

  // allocate memory from hugepages with 1024-bytes cache align
  va = rte_malloc("data", length, SHMEM_BOUNDARY_SIZE);
  if (va == NULL) {
    llf_err(FAILURE_MEMORY_ALLOC, "  Failed to allocate HUGEPAGE.\n");
    return NULL;
  }

  chklen = length;
  pthread_mutex_lock(&region_mutex);
  do {
    pa = __dma_pa_from_va(va, &chklen);
    if (pa == 0) {
      if (__add_new_region(va, chklen) < 0)
        goto err_out;
    } else if (pa != rte_mem_virt2phy(va)) {
      if (__remap_region(va, chklen) < 0)
        goto err_out;
    }
  } while (pa == 0);

  if (length != chklen) {
    llf_warn(INVALID_DATA, "  Cannot allocate memory in a hugepage.\n");
    goto err_out;
  }
  pthread_mutex_unlock(&region_mutex);

  return va;

err_out:
  pthread_mutex_unlock(&region_mutex);
  rte_free(va);

  llf_err(FAILURE_MEMORY_ALLOC, "  Failed to get virt-phys map.\n");

  return NULL;
}


// cppcheck-suppress unusedFunction
void fpga_shmem_free(
  void *addr
) {
  llf_dbg("%s(addr(%#llx))\n", __func__, (uintptr_t)addr);

  rte_free(addr);
  fpga_shmem_unregister(addr);
}


uint64_t __dma_pa_from_va(
  void *va,
  uint64_t *len
) {
  return __fpga_shmem_mmap_v2p(va, len);
}


uint64_t dma_pa_from_va(
  void *va,
  uint64_t *len
) {
  llf_dbg("%s(va(%#llx), len(%#llx))\n", __func__, (uintptr_t)va, *len);

  if (((uintptr_t)va & DMA_WORD_LINE_MASK) || (*len & DMA_WORD_LINE_MASK)) {
    llf_warn(INVALID_ADDRESS, "  Alignment error!\n");
    return 0;
  }
  return __fpga_shmem_mmap_v2p(va, len);
}


void *local_phy2virt(
  uint64_t pa64
) {
  return __fpga_shmem_mmap_p2v(pa64);
}


int __fpga_shmem_init_host_info(void) {
  static bool is_get_host_info = false;
  if (is_get_host_info)
    return 0;
  is_get_host_info = !is_get_host_info;

  llf_dbg("%s()\n", __func__);
  FILE *fp;
  int free_hugepages;
  int free_hugepages_all = 0;

  /* get NUMA Node number */
  int node_index;
  for (node_index = 0; node_index < SHMEM_MAX_NUMA_NODE; node_index++) {
    char dirname[SHMEM_MAX_FILE_NAME_LEN];
    sprintf(dirname, SHMEM_FMT_NUMA_NODE_DIRECTORY, node_index);  // NOLINT
    DIR *dir = opendir(dirname);
    int err = errno;
    if (!dir && err == ENOENT) {
      break;
    }
    if (!dir) {
      llf_err(LIBFPGA_FATAL_ERROR, "Fatal error: Cannot get NUMA Node num at node_index=%d\n", node_index);
      return -LIBFPGA_FATAL_ERROR;
    }
    closedir(dir);
  }
  fpga_shmem_socket_num = node_index;
  llf_info(" fpga_shmem_socket_num=%d\n", fpga_shmem_socket_num);

  /* get free hugepages per node */
  memset(fpga_shmem_socket_limit, 0, sizeof(fpga_shmem_socket_limit));
  for (node_index = 0; node_index < fpga_shmem_socket_num; node_index++) {
    char filename[SHMEM_MAX_FILE_NAME_LEN];
    sprintf(filename, SHMEM_FMT_NUMA_NODE_FREE_HUGEPAGE, node_index);  // NOLINT
    fp = fopen(filename, "r");
    if (!fp) {
      int err = errno;
      llf_err(FAILURE_OPEN, "Failed to open %s(errno:%d)\n", filename, err);
      return -FAILURE_OPEN;
    }
    if (fscanf(fp, "%d", &free_hugepages) != 1) {
      int err = errno;
      llf_err(LIBFPGA_FATAL_ERROR, "Fatal error: Failed to get free_hugepages at node:%d(errno:%d)\n", node_index, err);
      return -LIBFPGA_FATAL_ERROR;
    }
    fpga_shmem_socket_limit[node_index] = free_hugepages;
    free_hugepages_all += free_hugepages;
    llf_info(" fpga_shmem_socket_limit[%d]=%d\n", node_index, fpga_shmem_socket_limit[node_index]);
    fclose(fp);
  }
  if (free_hugepages_all < SHMEM_MAX_HUGEPAGES)
    fpga_shmem_hugepage_limit = free_hugepages_all;
  if (free_hugepages_all > SHMEM_MAX_HUGEPAGES)
    llf_warn(INVALID_DATA, "This library can use hugepages only less than %dG\n", SHMEM_MAX_HUGEPAGES + 1);
  llf_info(" fpga_shmem_hugepage_limit=%d\n", fpga_shmem_hugepage_limit);

  /* get lcore max */
  int temp_max = -1, cpu_list_num;
  for (node_index = 0; node_index < fpga_shmem_socket_num; node_index++) {
    char filename[SHMEM_MAX_FILE_NAME_LEN];
    sprintf(filename, SHMEM_FMT_NUMA_NODE_CPULIST, node_index);  // NOLINT
    fp = fopen(filename, "r");
    if (!fp) {
      llf_err(FAILURE_OPEN, "Failed to open %s\n", filename);
      return -FAILURE_OPEN;
    }
    do {
      int ret = fscanf(fp, "%d", &cpu_list_num);
      if (ret == 0 || ret == EOF)
        break;
    } while (fscanf(fp, "%*c") != EOF);
    if (temp_max < cpu_list_num)
      temp_max = cpu_list_num;
    fclose(fp);
  }
  fpga_shmem_lcore_num = temp_max + 1;
  llf_info(" fpga_shmem_lcore_num=%d\n", fpga_shmem_lcore_num);

  return 0;
}


int *__fpga_shmem_get_socket_num(void) {
  return &fpga_shmem_socket_num;
}


int *__fpga_shmem_get_lcore_limit(void) {
  return &fpga_shmem_lcore_num;
}


int *__fpga_shmem_get_available_limit(void) {
  return &fpga_shmem_hugepage_limit;
}


int *__fpga_shmem_get_socket_limit(void) {
  return fpga_shmem_socket_limit;
}


// cppcheck-suppress unusedFunction
int fpga_shmem_init_arg(
  int argc,
  char **argv
) {
  log_libfpga_cmdline_arg(LIBFPGA_LOG_DEBUG, argc, argv, LIBSHMEM "%s", __func__);

  int ret;

  ret = rte_eal_init(argc, argv);
  if (ret < 0) {
    llf_err(-ret, "EAL initialization failed.\n");
  }

  return ret;
}
