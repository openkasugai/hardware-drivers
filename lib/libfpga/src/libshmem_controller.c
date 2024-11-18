/*************************************************
* Copyright 2024 NTT Corporation, FUJITSU LIMITED
* Licensed under the 3-Clause BSD License, see LICENSE for details.
* SPDX-License-Identifier: BSD-3-Clause
*************************************************/

#include <libshmem_controller.h>
#include <libshmem_socket.h>
#include <liblogging.h>

#include <libfpga_internal/libdpdkutil.h>

#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/file.h>
#include <pthread.h>


// LogLibFpga
#define LIBSHMEM_CONTROLLER "[CTRL] "
#undef FPGA_LOGGER_LIBNAME
#define FPGA_LOGGER_LIBNAME LIBSHMEM LIBSHMEM_CONTROLLER


#define SHMEM_MAX_CHKCNT 10  /**< Max num to check shmem controller established */


/**
 * @enum shmem_ctrl_cmd_t
 *   shmem controller's command id
 */
typedef enum SHMEM_CTRL_CMD {
  SHMEM_CTRL_CMD_NONE = 0,                    /**< nothing */
  SHMEM_CTRL_CMD_START,                       /**< create shmem manager */
  SHMEM_CTRL_CMD_START_DEFAULT_SIZE,          /**< create shmem manager without setting size */
  SHMEM_CTRL_CMD_STOP,                        /**< delete shmem manager */
  SHMEM_CTRL_CMD_GET_LIMIT,                   /**< get Hugepage's limit pages */
  SHMEM_CTRL_CMD_GET_AVAIL,                   /**< get Hugepage's available pages */
  SHMEM_CTRL_CMD_GET_INFO,                    /**< get Hugepage's information */
  SHMEM_CTRL_CMD_GET_MANAGER_PID,             /**< get shmem manager's pid */
  SHMEM_CTRL_CMD_INITIALIZE = 0xFE,           /**< initialize shmem controller */
  SHMEM_CTRL_CMD_FINISH_MANAGERS_ALL = 0xFF,  /**< delete all shmem managers and controller */
} shmem_ctrl_cmd_t;


/**
 * @struct fpga_shmem_ctrl_info_t
 * @brief Struct for shmem controller's command
 * @var fpga_shmem_ctrl_info_t::cmd
 *      command for fpga's shmem controller
 * @var fpga_shmem_ctrl_info_t::file_prefix
 *      DPDK's file prefix
 * @var fpga_shmem_ctrl_info_t::socket_limit
 *      Num of hugepages per a socket
 * @var fpga_shmem_ctrl_info_t::lcore_mask
 *      [TBD]cpu logical core mask
 * @var fpga_shmem_ctrl_info_t::log_flag
 *      [TBD]DPDK's log flag
 */
typedef struct fpga_shmem_ctrl_info {
  shmem_ctrl_cmd_t cmd;
  char file_prefix[SHMEM_MAX_HUGEPAGE_PREFIX];
  uint32_t socket_limit[SHMEM_MAX_NUMA_NODE];
  bool lcore_mask[SHMEM_MAX_LCORE];
  int log_flag;
}fpga_shmem_ctrl_info_t;


/**
 * static global variable: thread id to get prefix of secondary process falling out
 */
static pthread_t fpga_shmem_recv_tid = 0;

/**
 * static global variable: mutex for handling error file_prefix list
 */
static pthread_mutex_t fpga_shmem_recv_tid_mutex
  = PTHREAD_MUTEX_INITIALIZER;

/**
 * static global variable: error file_prefix list(LIFO)
 */
static char **fpga_shmem_fallingout_prefix_list
  = NULL;

/**
 * static global variable: Callback function for shmem_manager
 */
static shmem_func fpga_shmem_callback_function
  = callback_function_notification_prefix;

/**
 * static global variable: Listen port for shmem controller
 */
static unsigned short shmem_ctrlr_listen_port
  = SHMEM_CONTROLLER_PORT;

/**
 * static global variable: Listen port for shmem controller
 */
static char shmem_ctrlr_listen_addr[32]
  = LOCALHOST;


/**
 * static global variable: Listen port for shmem controller's notice thread
 */
static unsigned short shmem_ctrlr_notice_listen_port
  = SHMEM_CONTROLLER_PORT_NOTICE;

/**
 * static global variable: Listen port for shmem controller's notice thread
 */
static char shmem_ctrlr_notice_listen_addr[32]
  = LOCALHOST;


static void __fpga_shmem_fallingout_notification_thread(void) {
  llf_dbg("[%s]\n", __func__);

  int ret;

  int fd;
  struct sockaddr_in server;

  char *recv_prefix = NULL;
  socklen_t recv_len;

  shmem_socket_response_t send_response;

reconnect:
  // Accept at fpga_shmem_recv_tid thread of parent process
  fd = fpga_shmem_get_fd_server(
    &server,
    shmem_ctrlr_notice_listen_port,
    shmem_ctrlr_notice_listen_addr);
  if (fd < 0) {
    llf_err(-fd, "[%s]Failed to establish connection...\n", __func__);
    sleep(1);
    goto reconnect;
  }

  do {
    // Receive error file_prefix from shmem manager
    ret = fpga_shmem_recv(fd, (void**)&recv_prefix, &recv_len); // NOLINT

    if (ret == 1) {
      // connection lost
      close(fd);
      fd = -1;
      if (recv_prefix) {
        free(recv_prefix);
        recv_prefix = NULL;
      }
      goto reconnect;
    } else if (ret < 0) {
      send_response = RES_NG;
      llf_err(-ret, "Failed to receive file_prefix at %s...\n", __func__);
    } else {
      send_response = RES_OK;
      // Set file_prefix at global static variable of main process
      fpga_shmem_set_error_prefix(recv_prefix);
      if (recv_prefix) {
        free(recv_prefix);
        recv_prefix = NULL;
      }
    }

    ret = fpga_shmem_send(fd, (void*)&send_response, sizeof(send_response));  // NOLINT
    if (ret < 0) {
      llf_err(-ret, "Failed to send response at %s...\n", __func__);
    }
  }while(1);
}


