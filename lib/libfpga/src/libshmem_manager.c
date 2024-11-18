/*************************************************
* Copyright 2024 NTT Corporation, FUJITSU LIMITED
* Licensed under the 3-Clause BSD License, see LICENSE for details.
* SPDX-License-Identifier: BSD-3-Clause
*************************************************/

#include <libshmem_manager.h>
#include <liblogging.h>

#include <libfpga_internal/libdpdkutil.h>
#include <libfpga_internal/libshmem_internal.h>

#include <unistd.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/file.h>


// LogLibFpga
#define LIBSHMEM_MANAGER  "[MNGR] "
#undef FPGA_LOGGER_LIBNAME
#define FPGA_LOGGER_LIBNAME LIBSHMEM LIBSHMEM_MANAGER


/**
 * Definition of format for file which transfer data with shmem manager and caller
 */
#define SHMEM_PARENT_CHILD_TSF          "/var/run/dpdk/%s/tsf"


/**
 * @enum SHMEM_IS_INITIALIZED
 * @brief Enumeration for shmem manager status
 */
enum SHMEM_IS_INITIALIZED{
  SHMEM_NOT_INITIALIZED = 0,  /**< not launched */
  SHMEM_INITIALIZED,          /**< succeed to be launched */
  SHMEM_INITIALIZE_FAILED,    /**< failed to be launched */
  SHMEM_FINISHED,             /**< not used */
};


/**
 * static global variable: List of ShmemManagers
 */
static fpga_shmem_manager_info_t manager_infos[SHMEM_MAX_HUGEPAGES];

/**
 * static global variable: Flag to check ShmemController's signal for finish
 */
static volatile bool signal_flg = false;

/**
 * static global variable: Flag to check which can be killed safely
 */
static bool signal_set = false;

/**
 * static global variable: Flag to check which can be killed safely
 */
static int (*signal_function)(sig_func) = NULL;


/**
 * @brief Function for finish manager(this function can be made by user)
 */
static void __fpga_shmem_finish_signaling(
  int signum
) {
  llf_dbg("%s(signum(%d))\n", __func__, signum);

  signal_flg = true;
}

/**
 * @brief Function set signaling for finish manager
 */
static int __fpga_shmem_set_signal(
  const sig_func func
) {
  llf_dbg("%s(sigfunc(%#llx))\n", __func__, (uint64_t)func);

  int ret = 0;
  struct sigaction signal;
  memset(&signal, 0, sizeof(signal));
  signal.sa_handler = func;
  signal.sa_flags = SA_RESTART;

  // Set signal handler
  ret = sigaction(SIGUSR1, &signal, NULL);
  if (ret) {
    int err = errno;
    llf_err(FAILURE_REGISTER_SIGNAL, "Failed to sigaction SIGUSR1(errno:%d)\n", err);
    return -FAILURE_REGISTER_SIGNAL;
  }

  return 0;
}


int fpga_shmem_set_signal(void) {
  static int set_once = 0;
  if (set_once) {
    llf_err(ALREADY_INITIALIZED, "%s()\n", __func__);
    return -ALREADY_INITIALIZED;
  }
  llf_dbg("%s()\n", __func__);
  int ret;

  if (!signal_function)
    signal_function = __fpga_shmem_set_signal;
  ret = signal_function(__fpga_shmem_finish_signaling);
  if (ret) {
    return ret;
  }

  // Init manager_infos
  memset(manager_infos, 0, sizeof(manager_infos));

  // Set flag to enable fpga_shmem_manager_init()
  signal_set = true;

  // Get system value of Hugepages/lcores/NUMA nodes
  __fpga_shmem_init_host_info();

  set_once = 1;

  return 0;
}


/**
 * @brief equal "rm <dirname> && rmdir <dirname>"
 */
