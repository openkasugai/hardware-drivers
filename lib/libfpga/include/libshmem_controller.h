/*************************************************
* Copyright 2024 NTT Corporation, FUJITSU LIMITED
* Licensed under the 3-Clause BSD License, see LICENSE for details.
* SPDX-License-Identifier: BSD-3-Clause
*************************************************/
/**
 * @file libshmem_controller.h
 * @brief Header file for shared memory controller
 */

#ifndef LIBFPGA_INCLUDE_LIBSHMEM_CONTROLLER_H_
#define LIBFPGA_INCLUDE_LIBSHMEM_CONTROLLER_H_

#include <libshmem_manager.h>


#ifdef __cplusplus
extern "C" {
#endif

/**
 * shmem_controller's default listen tcp ip address
 */
#define LOCALHOST                     "127.0.0.1"

/**
 * shmem_controller's default listen tcp port
 */
#define SHMEM_CONTROLLER_PORT         60000

/**
 * shmem_controller's default listen tcp port for notice error prefix
 */
#define SHMEM_CONTROLLER_PORT_NOTICE  (SHMEM_CONTROLLER_PORT+1)


/**
 * @brief API which launch shmem_controller
 * @param[in] port
 *   TCP port(unused)
 * @param[in] addr
 *   TCP ip address(unused)
 * @retval 0
 *   Success
 * @retval -ALREADY_INITIALIZED
 *   e.g.) Signal is already set
 * @retval -FAILURE_REGISTER_SIGNAL
 *   e.g.) Failed sigaction()
 *
 * @details
 *   Execute shmem_controller tasks.@n
 *   Open TCP port and wait for receiving requests to create/delete shmem_manager(s).@n
 *   This API execute inf loop for the tasks, and exit the loop when fpga_shmem_controller_finish() is called.@n
 *   Request APIs are belows.
 * @sa fpga_shmem_enable
 * @sa fpga_shmem_disable
 * @sa fpga_shmem_get_manager_pid
 * @sa fpga_shmem_controller_finish
 * @note
 *   Generally, This API will not be called by user process.@n
 *   This API will be used by people who want to create shmem_controller by themselves.@n
 */
int fpga_shmem_controller_launch(
        unsigned short port,
        const char *addr);

/**
 * @brief API which launch controller in background
 * @param[in] port
 *   TCP port(unused)
 * @param[in] addr
 *   TCP ip address(unused)
 * @retval 0
 *   Success
 * @retval -FAILURE_FORK
 *   Failed fork()
 * @retval -FAILURE_ESTABLISH
 *   Failed pthread_create()
 *
 * @details
 *   Fork process and execute shmem_controller tasks in background.@n
 *   The parent process(launcher) will create a thread to get a file_prefix of shmem_manager
 *    which secondary process with the same file_prefix has been dead in error with.@n
 *   Please call fpga_shmem_controller_finish() to finish the thread created by parent process.@n
 *   The child process(shmem_controller) call fpga_shmem_controller_launch()
 *    and exit() the process in this API without returning main process.
 * @sa fpga_shmem_controller_launch
 */
int fpga_shmem_controller_init(
        unsigned short port __attribute__((unused)),
        const char *addr __attribute__((unused)));

/**
 * @brief API which request shmem_controller to launch shmem_manager
 * @param[in] file_prefix
 *   DPDK's file prefix
 * @param[in] socket_limit
 *   Request Hugepage size(GB) per a socket@n
 *   (value:NULL is allowed but there is no guarantee that shmem_controller will work properly if you use.)
 * @retval 0
 *   Success
 * @retval -INVALID_ARGUMENT
 *   e.g.) `file_prefix` is null
 * @retval -FAILURE_ESTABLISH
 *   e.g.) There is no process which execute fpga_shmem_controller_launch()
 * @retval -FAILURE_TRANSFER
 *   e.g.) Failed send()/recv()
 * @retval -FAILURE_MEMORY_ALLOC
 *   Failed to allocate memory
 * @retval -FAILURE_CONTROLLER
 *   e.g.) shmem_controller failed to create shmem_manager
 *
 * @details
 *   Request shmem_controller to launch shmem_manager.@n
 *   This API will succeed if there is a process
 *    which call fpga_shmem_controller_init() or fpga_shmem_controller_launch()
 *    even if the process which call this API have not call one of them.
 * @remarks
 *   If you use default callback function, the shmem_manager will finish
 *    when there were secondary process(= exist the lock file) and succeed to lock the file.
 *   So, the process as follows will fail:
 *   @li PRIMARY:fpga_shmem_controller_init()
 *   @li PRIMARY:fpga_shmem_enable()
 *   @li SECONDARY:fpga_shmem_init()
 *   @li SECONDARY:fpga_shmem_finish()
 *   @li PRIMARY:Automatically finish shmem_manager because file exist, not locked
 *   @li SECONDARY:fpga_shmem_init() <- Failure!!
 *   However, the process as follows will succeed:
 *   @li PRIMARY:fpga_shmem_controller_init()
 *   @li PRIMARY:fpga_shmem_enable()
 *   @li SECONDARY:fpga_shmem_init()
 *   @li SECONDARY:fpga_shmem_init() <- Success!!
 */
int fpga_shmem_enable(
        const char *file_prefix,
        const uint32_t socket_limit[]);

/**
 * @brief API which request shmem_controller to launch shmem_manager with checking
 * @param[in] file_prefix
 *   DPDK's file prefix
 * @param[in] socket_limit
 *   Request Hugepage size(GB) per a socket@n
 *   (value:NULL is allowed but there is no guarantee that shmem_controller will work properly if you use.)
 * @retval 0
 *   Success
 * @retval -FAILURE_ESTABLISH
 *   Failed to establish shmem_manager after disabling shmem_manager
 * @return
 *   @sa fpga_shmem_enable
 *   @sa fpga_shmem_check_inuse
 *   @sa fpga_shmem_check_error_prefix
 *   @sa fpga_shmem_disable
 *
 * @details
 *   Request shmem_controller to launch shmem_manager with check.@n
 *   Establish shmem_manager with using below APIs:
 *   @sa fpga_shmem_enable
 *   @sa fpga_shmem_check_inuse
 *   @sa fpga_shmem_check_error_prefix
 *   @sa fpga_shmem_disable
 */
int fpga_shmem_enable_with_check(
        const char *file_prefix,
        const uint32_t socket_limit[]);

/**
 * @brief API which request controller to finish shmem_manager
 * @param[in] file_prefix
 *   DPDK's file prefix
 * @retval 0
 *   Success
 * @retval -INVALID_ARGUMENT
 *   e.g.) `file_prefix` is null
 * @retval -FAILURE_ESTABLISH
 *   e.g.) There is no process which execute fpga_shmem_controller_launch()
 * @retval -FAILURE_TRANSFER
 *   e.g.) Failed send()/recv()
 * @retval -FAILURE_MEMORY_ALLOC
 *   Failed to allocate memory
 * @retval -FAILURE_CONTROLLER
 *   e.g.) shmem_controller failed to confirm successful completion of shmem_manager
 *
 * @details
 *   Request shmem_controller to finish shmem_manager.@n
 *   This API will not check if finalization succeeded or not.@n
 *   This API will succeed if there is a process
 *    which call fpga_shmem_controller_init() or fpga_shmem_controller_launch()
 *    even if the process which call this API have not call one of them.
 */
int fpga_shmem_disable(
        const char *file_prefix);

/**
 * @brief API which request shmem_controller to finish shmem_manager with checking
 * @param[in] file_prefix
 *   DPDK's file prefix
 * @param[in] flag_forced
 *   Flag to switch finishing shmem_manager forcibly
 * @retval 0
 *   Success
 * @retval -INVALID_OPERATION
 *   Maybe secondary process is still alive.
 * @return
 *   @sa fpga_shmem_check_inuse
 *   @sa fpga_shmem_check_error_prefix
 *   @sa fpga_shmem_disable
 *
 * @details
 *   Request shmem_controller to finish shmem_manager with check and
 *    can finish forcibly by `flag_forced`.@n
 *   When flag_forced is NOT enabled(=fpga_shmem_disable_with_check),
 *    this API will fail to finish shmem_manager not used yet.@n
 *   Finish shmem_manager with using below APIs:
 *   @sa fpga_shmem_check_inuse
 *   @sa fpga_shmem_check_error_prefix
 *   @sa fpga_shmem_disable
 */
int fpga_shmem_disable_forcibly(
        const char *file_prefix,
        bool flag_forced);

/**
 * @brief API which request shmem_controller to finish shmem_manager with checking
 * @param[in] file_prefix
 *   DPDK's file prefix
 * @retval 0
 *   Success
 * @retval -INVALID_OPERATION
 *   Maybe secondary process is still alive.
 * @return
 *   @sa fpga_shmem_check_inuse
 *   @sa fpga_shmem_check_error_prefix
 *   @sa fpga_shmem_disable
 *
 * @details
 *   Request shmem_controller to finish shmem_manager with check.@n
 *   Finish shmem_manager with using below APIs:
 *   @sa fpga_shmem_check_inuse
 *   @sa fpga_shmem_check_error_prefix
 *   @sa fpga_shmem_disable
 */
int fpga_shmem_disable_with_check(
        const char *file_prefix);

/**
 * @brief API which request controller to get manager's Process ID
 * @param[in] file_prefix
 *   DPDK's file prefix
 * @param[out] pid
 *   pointer variable to get PID of manager
 * @retval 0
 *   Success
 * @retval -INVALID_ARGUMENT
 *   e.g.) `file_prefix` is null, `pid` is NULL
 * @retval -FAILURE_ESTABLISH
 *   e.g.) There is no process which execute fpga_shmem_controller_launch()
 * @retval -FAILURE_TRANSFER
 *   e.g.) Failed send()/recv()
 * @retval -FAILURE_MEMORY_ALLOC
 *   Failed to allocate memory
 * @retval -FAILURE_CONTROLLER
 *   e.g.) shmem_controller failed to get pid of shmem_manager
 *
 * @details
 *   Request shmem_controller to send PID of shmem_manager to this API and get through `pid`.@n
 *   If the shmem_manager with `file_prefix` is alive, `pid` will be its process ID(i.e. larger than 0),
 *    and if not alive, `pid` will be 0.@n
 *   Please allocate memory for `*pid` by user.@n
 *   The value of `*pid` is undefined when this API fails.@n
 *   This API will succeed if there is a process
 *    which call fpga_shmem_controller_init() or fpga_shmem_controller_launch()
 *    even if the process which call this API have not call one of them.
 */
int fpga_shmem_get_manager_pid(
        const char *file_prefix,
        pid_t *pid);

/**
 * @brief API which request controller to check if target shmem_manager is inuse
 * @param[in] file_prefix
 *   DPDK's file prefix
 * @param[out] is_alive
 *   pointer variable to get the result.
 *   @li true(non-zero) : shmem_manager with `file_prefix` is inuse
 *   @li false(zero) : shmem_manager with `file_prefix` is NOT inuse
 * @retval 0
 *   Success
 * @retval -INVALID_ARGUMENT
 *   e.g.) `file_prefix` is null, `pid` is NULL
 * @retval -FAILURE_ESTABLISH
 *   e.g.) There is no process which execute fpga_shmem_controller_launch()
 * @retval -FAILURE_TRANSFER
 *   e.g.) Failed send()/recv()
 * @retval -FAILURE_MEMORY_ALLOC
 *   Failed to allocate memory
 * @retval -FAILURE_CONTROLLER
 *   e.g.) shmem_controller failed to get pid of shmem_manager
 *
 * @details
 *   Request shmem_controller to check a shmem_manager with `file_prefix` is inuse.@n
 *   This function wraps fpga_shmem_get_manager_pid() and
 *    if PID is over 1, set true into `*is_inuse`, otherwise set false.@n
 *   Please allocate memory for `*is_inuse` by user.@n
 *   The value of `*is_inuse` is undefined when this API fails.@n
 *   This API will succeed if there is a process
 *    which call fpga_shmem_controller_init() or fpga_shmem_controller_launch()
 *    even if the process which call this API have not call one of them.
 */
int fpga_shmem_check_inuse(
        const char *file_prefix,
        uint32_t *is_inuse);

/**
 * @brief API which request controller to finish all managers and controller
 * @param void
 * @retval 0
 *   Success
 * @retval -FAILURE_ESTABLISH
 *   e.g.) There is no process which execute fpga_shmem_controller_launch()
 * @retval -FAILURE_TRANSFER
 *   e.g.) Failed send()/recv()
 * @retval -FAILURE_MEMORY_ALLOC
 *   Failed to allocate memory
 * @retval -FAILURE_CONTROLLER
 *   e.g.) shmem_controller failed to confirm successful completion of all shmem_managers
 *
 * @details
 *   Request shmem_controller to finish all shmem_managers and shmem_controller self.@n
 *   Cancel the thread created by fpga_shmem_controller_init() and
 *    exit the inf loop created by fpga_shmem_controller_launch()
 *    after finishing all shmem_managers.
 */
int fpga_shmem_controller_finish(void);

/**
 * @brief API which set file_prefix into a global variable list
 * @param[in] file_prefix
 *   DPDK's file prefix
 * @retval 0
 *   Success
 * @retval -INVALID_ARGUMENT
 *   e.g.) `file_prefix` is null
 * @retval -FAILURE_MEMORY_ALLOC
 *   Failed to allocate memory
 *
 * @details
 *   Set `file_prefix` into the list.@n
 * @note
 *   Generally, This API will not be called by user process.@n
 *   The thread created by fpga_shmem_controller_init() will set a file_prefix of shmem_manager
 *    which secondary process with the same file_prefix has been dead in error with by this API.
 */
int fpga_shmem_set_error_prefix(
        const char *file_prefix);

/**
 * @brief API which get file_prefix from a global variable list(LIFO)
 * @param void
 * @return file_prefix
 *   error file_prefix.
 * @retval NULL
 *   e.g.) There is no file_prefix, Failed to get file_prefix
 *
 * @details
 *   Get `file_prefix` from the tail of the list.@n
 *   The `file_prefix` is a file_prefix which secondary process with the same file_prefix has been dead in error.@n
 *   The memory for the `*file_prefix` will be allocated by this library and return through retval,
 *    so please explicitly free by fpga_shmem_delete_error_prefix(),
 *    but please do NOT free by free().@n
 * @note
 *   This API will succeed to get file_prefix when fpga_shmem_controller_init() in the same process because
 *    fpga_shmem_set_error_prefix() will be called only in the thread created by fpga_shmem_controller_init().
 *   This API may fail to get file_prefix when you change callback function of shmem_manager
 *    because default callback function send the prefix to the thread.
 */
const char* fpga_shmem_get_error_prefix(void);

/**
 * @brief API which check if target file_prefix is in a global variable list(LIFO)
 * @param[in] file_prefix
 *   DPDK's file prefix
 * @param[out] is_success
 *   pointer variable to get the result.
 *   @li true(non-zero) : `file_prefix` exist in the list
 *   @li false(zero) : `file_prefix` does NOT exist in the list,list does NOT exit
 * @retval 0
 *   Success
 * @retval -INVALID_ARGUMENT
 *   e.g.) `file_prefix` is null
 *
 * @details
 *   Check if `file_prefix` is in the list.@n
 *   If `file_prefix` exist in the list, set true into `*is_exist`, otherwise set false.@n
 *   Please allocate memory for `*is_exist` by user.@n
 *   The value of `*is_exist` is undefined when this API fails.
 * @note
 *   This API will succeed to get file_prefix when fpga_shmem_controller_init() in the same process because
 *    fpga_shmem_set_error_prefix() will be called only in the thread created by fpga_shmem_controller_init().
 */
int fpga_shmem_check_error_prefix(
        const char *file_prefix,
        uint32_t *is_exist);

/**
 * @brief API which delete file_prefix from a global variable list
 * @param[in] file_prefix
 *   DPDK's file prefix
 * @retval 0
 *   Success
 * @retval -INVALID_ARGUMENT
 *   e.g.) `file_prefix` is null
 * @retval -INVALID_DATA
 *   e.g.) Not exist the list for error file_prefix
 * @retval -MISMATCH_FILE_PREFIX
 *   e.g.) Not found the same file_prefix from the list
 *
 * @details
 *   Delete `file_prefix` from the list.@n
 * @note
 *   This API will succeed to get file_prefix when fpga_shmem_controller_init() in the same process because
 *    fpga_shmem_set_error_prefix() will be called only in the thread created by fpga_shmem_controller_init().
 */
int fpga_shmem_delete_error_prefix(
        const char *file_prefix);

/**
 * @brief API which set callback function for shmem_manager
 * @param[in] clb
 *   shmem_manager's callback function
 * @retval 0
 *   Success
 * @retval -INVALID_ARGUMENT
 *   e.g.) `clb` is null
 *
 * @details
 *   Set a callback function which shmem_manager periodically calls.@n
 *   In this library, default callback function is callback_function_notification_prefix().@n
 *   This API should be called before calling fpga_shmem_controller_launch(), fpga_shmem_controller_init()
 *    if you want to set a callback function.
 * @note
 *   If you set callback function which will not send error prefix to the thread created by
 *    fpga_shmem_controller_init(), fpga_shmem_get_error_prefix() will not work properly
 */
int fpga_shmem_controller_set_clb(
        const shmem_func clb);

/**
 * @brief API which set IP address/port which shmem_controller TCP listen
 * @param[in] port
 *   TCP port
 * @param[in] addr
 *   TCP ip address
 * @retval 0
 *   Success
 * @retval -INVALID_ARGUMENT
 *   e.g.) `addr` is null
 * @retval -INVALID_DATA
 *   e.g.) Both of `port` and `addr` is the same with notice port of shmem_controller
 *
 * @details
 *   Set TCP IP address and port which shmem_controller listen to receive requests
 *    and fpga_shmem_enable(),... connect to send requests.@n
 *   If the data is the same with the notification addr/port,
 *    this API will fail to avoid binding the same addr/port.
 */
int fpga_shmem_controller_set_ip(
        unsigned short port,
        const char *addr);

/**
 * @brief API which get IP address/port which shmem_controller TCP listen
 * @param[out] port
 *   pointer variable to get TCP port
 * @param[out] addr
 *   pointer variable to get TCP ip address
 * @retval 0
 *   Success
 * @retval -INVALID_ARGUMENT
 *   e.g.) `port` is null, `addr` is null
 * @retval -FAILURE_MEMORY_ALLOC
 *   Failed to allocate memory
 *
 * @details
 *   Get TCP IP address and port which shmem_controller listen for receive requests.@n
 *   Please allocate memory for `*port` by user.@n
 *   `*addr` will be allocated by this API, so please explicitly free `*addr` by free().@n
 *   The value of `*port` and `*addr` are undefined when this API fails.
 */
int fpga_shmem_controller_get_ip(
        unsigned short *port,
        char **addr);

/**
 * @brief API which set IP address/port which launcher TCP listen
 * @param[in] port
 *   TCP port
 * @param[in] addr
 *   TCP ip address
 * @retval 0
 *   Success
 * @retval -INVALID_ARGUMENT
 *   e.g.) `addr` is null
 * @retval -INVALID_DATA
 *   e.g.) Both of `port` and `addr` is the same with port of shmem_controller
 *
 * @details
 *   Set TCP IP address and port which the thread created by fpga_shmem_controller_init()
 *    listen for receive the file_prefix which secondary process with the same file_prefix
 *    has been dead in error with.@n
 *   If the data is the same with the addr/port for requests of shmem_controller,
 *    this API will fail to avoid binding the same addr/port.
 */
int fpga_shmem_controller_set_ip_notice(
        unsigned short port,
        const char *addr);

/**
 * @brief API which get IP address for notification from shmem_controller
 * @param[out] port
 *   pointer variable to get TCP port
 * @param[out] addr
 *   pointer variable to get TCP ip address
 * @retval 0
 *   Success
 * @retval -INVALID_ARGUMENT
 *   e.g.) `addr` is null
 * @retval -FAILURE_MEMORY_ALLOC
 *   Failed to allocate memory
 *
 * @details
 *   Get TCP IP address and port which the thread created by fpga_shmem_controller_init()
 *    listen for receive the file_prefix which secondary process with the same file_prefix
 *    has been dead in error with.@n
 *   Please allocate memory for `*port` by user.@n
 *   `*addr` will be allocated by this API, so please explicitly free `*addr` by free().@n
 *   The value of `*port` and `*addr` are undefined when this API fails.
 */
int fpga_shmem_controller_get_ip_notice(
        unsigned short *port,
        char **addr);

/**
 * @brief Default Callback function of shmem_manager
 * @details
 *   This function will check if the file created by fpga_shmem_init() is exist or not
 *    and if it is possible to lock the file or not.@n
 *   When the lock file is exist and lock is available, the secondary process finish using shmem.@n
 *   After the success of locking the file, check the data of the lock file.@n
 *   If the lock file is not empty, the secondary process has called fpga_shmem_finish()
 *    and the process will not be able to use shmem since then.@n
 *   If the lock file is empty, the secondary process has not called fpga_shmem_finish()
 *    but the process is dead, so send the prefix to the thread created by fpga_shmem_controller_init()
 *    and do inf loop waiting for fpga_shmem_disable().
 */
int callback_function_notification_prefix(
        void* arg);

#ifdef __cplusplus
}
#endif

#endif  // LIBFPGA_INCLUDE_LIBSHMEM_CONTROLLER_H_