// cppcheck-suppress unusedFunction
int fpga_shmem_controller_init(
  unsigned short port __attribute__((unused)),
  const char *addr __attribute__((unused))
) {
  llf_dbg("%s(port(%d), addr(%s))\n", __func__, port, addr ? addr : "<null>");

  int ret;

  // Launch shmem controller in background(at child process)
  pid_t pid = fork();
  if (pid < 0) {
    int err = errno;
    llf_err(FAILURE_FORK, "Failed to fork for shmem_controller(error:%d)\n", err);
    return -FAILURE_FORK;
  } else if (pid == 0) {
    // Change logfile from parent process to avoid being overwritten
    libfpga_log_reset_output_file();
    llf_dbg("[CONTROLLER] %s(port(%d), addr(%s)\n", __func__, port, addr ? addr : "<null>");

    // Execute shmem_controller
    ret = fpga_shmem_controller_launch(port, addr ? addr : "<null>");

    // Should not return main process
    exit(abs(ret));
  }

  // Wait for shmem controller launching
  struct sockaddr_in connector;
  fpga_shmem_ctrl_info_t command;
  command.cmd = SHMEM_CTRL_CMD_INITIALIZE;
  shmem_socket_response_t *recv_response = NULL;
  socklen_t recv_len;
  for (int chkcnt = 0; chkcnt < SHMEM_MAX_CHKCNT; chkcnt++) {
    sleep(1);
    llf_dbg(" Check connection with shmem controller(%d/%d) start.\n", chkcnt + 1, SHMEM_MAX_CHKCNT);
    int client_fd = fpga_shmem_get_fd_client(
      &connector,
      shmem_ctrlr_listen_port,
      shmem_ctrlr_listen_addr);
    if (client_fd < 0) {
      continue;
    }
    recv_response = NULL;
    (void)fpga_shmem_send(client_fd, (void*)&command, sizeof(command));  // NOLINT
    (void)fpga_shmem_recv(client_fd, (void**)&recv_response, &recv_len);  // NOLINT
    if (recv_response && (*recv_response && RES_INIT)) {
      llf_dbg(" Check connection with shmem controller(%d/%d) OK.\n", chkcnt + 1, SHMEM_MAX_CHKCNT);
      free(recv_response);
      close(client_fd);
      break;
    }
    llf_dbg(" Check connection with shmem controller(%d/%d) NG...\n", chkcnt + 1, SHMEM_MAX_CHKCNT);

    if (recv_response)
      free(recv_response);
    close(client_fd);
  }

  if (!fpga_shmem_recv_tid) {
    ret = pthread_create(
      &fpga_shmem_recv_tid,
      NULL,
      (void*)__fpga_shmem_fallingout_notification_thread,  // NOLINT
      NULL);
    if (ret) {
      int err = errno;
      llf_err(FAILURE_ESTABLISH, "Failed to create thread(errno:%d)\n", err);
      return -FAILURE_ESTABLISH;
    }
  }

  return 0;
}