static int __remove_hugedir(
  const char *dirname
) {
  int ret, err;

  DIR *dir = opendir(dirname);
  if (!dir) {
    err = errno;
    if (err == ENOENT) {
      llf_dbg("Ignored operation: directory(%s) is already NOT exist.\n", dirname);
      return 0;
    } else {
      llf_err(FAILURE_OPEN, "Failed to open directory(%s)(errno:%d)\n", dirname, err);
      return -FAILURE_OPEN;
    }
  }
  struct dirent *ent;
  while ((ent = readdir(dir))) {
    char filename[SHMEM_MAX_FILE_NAME_LEN+SHMEM_MAX_FILE_NAME_LEN+1];
    sprintf(filename, "%s/%s", dirname, ent->d_name);  // NOLINT
    if (strcmp(ent->d_name, ".") && strcmp(ent->d_name, ".."))
      unlink(filename);
  }
  closedir(dir);

  ret = rmdir(dirname);
  err = errno;
  if (ret) {
    if (err == ENONET) {
      llf_dbg("Ignored operation: directory(%s) is already NOT exist.\n", dirname);
    } else {
      llf_err(INVALID_OPERATION, "Failed to remove directory(%s)(errno:%d)\n", dirname, err);
      return -INVALID_OPERATION;
    }
  } else {
    llf_dbg("Succeed to remove directory(%s)\n", dirname);
  }
  return 0;
}


/**
 * @brief clear data of specified index without checking if process finished or not
 */
static void __fpga_shmem_reset_manager_info(
  int index
) {
  memset(&manager_infos[index], 0, sizeof(manager_infos[index]));
}


/**
 * @brief update managers_info
 */
static void __fpga_shmem_check_health(void) {
  int pid;                // process id
  int status;             // status from waitpid
  int exit_stat;          // exit status of child process
  int num_dead_proc = 0;  // the num of Dead process
  for (int i = 0; i < fpga_shmem_get_available_limit(); i++) {
    // Check all index
    if (manager_infos[i].pid <= 0) {
      // data is not set if pid is lesser than 0
      // data is not controled by this process if pid is 0
      continue;
    }
    pid = waitpid(manager_infos[i].pid, &status, WNOHANG);
    if (pid == 0) {
      // Child process is still alive, so skip
      continue;
    }
    // Child process is dead
    if (WIFEXITED(status)) {
      // Child process normaly died
      exit_stat = WEXITSTATUS(status);
      if (exit_stat) {
        // Child process finish in failure
        llf_err(exit_stat, "Primary process detect Child process[%d] died in %d.\n", manager_infos[i].pid, exit_stat);
        __fpga_shmem_reset_manager_info(i);
        num_dead_proc++;
      } else {
        // Child process finish safely
        llf_dbg("Primary process detect Child process[%d] finished safely.\n", manager_infos[i].pid);
        char dirname[SHMEM_MAX_FILE_NAME_LEN];
        memset(dirname, 0, sizeof(dirname));
        snprintf(dirname, SHMEM_MAX_FILE_NAME_LEN, "/dev/hugepages/%s", manager_infos[i].file_prefix);
        __remove_hugedir(dirname);
        char rundir[SHMEM_MAX_FILE_NAME_LEN];
        memset(rundir, 0, sizeof(rundir));
        snprintf(rundir, SHMEM_MAX_FILE_NAME_LEN, "/var/run/dpdk/%s", manager_infos[i].file_prefix);
        __remove_hugedir(rundir);
        __fpga_shmem_reset_manager_info(i);
      }
    }
  }

  // the number of Child process which finish in failure(includes Callback function)
  // return num_dead_proc;
}


/**
 * @brief Get the smallest free index in manager_infos
 */
static int __fpga_shmem_get_index(void) {
  __fpga_shmem_check_health();
  for (int i = 0; i < fpga_shmem_get_available_limit(); i++) {
    if (!manager_infos[i].file_prefix[0])
      return i;
  }
  return -1;
}


/**
 * @brief set data into managers_info
 */
static int __fpga_shmem_set_manager_info(
  int index,
  const char *file_prefix,
  pid_t pid,
  const uint32_t socket_limit[]
) {
  __fpga_shmem_check_health();
  strcpy(manager_infos[index].file_prefix, file_prefix);  // NOLINT
  manager_infos[index].pid = pid;
  manager_infos[index].hp_limit = 0;
  if (socket_limit) {
    for (int i = 0; i < fpga_shmem_get_socket_num(); i++) {
      manager_infos[index].socket_limit[i] = socket_limit[i];
      manager_infos[index].hp_limit += socket_limit[i];
    }
  }
  return 0;
}


/**
 * @brief User process
 */
