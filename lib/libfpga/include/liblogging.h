/*************************************************
* Copyright 2024 NTT Corporation, FUJITSU LIMITED
* Licensed under the 3-Clause BSD License, see LICENSE for details.
* SPDX-License-Identifier: BSD-3-Clause
*************************************************/
/**
 * @file liblogging.h
 * @brief Header file for logger of libfpga
 */

#ifndef LIBFPGA_INCLUDE_LIBLOGGING_H_
#define LIBFPGA_INCLUDE_LIBLOGGING_H_

#ifdef __cplusplus
extern "C" {
#endif


#define LOGFILE "libfpga-log-"          /**< Prefix of output file */

// LogLevel Definition
#define LIBFPGA_LOG_NOTHING         0   /**< libfpga's loglevel : print nothing(used only in libfpga_log_set_level() as API) */
#define LIBFPGA_LOG_PRINT           1   /**< libfpga's loglevel : print stdout too */
#define LIBFPGA_LOG_ERROR           2   /**< libfpga's loglevel : print error level(default) */
#define LIBFPGA_LOG_WARN            3   /**< libfpga's loglevel : print warning level */
#define LIBFPGA_LOG_INFO            4   /**< libfpga's loglevel : print info level */
#define LIBFPGA_LOG_DEBUG           5   /**< libfpga's loglevel : print debug level */
#define LIBFPGA_LOG_ALL             10  /**< libfpga's loglevel : print all log */


// Error Definition
#define NOT_INITIALIZED             1   /**< General error : not initialized */
#define ALREADY_INITIALIZED         2   /**< General error : already initialized */
#define ALREADY_ASSIGNED            3   /**< General error : already assigned */
#define NO_DEVICES                  5   /**< General error : there is no valid device */
#define FULL_ELEMENT                35  /**< General error : list,table,etc... is full */

#define ENQUEUE_QUEFULL             11  /**< DMA error : DMA's queue is full */
#define DEQUEUE_TIMEOUT             12  /**< DMA error : dequeuing DMA's queue is timeout */
#define UNAVAILABLE_CHID            15  /**< DMA error : target channel is not implemented */
#define ALREADY_ACTIVE_CHID         16  /**< DMA error : target channel is already activated */
#define CONNECTOR_ID_MISMATCH       13  /**< DMA error : Not found matching connector_id */

#define FUNC_CHAIN_ID_MISMATCH      14  /**< FunctionChain error : Not found matching function channel id */
#define TABLE_UPDATE_TIMEOUT        17  /**< FunctionChain error : Failed to confirm successful completion of update */

#define CALLBACK_FUNCTION           19  /**< Shmem error : Finished by callback function */
#define MISMATCH_FILE_PREFIX        20  /**< Shmem error : Not found matching file_prefix */
#define NOT_REGISTERED_SIGNAL       23  /**< Shmem error : Signal is not set by specific API yet. */
#define CONNECTION_LOST             31  /**< Shmem error : connection lost */

#define INVALID_ARGUMENT            4   /**< Invalid operation : User's arguments are invalid */
#define INVALID_ADDRESS             10  /**< Invalid operation : User's address of memory is invalid(e.g. boundary) */
#define INVALID_DATA                29  /**< Invalid operation : User's data are invalid */
#define INVALID_PARAMETER           36  /**< Invalid operation : User's parameters are invalid */
#define INVALID_OPERATION           37  /**< Invalid operation : User's operations are invalid */

#define FAILURE_DEVICE_OPEN         7   /**< Failure system call or function : Opening device file of xpcie driver */
#define FAILURE_INITIALIZE          18  /**< Failure system call or function : Initializing */
#define FAILURE_ESTABLISH           26  /**< Failure system call or function : Establishing */
#define FAILURE_CONTROLLER          28  /**< Failure system call or function : ShmemController's process */

#define FAILURE_MEMORY_ALLOC        6   /**< Failure system call or function : malloc(),strdup() */
#define FAILURE_OPEN                32  /**< Failure system call or function : open() */
#define FAILURE_READ                33  /**< Failure system call or function : read() */
#define FAILURE_WRITE               34  /**< Failure system call or function : write() */
#define FAILURE_IOCTL               8   /**< Failure system call or function : ioctl() */
#define FAILURE_MMAP                9   /**< Failure system call or function : mmap() */
#define FAILURE_BIND                30  /**< Failure system call or function : bind() */
#define FAILURE_TRANSFER            27  /**< Failure system call or function : send(),recv() */
#define FAILURE_SEND_SIGNAL         21  /**< Failure system call or function : kill() */
#define FAILURE_REGISTER_SIGNAL     22  /**< Failure system call or function : sigaction() */
#define FAILURE_FORK                24  /**< Failure system call or function : fork() */
#define FAILURE_MKDIR               25  /**< Failure system call or function : mkdir() */

#define UNKNOWN_EXCEPTION           254 /**< Unknown reason error */
#define LIBFPGA_FATAL_ERROR         255 /**< Error need to stop process immediately */

#define LIBCHAIN                    "libchain:   " /**< Log's prefix for libchain* */
#define LIBDMA                      "libdma:     " /**< Log's prefix for libdma */
#define LIBLLDMA                    "liblldma:   " /**< Log's prefix for liblldma */
#define LIBFPGACTL                  "libfpgactl: " /**< Log's prefix for libfpgactl */
#define LIBSHMEM                    "libshmem:   " /**< Log's prefix for libshmem* */
#define LIBPTU                      "libptu:     " /**< Log's prefix for libptu */
#define LIBLOGGING                  "liblogging: " /**< Log's prefix for liblogging */
#define LIBFUNCTION                 "libfunc:    " /**< Log's prefix for libfunction* */
#define LIBDIRECTTRANS              "libdirect:  " /**< Log's prefix for libdirecttrans* */
#define LIBPOWER                    "libpower:   " /**< Log's prefix for libpower */
#define LIBTEMP                     "libtemp:    " /**< Log's prefix for libtemp */
#define LIBFPGABS                   "libfpgabs:  " /**< Log's prefix for libfpgabs */
#define LIBUNKNOWN                  "unknown:    " /**< Log's default prefix */


/**
 * Macro for the library name used in llf_***()@n
 * When you want to use these and want to change, do as follows:
~~~{.c}
#define FPGA_LOGGER_LIBNAME "What you want to set"
#include <liblogging.h>
~~~
 * OR
~~~{.c}
#include <liblogging.h>
#undef FPGA_LOGGER_LIBNAME
#define FPGA_LOGGER_LIBNAME "What you want to set"
~~~
 */
#ifndef FPGA_LOGGER_LIBNAME
#define FPGA_LOGGER_LIBNAME           LIBUNKNOWN
#endif  // FPGA_LOGGER_LIBNAME


/**
 * LogLibFpga: error level
 */
#define llf_err(err, fmt, arg...)   log_libfpga(LIBFPGA_LOG_ERROR, FPGA_LOGGER_LIBNAME "[%d]" fmt, -err, ##arg)

/**
 * LogLibFpga: warn level
 */
#define llf_warn(err, fmt, arg...)  log_libfpga(LIBFPGA_LOG_WARN,  FPGA_LOGGER_LIBNAME "[%d]" fmt, -err, ##arg)

/**
 * LogLibFpga: info level
 */
#define llf_info(fmt, arg...)       log_libfpga(LIBFPGA_LOG_INFO,  FPGA_LOGGER_LIBNAME fmt, ##arg)

/**
 * LogLibFpga: debug level
 */
#define llf_dbg(fmt, arg...)        log_libfpga(LIBFPGA_LOG_DEBUG, FPGA_LOGGER_LIBNAME fmt, ##arg)

/**
 * LogLibFpga: print level
 */
#define llf_pr(fmt, arg...)         log_libfpga(LIBFPGA_LOG_PRINT, "        " FPGA_LOGGER_LIBNAME fmt, ##arg)


/**
 * @brief API which set loglevel of output
 * @param[in] level
 *   Loglevel to set
 * @return void
 *
 * @details
 *   Set loglevel as `level`.@n
 *   When warning level is set, print level more critical than warning
 *    (i.e. LIBFPGA_LOG_PRINT/LIBFPGA_LOG_ERROR/LIBFPGA_LOG_WARN)@n
 *   Defalut value is LIBFPGA_LOG_ERROR.
 */
void libfpga_log_set_level(
        int level);

/**
 * @brief API which get loglevel of output
 * @param void
 * @return log level of libfpga output
 *
 * @details
 *   Get loglevel as `retval`.@n
 *   Defalut value is LIBFPGA_LOG_ERROR.
 */
int libfpga_log_get_level(void);

/**
 * @brief API which set log output only to stdout
 * @param void
 * @return void
 *
 * @details
 *   Print log only to stdout, not create logfile.
 */
void libfpga_log_set_output_stdout(void);

/**
 * @brief API which set log output to file
 * @param void
 * @return void
 *
 * @details
 *   Print log to file.@n
 *   Defalut status.
 */
void libfpga_log_quit_output_stdout(void);

/**
 * @brief API which get log output's status
 * @param void
 * @retval true(non-zero) : only to stdout
 * @retval false(zero) : to file
 *
 * @details
 *   Get status of log output.
 */
int libfpga_log_get_output_stdout(void);

/**
 * @brief API which enable timestamp
 * @param void
 * @return void
 *
 * @details
 *   Print timestamp to log.@n
 *   Defalut status.
 */
void libfpga_log_set_timestamp(void);

/**
 * @brief API which disable timestamp
 * @param void
 * @return void
 *
 * @details
 *   Print NO timestamp to log.
 */
void libfpga_log_quit_timestamp(void);

/**
 * @brief API which get timestamp's status
 * @param void
 * @retval true(non-zero) : enabled
 * @retval false(zero) : disabled
 *
 * @details
 *   Get status of timestamp.
 */
int libfpga_log_get_timestamp(void);

/**
 * @brief API which parse logger options
 * @param[in] argc : cmdline's argc
 * @param[in] argv : cmdline's argv
 * @return num of options which handled by this API
 * @retval -INVALID_ARGUMENT : Invalid option
 *
 * @details
 *   usage: `<APP> [-ptsf] [-l <level>]@n
 *   options :
 *   @li -l, --lib-loglevel      : set logLevel
 *   @li -p, --set-timestamp     : enable timestamP
 *   @li -t, --quit-timestamp    : disable Timestamp
 *   @li -s, --set-output-stdout : output:only Stdout(not create file)
 *   @li -f, --set-output-file   : output:logFile(create file)
 */
int libfpga_log_parse_args(
        int argc,
        char **argv);

/**
 * @brief API which set flag to close opening logfile and create a new file.
 * @param void
 * @return void
 *
 * @details
 *   This API will be used when forking process.@n
 *   If some process use the same fd opened before fork,
 *    the file may be overwritten, so remake logfile by using this API.
 */
void libfpga_log_reset_output_file(void);

/**
 * @brief API which print libfpga's log
 *        (Normaly user will not use this API.)
 * @param[in] level : setting loglevel
 * @param[in] format : See printf()
 * @param[in] ... : See printf()
 * @retval 0 : success
 * @retval -1 : failed
 *
 * @details
 *   print timestamp(if need), loglevel(if need) and `format` with args into libfpga's log output.
 * @par example
~~~{}
17:05:51[debug] libshmem:   fpga_shmem_init_sys(file_prefix((null)), huge_dir((null)), socket_limit(0), lcore_mask(0))
17:05:51[debug] libshmem:   __fpga_shmem_init_host_info()
17:05:51[info]  libshmem:    fpga_shmem_socket_num=2
17:05:51[info]  libshmem:    fpga_shmem_socket_limit[0]=8
17:05:51[info]  libshmem:    fpga_shmem_socket_limit[1]=8
17:05:51[info]  libshmem:    fpga_shmem_hugepage_limit=16
17:05:51[info]  libshmem:    fpga_shmem_lcore_num=112
17:05:51[debug] libshmem:   rte_eal_init(argc(4), argv[0](__fpga_shmem_init), argv[1](--proc-type), argv[2](primary), argv[3](--))
~~~
 */
int log_libfpga(
        int level,
        const char *format,
        ...);

/**
 * @brief API which print libfpga's log with printing command line arguments
 *        (Normaly user will not use this API.)
 * @param[in] level : setting loglevel
 * @param[in] argc : cmdline's argc
 * @param[in] argv : cmdline's argv
 * @param[in] format : See printf()
 * @param[in] ... : See printf()
 * @retval 0 : success
 * @retval -1 : failed
 *
 * @details
 *   print timestamp(if need), loglevel(if need), `argc`, `argv` and `format` with args into libfpga's log output.
 * @par example
~~~{}
17:05:51[debug] libshmem:   fpga_shmem_init_sys(file_prefix((null)), huge_dir((null)), socket_limit(0), lcore_mask(0))
17:05:51[debug] libshmem:   __fpga_shmem_init_host_info()
17:05:51[info]  libshmem:    fpga_shmem_socket_num=2
17:05:51[info]  libshmem:    fpga_shmem_socket_limit[0]=8
17:05:51[info]  libshmem:    fpga_shmem_socket_limit[1]=8
17:05:51[info]  libshmem:    fpga_shmem_hugepage_limit=16
17:05:51[info]  libshmem:    fpga_shmem_lcore_num=112
17:05:51[debug] libshmem:   rte_eal_init(argc(4), argv[0](__fpga_shmem_init), argv[1](--proc-type), argv[2](primary), argv[3](--))
~~~
 */
int log_libfpga_cmdline_arg(
        int level,
        int argc,
        char **argv,
        const char *format,
        ...);

#ifdef __cplusplus
}
#endif

#endif  // LIBFPGA_INCLUDE_LIBLOGGING_H_