static int __fpga_shmem_controller_execute(
  int fd,
  fpga_shmem_ctrl_info_t command,
  shmem_socket_response_t *send_response,
  void **send_data,
  socklen_t *send_len
) {
  llf_dbg("%s(fd(%d))\n", __func__, fd);

  int ret = 0;
  int err;

  char lock_file[SHMEM_MAX_FILE_NAME_LEN];

  *send_response = RES_OK;

  switch (command.cmd) {
  case SHMEM_CTRL_CMD_START:
    // Launch shmem manager with file_prefix/socket_limit
    llf_dbg(" SHMEM_CTRL_CMD_START\n");
    sprintf(lock_file, SHMEM_FMT_FLOCK_FILE, command.file_prefix);  // NOLINT
    ret = fpga_shmem_manager_init(
      command.file_prefix,
      command.socket_limit,
      NULL,
      fpga_shmem_callback_function,
      (void*)lock_file);  // NOLINT
    if (ret) {
      *send_response = RES_NG;
    }
    break;

  case SHMEM_CTRL_CMD_START_DEFAULT_SIZE:
    // Launch shmem manager with file_prefix and WITHOUT socket_limit
    llf_dbg(" SHMEM_CTRL_CMD_START_DEFAULT_SIZE\n");
    sprintf(lock_file, SHMEM_FMT_FLOCK_FILE, command.file_prefix);  // NOLINT
    ret = fpga_shmem_manager_init(
      command.file_prefix,
      NULL,
      NULL,
      fpga_shmem_callback_function,
      (void*)lock_file);  // NOLINT
    if (ret) {
      *send_response = RES_NG;
    }
    break;

  case SHMEM_CTRL_CMD_STOP:
    // Finish shmem manager which matches file_prefix
    llf_dbg(" SHMEM_CTRL_CMD_STOP\n");
    ret = fpga_shmem_manager_finish(command.file_prefix);

    // Check if the same file_prefix manager is already dead or not
    if (ret && ret != -MISMATCH_FILE_PREFIX) {
      *send_response = RES_NG;
    }
    if (ret == -MISMATCH_FILE_PREFIX) {
      llf_dbg(" No need to stop shmem manager(%s)\n", command.file_prefix);
    }

    /* Delete lock file(used for user pod's finish detect).
     *  The lock_file should have been deleted by fpga_shmem_menager_finish(),
     *  but, in case, try to delete lock_file by controller.*/
    sprintf(lock_file, SHMEM_FMT_FLOCK_FILE, command.file_prefix);  // NOLINT
    do {
      ret = unlink(lock_file);
      err = errno;
    } while (ret && err == EBUSY);
    if (ret && err != ENOENT) {
      *send_response = RES_NG;
      llf_err(LIBFPGA_FATAL_ERROR, " Failed to delete %s(errno:%d)\n", lock_file, err);
    } else {
      llf_dbg(" Succeed to delete %s.\n", lock_file);
    }
    break;

  case SHMEM_CTRL_CMD_GET_MANAGER_PID:
    // Get pid of shmem manager(alive > 0)
    llf_dbg(" SHMEM_CTRL_CMD_GET_MANAGER_PID\n");
    *send_len = sizeof(pid_t);
    *((pid_t**)send_data) = (pid_t*)malloc(*send_len);  // NOLINT
    **((pid_t**)send_data) = fpga_shmem_get_pid_from_prefix(command.file_prefix); // NOLINT
    llf_dbg("  file_prefix(%s) : PID(%d)\n", command.file_prefix, **((pid_t**)send_data));  // NOLINT
    break;

  case SHMEM_CTRL_CMD_GET_AVAIL:
    // Get num of available hugepages
    llf_dbg(" SHMEM_CTRL_CMD_GET_AVAIL\n");
    llf_info("  fpga_shmem_get_available_pages() = %d\n", fpga_shmem_get_available_pages());
    break;

  case SHMEM_CTRL_CMD_GET_LIMIT:
    // Get max num of available hugepages
    llf_dbg(" SHMEM_CTRL_CMD_GET_LIMIT\n");
    llf_info("  fpga_shmem_get_available_limit() = %d\n", fpga_shmem_get_available_limit());
    break;

  case SHMEM_CTRL_CMD_GET_INFO:
    // Dump shmem controller's info
    llf_dbg(" SHMEM_CTRL_CMD_GET_INFO\n");
    fpga_shmem_dump_manager_infos(NULL);
    break;

  case SHMEM_CTRL_CMD_INITIALIZE:
    // Return Initialize Success
    llf_dbg(" SHMEM_CTRL_CMD_INITIALIZE\n");
    *send_response = RES_INIT;
    break;

  case SHMEM_CTRL_CMD_FINISH_MANAGERS_ALL:
    // Finish all shmem managers and controller
    llf_dbg(" SHMEM_CTRL_CMD_FINISH_MANAGERS_ALL\n");
    ret = fpga_shmem_manager_finish_all();
    if (ret)
      llf_err(-ret, " Failed to finish all shmem managers\n");
    else
      llf_dbg(" Succeed to finish all shmem managers.\n");
    *send_response = RES_QUIT;
    break;

  default:
    break;
  }
  return ret;
}


int fpga_shmem_controller_launch(
  unsigned short port,
  const char *addr
) {
#ifdef ENABLE_SHMEM_LISTEN_PORT_SETTABLE
  if (!addr) {
    llf_err(INVALID_ARGUMENT, "%s(port(%d), addr(%s))\n", __func__, port, "<null>");
    return -INVALID_ARGUMENT;
  }
  llf_dbg("%s(port(%d), addr(%s))\n", __func__, port, addr);
  shmem_ctrlr_listen_port = port;
  memset(shmem_ctrlr_listen_addr, 0, sizeof(shmem_ctrlr_listen_addr));
  strncpy(shmem_ctrlr_listen_addr, addr, sizeof(shmem_ctrlr_listen_addr) - 1);
#else
  llf_dbg("%s(port(%d), addr(%s))<use default value>\n",
    __func__, shmem_ctrlr_listen_port, shmem_ctrlr_listen_addr);
#endif


  int ret;

  int fd;
  struct sockaddr_in server;

  fpga_shmem_ctrl_info_t *recv_command = NULL;
  socklen_t recv_len;

  shmem_socket_response_t send_response;
  void *send_data = NULL;
  socklen_t send_len = 0;

  // Set sigaction to finish shmem_manager safely
  ret = fpga_shmem_set_signal();
  if (ret) {
    llf_err(-ret, "Failed to set signal\n");
    return ret;
  }

  // Establish connection with shmem launcher
reconnect:
  fd = fpga_shmem_get_fd_server(
    &server,
    shmem_ctrlr_listen_port,
    shmem_ctrlr_listen_addr);
  if (fd < 0) {
    llf_err(-fd, "Failed to establish connection...\n");
    sleep(1);
    goto reconnect;
  }

  do {
    // Receive command
    ret = fpga_shmem_recv(fd, (void**)&recv_command, &recv_len);  // NOLINT
    if (ret == 1) {
      /* *** connection lost *** */
      close(fd);
      fd = -1;
      goto reconnect;
    } else if (ret < 0) {
      send_response = RES_NG;
      ret = fpga_shmem_send(fd, &send_response, sizeof(send_response));
      if (ret)
        llf_err(-ret, "Failed to send reponse of getting command...\n");
      continue;
    }
    if (!recv_command)
      continue;

    // Execute shmem controller task
    (void)__fpga_shmem_controller_execute(fd, *recv_command, &send_response, &send_data, &send_len);  // NOLINT
    free(recv_command);
    recv_command = NULL;

    // send response
    ret = fpga_shmem_send(fd, &send_response, sizeof(send_response));
    if (ret)
      llf_err(-ret, "Failed to send reponse of executing command...\n");
    if (send_response == RES_OK) {
      ret = fpga_shmem_send(fd, send_data, send_len);
      if (ret)
        llf_err(-ret, "Failed to send data of executing command...\n");
      if (send_data)
        free(send_data);
      send_data = NULL;
      send_len = 0;
    }

    // Check finish
    if (send_response == RES_QUIT) {
      // Get command to finish all managers and controller self
      sleep(1);
      break;
    }
  }while(1);

  close(fd);
  llf_dbg("%s() finish by right operation\n", __func__);
  return 0;
}