static int __fpga_shmem_parent_process(
  pid_t pid,
  const char *file_prefix,
  const uint32_t socket_limit[]
) {
  llf_dbg("%s(pid(%d), file_prefix(%s), limit(%#llx))\n", __func__, pid, file_prefix, (uint64_t)socket_limit);

  // Get the smallest and free index
  int index = __fpga_shmem_get_index();
  if (index < 0) {
    llf_err(FULL_ELEMENT, "Invalid operation: List for management is full.\n");
    return -FULL_ELEMENT;
  }
  llf_dbg(" Got index : %d\n", index);

  // Data transfer file between parent and child process
  // File will be made by child process
  char filename[SHMEM_MAX_FILE_NAME_LEN];
  sprintf(filename, SHMEM_PARENT_CHILD_TSF, file_prefix);  // NOLINT
  FILE *fp;
  unsigned int count = 0;
  do {
    usleep(50 * 1000);
    count++;
    if (count > 1200)
      goto CHILD_INIT;
  }while(!(fp = fopen(filename, "r")));
  while (!fscanf(fp, "%d", &manager_infos[index].is_initialized))
      continue;
  fclose(fp);
  unlink(filename);

CHILD_INIT:
  if (manager_infos[index].is_initialized == SHMEM_INITIALIZED) {
    // Success to make Child process
    // Set the data for controller
    int ret = __fpga_shmem_set_manager_info(index, file_prefix, pid, socket_limit);
    if (ret) {
      llf_err(FAILURE_MEMORY_ALLOC, "Failed to alloc memory for ShmemManager information.\n");
      ret = kill(pid, SIGUSR1);
      if (ret) {
        int err = errno;
        llf_err(FAILURE_SEND_SIGNAL, " Failed to kill process(errno:%d)\n", err);
        return -FAILURE_SEND_SIGNAL;
      }
      return -FAILURE_MEMORY_ALLOC;
    }
  } else {
    // failed to make Child process
    llf_err(FAILURE_INITIALIZE, "Maybe Failed to initialize DPDK(stat:%d)\n",
      manager_infos[index].is_initialized);
    manager_infos[index].is_initialized = 0;
    return -FAILURE_INITIALIZE;
  }
  return 0;
}


/**
 * @brief shmem manager process
 */
static void __fpga_shmem_child_process(
  const char *file_prefix,
  const uint32_t hp_socket[],
  const bool lcore_mask[],
  const shmem_func clb,
  void *arg
) {
  int exit_val = 0;
  llf_dbg("%s(file_prefix(%s), limit(%#lx), clb(%#lx), arg(%#lx)\n",
    __func__, file_prefix, (uint64_t)hp_socket, (uint64_t)clb, (uint64_t)arg);

  // transfer file names
  char filename[SHMEM_MAX_FILE_NAME_LEN];
  sprintf(filename, SHMEM_PARENT_CHILD_TSF, file_prefix);  // NOLINT
  unlink(filename);
  FILE *fp;

  // DPDK initialize as primary
  // init DPDK as primary
  char huge_dir[SHMEM_MAX_FILE_NAME_LEN];
  sprintf(huge_dir, "/dev/hugepages/%s", file_prefix);  // NOLINT
  int childret = fpga_shmem_init_sys(file_prefix, huge_dir, hp_socket, lcore_mask, 0);
  if (childret < 0) {
    llf_err(FAILURE_INITIALIZE, "Failed to initialize DPDK as Primary\n");
    exit_val = FAILURE_INITIALIZE;
  }
  // Create transfer file with parent process
  fp = fopen(filename, "w");
  if (fp == NULL) {
    // Finish launched ShmemManager if failed to finish
    llf_err(FAILURE_OPEN, "Failed to create transfer file(%s)\n", filename);
    exit_val = FAILURE_OPEN;
    if (childret == 0) {
      fpga_shmem_finish();
      char rundir[SHMEM_MAX_FILE_NAME_LEN];
      sprintf(rundir, "/var/run/dpdk/%s", file_prefix);  // NOLINT
      __remove_hugedir(rundir);
    }
    __remove_hugedir(huge_dir);
  } else {
    // send to parent that initializing status
    fprintf(fp, "%d", exit_val ? SHMEM_INITIALIZE_FAILED : SHMEM_INITIALIZED);
    fclose(fp);
  }
  // exit process if something error happened
  if (exit_val) {
    llf_err(exit_val, "Exit(%d): Failed to Launch ShmemManager\n", exit_val);
    exit(exit_val);
  }

  llf_dbg("Succeed to Initialize DPDK as primary process\n");

  // loop before call back function detect something wrong(ret!=0)
  // loop before receive signal SIGUSR1
  do {
    // CALLBACK_FUNCTION
    // check call back function is NULL
    if (clb) {
      if ((childret = clb(arg))) {
        // call back function's retval is not 0
        llf_err(CALLBACK_FUNCTION, "Detected non-zero value(%d) from Callback function\n", childret);
        fpga_shmem_finish();
        __remove_hugedir(huge_dir);
        char rundir[SHMEM_MAX_FILE_NAME_LEN];
        sprintf(rundir, "/var/run/dpdk/%s", file_prefix);  // NOLINT
        __remove_hugedir(rundir);
        llf_dbg("Exit(%d): Finished Primary DPDK Process by Callback function\n", CALLBACK_FUNCTION);
        exit(CALLBACK_FUNCTION);
      }
    }
    usleep(500 * 1000);
  }while(!signal_flg);

  // finish in success
  fpga_shmem_finish();
  __remove_hugedir(huge_dir);
  char rundir[SHMEM_MAX_FILE_NAME_LEN];
  sprintf(rundir, "/var/run/dpdk/%s", file_prefix);  // NOLINT
  __remove_hugedir(rundir);

  llf_dbg("Exit(%d): Finished Primary DPDK Process safely by signal\n", exit_val);
  exit(exit_val);
}