static int __fpga_shmem_controller_request(
  fpga_shmem_ctrl_info_t command,
  void *data
) {
  // Check validity of command
  switch (command.cmd) {
  case SHMEM_CTRL_CMD_START:
  case SHMEM_CTRL_CMD_START_DEFAULT_SIZE:
  case SHMEM_CTRL_CMD_STOP:
  case SHMEM_CTRL_CMD_GET_LIMIT:
  case SHMEM_CTRL_CMD_GET_AVAIL:
  case SHMEM_CTRL_CMD_GET_INFO:
  case SHMEM_CTRL_CMD_GET_MANAGER_PID:
  case SHMEM_CTRL_CMD_FINISH_MANAGERS_ALL:
    break;
  default:
    llf_err(INVALID_ARGUMENT, "Invalid operation: %d is not supported...\n", command.cmd);
    return -INVALID_ARGUMENT;
  }

  llf_dbg("%s(command(%d), data(%#llx))\n", __func__, command.cmd, (uintptr_t)data);
  int ret;

  // Establish connection with shmem controller
  struct sockaddr_in client;
  static int fd_client = -1;
  if (fd_client < 0) {
    fd_client = fpga_shmem_get_fd_client(
      &client,
      shmem_ctrlr_listen_port,
      shmem_ctrlr_listen_addr);
    if (fd_client < 0) {
      llf_err(FAILURE_ESTABLISH, "Failed to connect with shmem controller(%d)\n", fd_client);
      return -FAILURE_ESTABLISH;
    }
  }

  // Send command
  ret = fpga_shmem_send(fd_client, (void*)&command, sizeof(command));  // NOLINT
  if (ret < 0) {
    llf_err(FAILURE_TRANSFER, "Failed to send data to shmem controller(%d)\n", ret);
    return -FAILURE_TRANSFER;
  }

  // Receive response(OK/NG/quit)
  shmem_socket_response_t *recv_response = NULL;
  socklen_t recv_len;
  ret = fpga_shmem_recv(fd_client, (void**)&recv_response, &recv_len);  // NOLINT
  if (ret < 0 || !recv_response) {
    llf_err(FAILURE_TRANSFER, "Failed to recv response from shmem controller(%d)\n", ret);
    return -FAILURE_TRANSFER;
  }

  // Receive response(data)
  void *recv_data = NULL;
  if (*recv_response == RES_OK) {
    ret = fpga_shmem_recv(fd_client, &recv_data, &recv_len);
    if (ret < 0) {
      free(recv_response);
      llf_err(FAILURE_TRANSFER, "Failed to recv data from shmem controller(%d)\n", ret);
      return -FAILURE_TRANSFER;
    }
  }

  // Switch(response)
  switch (*recv_response) {
  case RES_QUIT:
    ret = RES_QUIT;
    llf_dbg(" Accept for finishing shmem controller\n");
    close(fd_client);
    fd_client = -1;
    break;

  case RES_OK:
    ret = 0;
    // Switch(command)
    switch (command.cmd) {
    case SHMEM_CTRL_CMD_GET_MANAGER_PID:
      *((pid_t*)data) = *((pid_t*)recv_data);  // NOLINT
      llf_dbg(" (prefix,PID) = (%s,%d)\n", command.file_prefix, *((pid_t*)data));  // NOLINT
      break;
    case SHMEM_CTRL_CMD_GET_AVAIL:
    case SHMEM_CTRL_CMD_GET_LIMIT:
      llf_dbg(" recv_data = %d\n", (*(int*)recv_data));  // NOLINT
      break;
    case SHMEM_CTRL_CMD_GET_INFO:
      llf_dbg("file_prefix     hugepage_limit  pid             is_initialized  \n");
      fpga_shmem_manager_info_t *shmem_dumped_data = (fpga_shmem_manager_info_t*)recv_data; // NOLINT
      for (int index = 0; index < SHMEM_MAX_HUGEPAGES; index++) {
        llf_dbg("%-16s%-16d%-16d%-8d\n",
          shmem_dumped_data[index].file_prefix,
          shmem_dumped_data[index].hp_limit,
          shmem_dumped_data[index].pid,
          shmem_dumped_data[index].is_initialized);
      }
      break;
    default:
      break;
    }  // Switch(command)
    break;

  default:
    ret = -FAILURE_CONTROLLER;
    // Switch(command)
    switch (command.cmd) {
    case SHMEM_CTRL_CMD_START:
    case SHMEM_CTRL_CMD_START_DEFAULT_SIZE:
      llf_err(-ret, "Failed to establish Manager(%s)\n", command.file_prefix);
      break;
    case SHMEM_CTRL_CMD_STOP:
      llf_err(-ret, "Failed to finish Manager(%s)\n", command.file_prefix);
      break;
    case SHMEM_CTRL_CMD_GET_MANAGER_PID:
      llf_err(-ret, "Failed to get Manager PID of %s\n", command.file_prefix);
      break;
    default:
      llf_err(-ret, "Failed to execute command: %d\n", command.cmd);
      break;
    }  // Switch(command)
    break;
  }  // Switch(response)

  free(recv_response);
  if (recv_data)
    free(recv_data);

  return ret;
}