/**
 * @brief Check input file_prefix is not registerd yet
 */
static int __fpga_shmem_check_file_prefix(
  const char* file_prefix
) {
  __fpga_shmem_check_health();
  if (file_prefix == NULL)
    return -1;
  if (strlen(file_prefix) > SHMEM_MAX_HUGEPAGE_PREFIX)
    return -2;
  for (int i = 0; i < fpga_shmem_get_available_limit(); i++) {
    if (!manager_infos[i].file_prefix[0])
      continue;
    if (!strcmp(file_prefix, manager_infos[i].file_prefix))
      return -1;
  }
  return 0;
}


/**
 * @brief get sum of hugepages at index's socket using in this library
 */
static int __fpga_shmem_get_socket_sum(
  int index
) {
  __fpga_shmem_check_health();
  int sum = 0;
  for (int i = 0; i < fpga_shmem_get_available_limit(); i++)
    sum += manager_infos[i].socket_limit[index];
  return sum;
}


/**
 * @brief Check required memory size is possible
 */
static int __fpga_shmem_check_memory(
  const uint32_t socket_limit[]
) {
  __fpga_shmem_check_health();
  if (socket_limit == NULL)
    return 0;
  for (int i = 0; i < fpga_shmem_get_socket_num(); i++) {
    int available_per_socket = __fpga_shmem_get_socket_limit()[i] - __fpga_shmem_get_socket_sum(i);
    if (socket_limit[i] > available_per_socket) {
      llf_err(INVALID_ARGUMENT, "Invalid operation: Exceeded available memory at socket[%d](input/avail/limit)=(%d/%d/%d)\n",
        i, socket_limit[i], available_per_socket, __fpga_shmem_get_socket_limit()[i]);
      return -1;
    }
  }
  return 0;
}