int fpga_shmem_enable(
  const char *file_prefix,
  const uint32_t socket_limit[]
) {
//  if ((!file_prefix) || (!socket_limit)) {
  if (!file_prefix) {
    llf_err(INVALID_ARGUMENT, "%s(file_prefix(<null>), socket_limit(%#llx))\n", __func__, (uint64_t)socket_limit);
    return -INVALID_ARGUMENT;
  }
  llf_dbg("%s(file_prefix(%s), socket_limit(%#llx))\n", __func__, file_prefix, (uint64_t)socket_limit);

  fpga_shmem_ctrl_info_t command;
  memset(&command, 0, sizeof(command));

  memcpy(command.file_prefix, file_prefix, sizeof(command.file_prefix));
  if (socket_limit) {
    command.cmd = SHMEM_CTRL_CMD_START;
    memcpy(command.socket_limit, socket_limit, sizeof(command.socket_limit));
  } else {
    command.cmd = SHMEM_CTRL_CMD_START_DEFAULT_SIZE;
  }

  return __fpga_shmem_controller_request(
    command,
    NULL);
}


// cppcheck-suppress unusedFunction
int fpga_shmem_enable_with_check(
  const char *file_prefix,
  const uint32_t socket_limit[]
) {
//  if ((!file_prefix) || (!socket_limit)) {
  if (!file_prefix) {
    llf_err(INVALID_ARGUMENT, "%s(file_prefix(<null>), socket_limit(%#llx))\n", __func__, (uint64_t)socket_limit);
    return -INVALID_ARGUMENT;
  }
  llf_dbg("%s(file_prefix(%s), socket_limit(%#llx))\n", __func__, file_prefix, (uint64_t)socket_limit);

  int ret = fpga_shmem_enable(file_prefix, socket_limit);
  if (ret)
    return ret;

  // Sleep at least 1500ms = 1.5s to wait for reflection after disable/enable and before check_inuse
  usleep(1500 * 1000);

  uint32_t is_inuse;
  if ((ret = fpga_shmem_check_inuse(file_prefix, &is_inuse))) return ret;
  if (!is_inuse) return fpga_shmem_enable(file_prefix, socket_limit);

  uint32_t is_error;
  if ((ret = fpga_shmem_check_error_prefix(file_prefix, &is_error))) return ret;
  if (is_error) {
    if ((ret = fpga_shmem_disable(file_prefix))) return ret;
    // Sleep at least 1500ms = 1.5s to wait for reflection after disable/enable and before check_inuse
    usleep(1500 * 1000);
    if ((ret = fpga_shmem_check_inuse(file_prefix, &is_inuse))) return ret;
    if (!is_inuse) {
      fpga_shmem_delete_error_prefix(file_prefix);
      return fpga_shmem_enable(file_prefix, socket_limit);
    }
    return -FAILURE_ESTABLISH;
  }

  return 0;
}


int fpga_shmem_disable(
  const char *file_prefix
) {
  if (!file_prefix) {
    llf_err(INVALID_ARGUMENT, "%s(file_prefix(<null>))\n", __func__);
    return -INVALID_ARGUMENT;
  }
  llf_dbg("%s(file_prefix(%s))\n", __func__, file_prefix);

  fpga_shmem_ctrl_info_t command;
  memset(&command, 0, sizeof(command));

  memcpy(command.file_prefix, file_prefix, sizeof(command.file_prefix));
  command.cmd = SHMEM_CTRL_CMD_STOP;

  return __fpga_shmem_controller_request(
    command,
    NULL);
}


static int __fpga_shmem_disable_with_check(
  const char *file_prefix,
  bool flag_forced
) {
  if (!file_prefix) {
    llf_err(INVALID_ARGUMENT, "%s(file_prefix(<null>), flag_forced(%c))\n",
      __func__, flag_forced ? 'T' : 'F');
    return -INVALID_ARGUMENT;
  }

  int ret;

  uint32_t is_inuse;
  if ((ret = fpga_shmem_check_inuse(file_prefix, &is_inuse))) {
    llf_err(-ret, "Failed to check inuse %s...\n", file_prefix);
    return ret;
  }
  if (!is_inuse) return 0;

  uint32_t is_error;
  if ((ret = fpga_shmem_check_error_prefix(file_prefix, &is_error))) {
    llf_err(-ret, "Failed to check if %s is error prefix...\n", file_prefix);
    return ret;
  }
  if (!is_error && !flag_forced) {
    char lock_file[SHMEM_MAX_FILE_NAME_LEN];
    memset(lock_file, 0, sizeof(lock_file));
    snprintf(lock_file, sizeof(lock_file), "/var/run/dpdk/%s/.lock", file_prefix);
    int fd = open(lock_file, O_RDONLY);
    if (fd >= 0) {
      close(fd);
      llf_err(INVALID_OPERATION, "Invalid operation: %s is using yet...\n", file_prefix);
      return -INVALID_OPERATION;
    }
  }

  if ((ret = fpga_shmem_disable(file_prefix))) return ret;
  // Sleep at least 1500ms = 1.5s to wait for reflection after disable/enable and before check_inuse
  usleep(1500 * 1000);
  if ((ret = fpga_shmem_check_inuse(file_prefix, &is_inuse))) return ret;
  if (!is_inuse) {
    fpga_shmem_delete_error_prefix(file_prefix);
    return 0;
  }
  return -LIBFPGA_FATAL_ERROR;
}