int fpga_shmem_manager_init(
  const char *file_prefix,
  const uint32_t socket_limit[],
  const bool lcore_mask[],
  const shmem_func clb,
  void *arg
) {
  int ret = 0;

  // Check input
  if (__fpga_shmem_check_file_prefix(file_prefix)) {
    goto invalid_arg;
  }
  if (__fpga_shmem_check_memory(socket_limit)) {
    goto invalid_arg;
  }
  llf_dbg("%s(file_prefix(%s), limit(%#llx), lcore_mask(%#llx), clb(%#llx), arg(%#llx)\n",
    __func__, file_prefix, (uint64_t)socket_limit, (uint64_t)lcore_mask, (uint64_t)clb, (uint64_t)arg);

  // Need to set signal to finish Child process safely, before manager initialize
  if (!signal_set) {
    llf_err(NOT_REGISTERED_SIGNAL, "Invalid operation: Signal has NOT been registerd yet.\n");
    return -NOT_REGISTERED_SIGNAL;
  }

  // Make directory for protectecting invalid user looking the other users' hugepage
  char huge_dir[SHMEM_MAX_FILE_NAME_LEN];
  sprintf(huge_dir, "/dev/hugepages/%s", file_prefix);  // NOLINT
  if (mkdir(huge_dir, 0777)) {
    int err = errno;
    if (err != EEXIST) {
      llf_err(FAILURE_MKDIR, "Failed to mkdir %s(errno:%d)\n", huge_dir, err);
      return -FAILURE_MKDIR;
    }
  }

  // fork
  pid_t pid;
  pid = fork();
  if (pid < 0) {
    // fork error
    int err = errno;
    llf_err(FAILURE_FORK, "Failed to fork process for ShmemManager(errno:%d)\n", err);
    return -FAILURE_FORK;
  }

  if (pid == 0) {
    // Child process
    libfpga_log_reset_output_file();
    log_libfpga(LIBFPGA_LOG_DEBUG, LIBSHMEM "[MANAGER:pid(%d)] "
      "%s(file_prefix(%s), limit(%#llx), lcore_mask(%#llx), clb(%#llx), arg(%#llx)\n",
      getpid(), __func__, file_prefix, (uint64_t)socket_limit, (uint64_t)lcore_mask, (uint64_t)clb, (uint64_t)arg);
    __fpga_shmem_child_process(file_prefix, socket_limit, lcore_mask, clb, arg);
    // call exit() in __fpga_shmem_child_process function's all route
  }

  // Parent process
  ret = __fpga_shmem_parent_process(pid, file_prefix, socket_limit);

  return ret;

invalid_arg:
  llf_err(INVALID_ARGUMENT, "%s(file_prefix(%s), limit(%#llx), lcore_mask(%#llx), clb(%#llx), arg(%#llx)\n",
    __func__, file_prefix ? file_prefix : "<null>",
    (uint64_t)socket_limit, (uint64_t)lcore_mask, (uint64_t)clb, (uint64_t)arg);
  return -INVALID_ARGUMENT;
}


int fpga_shmem_manager_finish(
  const char *file_prefix
) {
  // Check if file_prefix is NULL
  if (!file_prefix) {
    llf_err(INVALID_ARGUMENT, "%s(file_prefix(<null>))\n", __func__);
    return -INVALID_ARGUMENT;
  }
  llf_dbg("%s(file_prefix(%s))\n", __func__, file_prefix);

  int ret = 1;
  int index;

  // Check if file_prefix is exist
  for (index = 0; index < fpga_shmem_get_available_limit(); index++) {
    if (!manager_infos[index].file_prefix[0]) {
      continue;
    }
    ret = strcmp(manager_infos[index].file_prefix, file_prefix);
    if (!ret) {
      break;
    }
  }
  if (ret) {
    llf_err(MISMATCH_FILE_PREFIX, "Invalid operation: File-prefix(%s) NOT found.\n", file_prefix);
    return -MISMATCH_FILE_PREFIX;
  }

  // in the right way, pid must be bigger than 0
  if (manager_infos[index].pid <= 0) {
    llf_err(LIBFPGA_FATAL_ERROR, "Fatal error: Invalid pid(pid:%d,index:%d)\n", manager_infos[index].pid, index);
    return -LIBFPGA_FATAL_ERROR;
  }

  // kill():send signal to Child process
  ret = kill(manager_infos[index].pid, SIGUSR1);
  if (ret) {
    int err = errno;
    llf_err(FAILURE_SEND_SIGNAL, "Failed to kill Child process(pid:%d)(errno:%d)\n", manager_infos[index].pid, err);
    return -FAILURE_SEND_SIGNAL;
  }

  return 0;
}