// cppcheck-suppress unusedFunction
int fpga_shmem_disable_forcibly(
  const char *file_prefix,
  bool flag_forced
) {
  if (!file_prefix) {
    llf_err(INVALID_ARGUMENT, "%s(file_prefix(<null>), flag_forced(%c))\n",
      __func__, flag_forced ? 'T' : 'F');
    return -INVALID_ARGUMENT;
  }
  llf_dbg("%s(file_prefix(%s), flag_forced(%c))\n",
      __func__, file_prefix, flag_forced ? 'T' : 'F');

  return __fpga_shmem_disable_with_check(file_prefix, flag_forced);
}


// cppcheck-suppress unusedFunction
int fpga_shmem_disable_with_check(
  const char *file_prefix
) {
  if (!file_prefix) {
    llf_err(INVALID_ARGUMENT, "%s(file_prefix(<null>))\n", __func__);
    return -INVALID_ARGUMENT;
  }
  llf_dbg("%s(file_prefix(%s))\n", __func__, file_prefix);

  return __fpga_shmem_disable_with_check(file_prefix, false);
}


int fpga_shmem_get_manager_pid(
  const char *file_prefix,
  pid_t *pid
) {
  if (!file_prefix || !pid) {
    llf_err(INVALID_ARGUMENT, "%s(file_prefix(%s), pid(%#llx))\n",
      __func__, file_prefix ? file_prefix : "<null>", (uintptr_t)pid);
    return -INVALID_ARGUMENT;
  }
  llf_dbg("%s(file_prefix(%s), pid(%#llx))\n", __func__, file_prefix, (uintptr_t)pid);

  fpga_shmem_ctrl_info_t command;
  memset(&command, 0, sizeof(command));

  memcpy(command.file_prefix, file_prefix, sizeof(command.file_prefix));
  command.cmd = SHMEM_CTRL_CMD_GET_MANAGER_PID;

  return __fpga_shmem_controller_request(
    command,
    (void*)pid);  // NOLINT
}


int fpga_shmem_check_inuse(
  const char *file_prefix,
  uint32_t *is_inuse
) {
  if (!file_prefix || !is_inuse) {
    llf_err(INVALID_ARGUMENT, "%s(file_prefix(%s), is_inuse(%#llx))\n",
      __func__, file_prefix ? file_prefix : "<null>", (uintptr_t)is_inuse);
    return -INVALID_ARGUMENT;
  }
  llf_dbg("%s(file_prefix(%s), is_inuse(%#llx))\n", __func__, file_prefix, (uintptr_t)is_inuse);

  pid_t pid;
  int ret = fpga_shmem_get_manager_pid(file_prefix, &pid);
  if (ret)
    return ret;

  if (pid > 0)
    *is_inuse = true;
  else
    *is_inuse = false;

  return 0;
}


// cppcheck-suppress unusedFunction
int fpga_shmem_controller_finish(void) {
  llf_dbg("%s()\n", __func__);

  int ret;
  fpga_shmem_ctrl_info_t command;
  memset(&command, 0, sizeof(command));

  command.cmd = SHMEM_CTRL_CMD_FINISH_MANAGERS_ALL;

  // Finish thread which receives notification of user pod finish
  if (fpga_shmem_recv_tid) {
    pthread_cancel(fpga_shmem_recv_tid);
    pthread_join(fpga_shmem_recv_tid, NULL);
  }

  ret = __fpga_shmem_controller_request(
    command,
    NULL);
  if (ret == RES_QUIT)
    return 0;
  return ret;
}


int fpga_shmem_set_error_prefix(
  const char *file_prefix
) {
  if (!file_prefix) {
    llf_err(INVALID_ARGUMENT, "%s(file_prefix(<null>))\n", __func__);
    return -INVALID_ARGUMENT;
  }
  llf_dbg("%s(file_prefix(%s))\n", __func__, file_prefix);

  pthread_mutex_lock(&fpga_shmem_recv_tid_mutex);
  // List not exist
  if (!fpga_shmem_fallingout_prefix_list) {
    // Create List and Insert file_prefix
    fpga_shmem_fallingout_prefix_list = (char**)malloc(sizeof(char*)*2);  // NOLINT
    *(fpga_shmem_fallingout_prefix_list + 0) = strdup(file_prefix);
    *(fpga_shmem_fallingout_prefix_list + 1) = NULL;
    pthread_mutex_unlock(&fpga_shmem_recv_tid_mutex);
    return 0;
  }

  // List exist
  char **temp_list;
  int num_prefix = 0;
  // Count elements of now List(except for the sentinel)
  for (; *(fpga_shmem_fallingout_prefix_list + num_prefix); num_prefix++) {}
  // Allocate memory of new List: (Counted_num) + (insert) + (sentinel)
  temp_list = (char**)malloc(sizeof(char*)*(num_prefix+2));  // NOLINT
  // Copy address of file_prefix from now List into new List
  for (int index = 0; index < num_prefix; index++)
    *(temp_list + index) = *(fpga_shmem_fallingout_prefix_list + index);
  // Free memory of now List
  if (fpga_shmem_fallingout_prefix_list)
    free(fpga_shmem_fallingout_prefix_list);

  // Change address of List from now List into new List, and insert new file_prefix
  fpga_shmem_fallingout_prefix_list = temp_list;
  *(fpga_shmem_fallingout_prefix_list + num_prefix) = strdup(file_prefix);
  *(fpga_shmem_fallingout_prefix_list + num_prefix + 1) = NULL;
  pthread_mutex_unlock(&fpga_shmem_recv_tid_mutex);
  return 0;
}


// cppcheck-suppress unusedFunction
const char* fpga_shmem_get_error_prefix(void) {
  llf_dbg("%s()\n", __func__);

  pthread_mutex_lock(&fpga_shmem_recv_tid_mutex);

  // List not exist
  if (!fpga_shmem_fallingout_prefix_list) {
    pthread_mutex_unlock(&fpga_shmem_recv_tid_mutex);
    return NULL;
  }

  // List exist
  int num_prefix = 0;
  // Count elements of now List(except for the sentinel)
  for (; *(fpga_shmem_fallingout_prefix_list + num_prefix); num_prefix++) {}
  if (!num_prefix) {
    // Only sentinel
    pthread_mutex_unlock(&fpga_shmem_recv_tid_mutex);
    return NULL;
  }

  pthread_mutex_unlock(&fpga_shmem_recv_tid_mutex);
  return *(fpga_shmem_fallingout_prefix_list + num_prefix - 1);
}


int fpga_shmem_check_error_prefix(
  const char *file_prefix,
  uint32_t *is_exist
) {
  if (!file_prefix || !is_exist) {
    llf_err(INVALID_ARGUMENT, "%s(file_prefix(%s), is_exist(%#llx))\n",
      __func__, file_prefix ? file_prefix : "<null>", (uintptr_t)is_exist);
    return -INVALID_ARGUMENT;
  }
  llf_dbg("%s(file_prefix(%s), is_exist(%#llx))\n",
    __func__, file_prefix, (uintptr_t)is_exist);

  pthread_mutex_lock(&fpga_shmem_recv_tid_mutex);

  *is_exist = false;

  // Check if list exists or not
  if (!fpga_shmem_fallingout_prefix_list) {
    pthread_mutex_unlock(&fpga_shmem_recv_tid_mutex);
    return 0;
  }

  // Check if file_prefix exists or not
  for (int num_prefix = 0; *(fpga_shmem_fallingout_prefix_list + num_prefix); num_prefix++) {
    if (strcmp(file_prefix, *(fpga_shmem_fallingout_prefix_list + num_prefix)) == 0) {
      *is_exist = true;
      break;
    }
  }

  pthread_mutex_unlock(&fpga_shmem_recv_tid_mutex);
  return 0;
}


int fpga_shmem_delete_error_prefix(
  const char *file_prefix
) {
  if (!file_prefix) {
    llf_err(INVALID_ARGUMENT, "%s(file_prefix(<null>))\n", __func__);
    return -INVALID_ARGUMENT;
  }
  llf_dbg("%s(file_prefix(%s))\n", __func__, file_prefix);

  pthread_mutex_lock(&fpga_shmem_recv_tid_mutex);

  // List not exist
  if (!fpga_shmem_fallingout_prefix_list) {
    pthread_mutex_unlock(&fpga_shmem_recv_tid_mutex);
    return -INVALID_DATA;
  }

  // List exist
  int num_prefix = 0, delete_index = -1;
  while (*(fpga_shmem_fallingout_prefix_list + num_prefix)) {
    if (strcmp(file_prefix, *(fpga_shmem_fallingout_prefix_list + num_prefix)) == 0)
      delete_index = num_prefix;
    num_prefix++;
  }
  if (!num_prefix) {
    pthread_mutex_unlock(&fpga_shmem_recv_tid_mutex);
    return -MISMATCH_FILE_PREFIX;
  }
  free(*(fpga_shmem_fallingout_prefix_list + delete_index));
  for (int i = delete_index; i < (num_prefix); i++) {
    *(fpga_shmem_fallingout_prefix_list + i) = *(fpga_shmem_fallingout_prefix_list + i + 1);
  }
  if (!(*fpga_shmem_fallingout_prefix_list)) {
    free(fpga_shmem_fallingout_prefix_list);
    fpga_shmem_fallingout_prefix_list = NULL;
  }

  pthread_mutex_unlock(&fpga_shmem_recv_tid_mutex);
  return 0;
}


// cppcheck-suppress unusedFunction
int fpga_shmem_controller_set_clb(
  const shmem_func clb
) {
  if (!clb) {
    llf_err(INVALID_ARGUMENT, "%s(clb(%#llx))\n", __func__, (uint64_t)clb);
    return -INVALID_ARGUMENT;
  }
  llf_dbg("%s(clb(%#llx))\n", __func__, (uint64_t)clb);

  fpga_shmem_callback_function = clb;

  return 0;
}


// cppcheck-suppress unusedFunction
int fpga_shmem_controller_set_ip(
  unsigned short port,
  const char *addr
) {
  if (!addr) {
    llf_err(INVALID_ARGUMENT, "%s(port(%d), addr(%s))\n", __func__, port, "<null>");
    return -INVALID_ARGUMENT;
  }
  llf_dbg("%s(port(%d), addr(%s))\n", __func__, port, addr);

  if (port == shmem_ctrlr_notice_listen_port && strcmp(addr, shmem_ctrlr_notice_listen_addr)) {
    llf_err(INVALID_DATA, "The IP address and port is the same as the notice port...\n");
    return -INVALID_DATA;
  }

  shmem_ctrlr_listen_port = port;
  memset(shmem_ctrlr_listen_addr, 0, sizeof(shmem_ctrlr_listen_addr));
  strncpy(shmem_ctrlr_listen_addr, addr, sizeof(shmem_ctrlr_listen_addr) - 1);

  return 0;
}