int fpga_shmem_manager_finish_all(void) {
  llf_dbg("%s()\n", __func__);

  int ret;
  int index;

  // kill all Child process
  for (index = 0; index < fpga_shmem_get_available_limit(); index++) {
    if (manager_infos[index].file_prefix[0]) {
      ret = fpga_shmem_manager_finish(manager_infos[index].file_prefix);
      if (ret) {
        llf_err(-ret, "Failed to finish Child process(pid:%d)\n", manager_infos[index].pid);
        return ret;
      }
    }
  }

  int cnt = 0;
  const int max_cnt = 20;
  while (cnt < max_cnt) {
    usleep(500 * 1000);
    // Check all Child process finish in success
    if (fpga_shmem_get_available_pages() == fpga_shmem_get_available_limit()) {
      break;
    }
    cnt++;
    llf_dbg(" Waiting for finish all managers(try/all)=(%d/%d)\n", cnt, max_cnt);
  }

  // Failure
  if (cnt == 20) {
    llf_err(LIBFPGA_FATAL_ERROR, "Fatal error: Cannot check whether all Child process died or not.\n");
    return -LIBFPGA_FATAL_ERROR;
  }

  return 0;
}


pid_t fpga_shmem_get_pid_from_prefix(
  const char* file_prefix
) {
  __fpga_shmem_check_health();
  for (int index = 0; index < fpga_shmem_get_available_limit(); index++) {
    if (!strcmp(manager_infos[index].file_prefix, file_prefix))
      return manager_infos[index].pid;
  }
  return 0;
}


int fpga_shmem_get_available_pages(void) {
  llf_dbg("%s()\n", __func__);
  int limit;
  int available_pages = 0;

  __fpga_shmem_check_health();
  limit = fpga_shmem_get_available_limit();
  for (int i = 0; i < limit; i++) {
    if (manager_infos[i].pid < 0) {
      continue;
    }
    available_pages += manager_infos[i].hp_limit;
  }

  return (limit - available_pages);
}


// cppcheck-suppress unusedFunction
int fpga_shmem_set_available_limit(
  const uint32_t socket_limit[]
) {
  llf_dbg("%s(socket_limit(%#llx))\n", __func__, (uint64_t)socket_limit);

  int hugepage_num = 0;
  for (int i = 0 ; i < fpga_shmem_get_socket_num(); i++) {
    hugepage_num += socket_limit[i];
  }
  if (hugepage_num < 0 && hugepage_num > SHMEM_MAX_HUGEPAGES) {
    return -INVALID_ARGUMENT;
  }

  if (fpga_shmem_get_available_limit() - fpga_shmem_get_available_pages() > hugepage_num) {
    return -INVALID_ARGUMENT;
  }
  for (int i = 0 ; i < fpga_shmem_get_socket_num(); i++) {
    __fpga_shmem_get_socket_limit()[i] = socket_limit[i];
  }
  *__fpga_shmem_get_available_limit() = hugepage_num;
  return 0;
}

int fpga_shmem_get_available_limit(void) {
  return *__fpga_shmem_get_available_limit();
}


// cppcheck-suppress unusedFunction
int fpga_shmem_set_lcore_limit(
  int lcore_limit
) {
  if (lcore_limit < 0 || lcore_limit > SHMEM_MAX_LCORE) {
    llf_err(INVALID_ARGUMENT, "%s(%d)\n", __func__, lcore_limit);
    return -INVALID_ARGUMENT;
  }
  llf_dbg("%s(lcore_limit(%d))\n", __func__, lcore_limit);
  *__fpga_shmem_get_lcore_limit() = lcore_limit;
  return 0;
}


int fpga_shmem_get_socket_num(void) {
  return *__fpga_shmem_get_socket_num();
}


// cppcheck-suppress unusedFunction
int fpga_shmem_get_lcore_limit(void) {
  return *__fpga_shmem_get_lcore_limit();
}


void fpga_shmem_dump_manager_infos(
  fpga_shmem_manager_info_t *data
) {
  llf_dbg("%s(data(%#llx))\n", __func__, (uint64_t)data);
  __fpga_shmem_check_health();

  if (data) {
    memcpy(data, manager_infos, sizeof(manager_infos));
  } else {
    llf_pr(
      "file_prefix     "
      "hugepage_limit  "
      "pid             "
      "is_initialized  \n");

    for (int index = 0; index < fpga_shmem_get_available_limit(); index++) {
      llf_pr("%-16s%-16d%-16d%-8d\n",
        manager_infos[index].file_prefix,
        manager_infos[index].hp_limit,
        manager_infos[index].pid,
        manager_infos[index].is_initialized);
    }
  }
}