// cppcheck-suppress unusedFunction
int fpga_shmem_controller_get_ip(
  unsigned short *port,
  char **addr
) {
  if (!port || !addr) {
    llf_err(INVALID_ARGUMENT, "%s(port(%#llx), addr(%#llx))\n",
      __func__, (uintptr_t)port, (uintptr_t)addr);
    return -INVALID_ARGUMENT;
  }
  llf_dbg("%s(port(%#llx), addr(%#llx))\n", __func__, (uintptr_t)port, (uintptr_t)addr);

  *addr = strdup(shmem_ctrlr_listen_addr);
  if (!*addr) {
    llf_err(FAILURE_MEMORY_ALLOC, "Failed to allocate memory for IP address of shmem_controller.\n");
    return -FAILURE_MEMORY_ALLOC;
  }

  *port = shmem_ctrlr_listen_port;

  return 0;
}


// cppcheck-suppress unusedFunction
int fpga_shmem_controller_set_ip_notice(
  unsigned short port,
  const char *addr
) {
  if (!addr) {
    llf_err(INVALID_ARGUMENT, "%s(port(%d), addr(%s))\n", __func__, port, "<null>");
    return -INVALID_ARGUMENT;
  }
  llf_dbg("%s(port(%d), addr(%s))\n", __func__, port, addr);

  if (port == shmem_ctrlr_listen_port && strcmp(addr, shmem_ctrlr_listen_addr)) {
    llf_err(INVALID_DATA, "The IP address and port is the same as the controller port...\n");
    return -INVALID_DATA;
  }

  shmem_ctrlr_notice_listen_port = port;
  memset(shmem_ctrlr_notice_listen_addr, 0, sizeof(shmem_ctrlr_notice_listen_addr));
  strncpy(shmem_ctrlr_notice_listen_addr, addr, sizeof(shmem_ctrlr_notice_listen_addr) - 1);

  return 0;
}


// cppcheck-suppress unusedFunction
int fpga_shmem_controller_get_ip_notice(
  unsigned short *port,
  char **addr
) {
  if (!port || !addr) {
    llf_err(INVALID_ARGUMENT, "%s(port(%#llx), addr(%#llx))\n",
      __func__, (uintptr_t)port, (uintptr_t)addr);
    return -INVALID_ARGUMENT;
  }
  llf_dbg("%s(port(%#llx), addr(%#llx))\n", __func__, (uintptr_t)port, (uintptr_t)addr);

  *addr = strdup(shmem_ctrlr_notice_listen_addr);
  if (!*addr) {
    llf_err(FAILURE_MEMORY_ALLOC, "Failed to allocate memory for IP address of shmem_controller.\n");
    return -FAILURE_MEMORY_ALLOC;
  }

  *port = shmem_ctrlr_notice_listen_port;

  return 0;
}


int callback_function_notification_prefix(
  void* arg
) {
  static int func_name_print = 1, notice_once = 0;
  int ret = 0;

  if (unlikely(notice_once)) {
    return ret;
  }

  char *filename = (char*)arg;  // NOLINT
  static char file_prefix[SHMEM_MAX_HUGEPAGE_PREFIX];

  // cppcheck-suppress invalidscanf
  (void)sscanf(filename, "/var/run/dpdk/%[^/]/.lock", file_prefix);
  if (unlikely(func_name_print)) {
    llf_dbg("[%s]%s(%s)\n", __func__, filename, file_prefix);
    func_name_print = 0;
  }

  sleep(1);

  // Check if File is exist or not
  int fd = open(filename, O_RDONLY);
  if (fd < 0)
    return ret;

  // Check if File is locked by user
  int ex_lock = flock(fd, LOCK_EX | LOCK_NB);
  if (ex_lock < 0) {
    close(fd);
    return ret;
  }
  // Succeed to lock file, so user call fpga_shmem_finish() or killed

  // read data
  int buf, len;
  len = read(fd, &buf, sizeof(buf));
  close(fd);

  // read data
  notice_once = 1;
  if (len != 0) {
    llf_dbg("[%s]User pod NORMAL finish detect...\n", __func__);
    return 1; /* None 0 return means finish by manager */
  } else {
    llf_err(len, "[%s]User pod ERROR finish detect(read buffer:%dbyte)...\n",
      __func__, (len == 0 ? 0 : buf));
  }

  /* Establish connection with shmem_controller launcher's
   * fpga_shmem_recv_tid thread */
  struct sockaddr_in data;
  int client_fd = fpga_shmem_get_fd_client(
    &data,
    shmem_ctrlr_notice_listen_port,
    shmem_ctrlr_notice_listen_addr);
  if (client_fd < 0) {
    return -1;  /* should be finished by manager */
  }
  shmem_socket_response_t *recv_response = NULL;
  socklen_t recv_len;
  (void)fpga_shmem_send(client_fd, (void*)file_prefix, strlen(file_prefix) + 1);  // NOLINT
  (void)fpga_shmem_recv(client_fd, (void**)&recv_response, &recv_len);  // NOLINT
  if (!recv_response || (*recv_response == RES_NG)) {
    ret = -1;  /* should be finished by manager */
  }

  if (recv_response)
    free(recv_response);

  close(client_fd);

  return ret;
}
