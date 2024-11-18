/*************************************************
* Copyright 2024 NTT Corporation, FUJITSU LIMITED
* Licensed under the 3-Clause BSD License, see LICENSE for details.
* SPDX-License-Identifier: BSD-3-Clause
*************************************************/
/**
 * @file libfpgabs.h
 * @brief Header file for configuration FPGA
 */

#ifndef LIBFPGA_INCLUDE_LIBFPGABS_H_
#define LIBFPGA_INCLUDE_LIBFPGABS_H_

#include <libfpgactl.h>

#ifdef __cplusplus
extern "C" {
#endif


/**
 * MCAP executable binary file name
 */
#define LIBFPGABS_MCAP_EXEC_FILE          "mcap"

/**
 * Max length for searching direcotry's used in fpga_config_region()
 */
#define LIBFPGABS_RECONFIG_FILE_DIR_LEN   200

/**
 * Prefix for Tandem configuration file
 */
#define LIBFPGABS_RECONFIG_TANDEM_PREFIX  "tandem-"

/**
 * Suffix for configuration file
 */
#define LIBFPGABS_RECONFIG_SUFFIX         ".bit"


/**
 * @enum fpga_region_t
 * @brief FPGA's region id for user
 */
typedef enum FPGA_MODULE_REGION {
  FPGA_MODULE_REGION_ALL = 0, /**< All region */
  FPGA_MODULE_REGION_MAX,     /**< Sentinel */
} fpga_region_t;


/**
 * @brief API which acquire a lock for FPGA
 * @param[in] dev_id
 *   FPGA's device id got by fpga_dev_init()
 * @param[in] region
 *   target region to lock
 * @retval 0
 *   Success
 * @retval -INVALID_ARGUMENT
 *   e.g.) `dev_id` is invalid value
 * @retval -FAILURE_IOCTL
 *   e.g.) xpcie_device.h used in driver and lib differ
 * @retval -INVALID_OPERATION
 *   e.g.) FPGA is being written
 *
 * @details
 *   The driver uses reference_count per FPGA module per lane to show FPGA's status if using or not.@n
 *   It is possible to increment reference_count by this API,
 *    but if FPGA is just being written by fpga_config_region(),fpga_write_bitstream(),
 *    this API will fail.@n
 *   The reference_count can be incremented any number of times by a fd,
 *    and when the process dies without release the lock,
 *    the lock will not be released automatically.
 */
int fpga_refcount_region_acquire(
        uint32_t dev_id,
        fpga_region_t region);

/**
 * @brief API which release a lock for FPGA
 * @param[in] dev_id
 *   FPGA's device id got by fpga_dev_init()
 * @param[in] region
 *   target region to lock
 * @retval 0
 *   Success
 * @retval -INVALID_ARGUMENT
 *   e.g.) `dev_id` is invalid value
 * @retval -FAILURE_IOCTL
 *   e.g.) xpcie_device.h used in driver and lib differ
 * @retval -INVALID_OPERATION
 *   e.g.) FPGA is being written
 *
 * @details
 *   The driver uses reference_count per FPGA module per lane to show FPGA's status if using or not.@n
 *   It is possible to decrement reference_count by this API,
 *    but if FPGA is just being written by fpga_config_region(),fpga_write_bitstream(),
 *    or FPGA's reference_count is already 0, This API will fail.@n
 *   The reference_count can be control any number of times by a fd,
 *    so it is possible to decrement a lock which other process has locked.
 */
int fpga_refcount_region_release(
        uint32_t dev_id,
        fpga_region_t region);

/**
 * @brief API which cleanup a lock for FPGA
 * @param[in] dev_id
 *   FPGA's device id got by fpga_dev_init()
 * @param[in] region
 *   target region to lock
 * @retval 0
 *   Success
 * @retval -INVALID_ARGUMENT
 *   e.g.) `dev_id` is invalid value
 * @retval -FAILURE_IOCTL
 *   e.g.) xpcie_device.h used in driver and lib differ
 *
 * @details
 *   The driver uses reference_count per FPGA module per lane to show FPGA's status if using or not.@n
 *   It is possible to set reference_count with 0 by this API forcibly.
 *    Even if FPGA is just being written by fpga_config_region(),fpga_write_bitstream(),
 *    this API will succeed.@n
 *   The reference_count can be control any number of times by a fd,
 *    so it is possible to cleanup a lock which other process has locked.
 */
int fpga_refcount_region_cleanup(
        uint32_t dev_id,
        fpga_region_t region);

/**
 * @brief API which cleanup a lock for FPGA
 * @param[in] dev_id
 *   FPGA's device id got by fpga_dev_init()
 * @param[in] region
 *   target region to lock
 * @param[out] refcount
 *   pointer variable to get reference count
 * @retval 0
 *   Success
 * @retval -INVALID_ARGUMENT
 *   e.g.) `dev_id` is invalid value, `refcount` is null
 * @retval -FAILURE_IOCTL
 *   e.g.) xpcie_device.h used in driver and lib differ
 *
 * @details
 *   The driver uses reference_count per FPGA module per lane to show FPGA's status if using or not.@n
 *   It is possible to get reference_count by this API.
 */
int fpga_refcount_region_get(
        uint32_t dev_id,
        fpga_region_t region,
        int *refcount);

/**
 * @brief API which acquire a lock for FPGA's all region
 * @param[in] dev_id
 *   FPGA's device id got by fpga_dev_init()
 * @retval 0
 *   Success
 * @retval -INVALID_ARGUMENT
 *   e.g.) `dev_id` is invalid value
 * @retval -FAILURE_IOCTL
 *   e.g.) xpcie_device.h used in driver and lib differ
 * @retval -INVALID_OPERATION
 *   e.g.) FPGA is being written
 *
 * @details
 *   Call fpga_refcount_region_acquire() with FPGA_MODULE_REGION_ALL.
 * @sa fpga_refcount_region_acquire()
 */
int fpga_refcount_acquire(
        uint32_t dev_id);

/**
 * @brief API which release a lock for FPGA's all region
 * @param[in] dev_id
 *   FPGA's device id got by fpga_dev_init()
 * @retval 0
 *   Success
 * @retval -INVALID_ARGUMENT
 *   e.g.) `dev_id` is invalid value
 * @retval -FAILURE_IOCTL
 *   e.g.) xpcie_device.h used in driver and lib differ
 * @retval -INVALID_OPERATION
 *   e.g.) FPGA is being written
 *
 * @details
 *   Call fpga_refcount_region_release() with FPGA_MODULE_REGION_ALL.
 * @sa fpga_refcount_region_release()
 */
int fpga_refcount_release(
        uint32_t dev_id);

/**
 * @brief API which cleanup a lock for FPGA's all region
 * @param[in] dev_id
 *   FPGA's device id got by fpga_dev_init()
 * @retval 0
 *   Success
 * @retval -INVALID_ARGUMENT
 *   e.g.) `dev_id` is invalid value
 * @retval -FAILURE_IOCTL
 *   e.g.) xpcie_device.h used in driver and lib differ
 *
 * @details
 *   Call fpga_refcount_region_cleanup() with FPGA_MODULE_REGION_ALL.
 * @sa fpga_refcount_region_cleanup()
 */
int fpga_refcount_cleanup(
        uint32_t dev_id);
/**
 * @brief old name of fpga_refcount_acquire()
 * @sa fpga_refcount_acquire()
 */
int fpga_ref_acquire(
        uint32_t dev_id);

/**
 * @brief old name of fpga_refcount_release()
 * @sa fpga_refcount_release()
 */
int fpga_ref_release(
        uint32_t dev_id);

/**
 * @brief old name of fpga_refcount_cleanup()
 * @sa fpga_refcount_cleanup()
 */
int fpga_ref_cleanup(
        uint32_t dev_id);

/**
 * @brief API which configure FPGA by Tandem configuration
 * @param[in] dev_id
 *   FPGA's device id got by fpga_dev_init()
 * @param[in] lane
 *   (not used)any value is ok
 * @param[in] file_path
 *   Configuration file's path
 * @retval 0
 *   Success
 * @retval -INVALID_ARGUMENT
 *   e.g.) `dev_id` is invalid, `file_path` is null
 * @retval -INVALID_OPERATION
 *   e.g.) FPGA is being written or using
 * @retval -FAILURE_MEMORY_ALLOC
 *   Failed to allocate memory
 * @retval -LIBFPGA_FATAL_ERROR
 *   e.g.) API of libmcap failed
 * @retval -FAILURE_IOCTL
 *   e.g.) Maybe xpcie_device.h used in driver and lib differ
 *
 * @details
 *   Execute Tandem configuration at `dev_id`th FPGA by `file_path` configuration file.@n
 *   `file_path` can be used like open()(i.e. abs path or path from current directory).@n
 *   This API needs executable `mcap` tool on the env `PATH`,
 *    so please move `mcap` tool to the location in the `PATH`(e.g. /usr/local/bin, /usr/bin)
 *    or set the directory of mcap when execute App as follows:
~~~{.c}
$ sudo env PATH=$PATH:<directory of `mcap`> <App>
~~~
 *
 * @note
 *   It is recommended that fpga_config_region() be used instead of this API.
 */
int fpga_write_bitstream(
        uint32_t dev_id,
        uint32_t lane,
        const char *file_path);

/**
 * @brief API which configure FPGA
 * @param[in] dev_id
 *   FPGA's device id got by fpga_dev_init()
 * @param[in] file_body
 *   Target file's body(stripped directory, prefix, suffix from file_path)
 * @param[in] region
 *   Target region
 * @retval 0
 *   Success
 * @retval -INVALID_ARGUMENT
 *   e.g.) `dev_id` is invalid, `file_path` is null
 * @retval -INVALID_OPERATION
 *   e.g.) FPGA is being written or using
 * @retval -FAILURE_MEMORY_ALLOC
 *   Failed to allocate memory
 * @retval -LIBFPGA_FATAL_ERROR
 *   e.g.) API of libmcap failed
 * @retval -FAILURE_IOCTL
 *   e.g.) Maybe xpcie_device.h used in driver and lib differ
 *
 * @details
 *   Execute Tandem configuration at `dev_id`th FPGA by the configuration file path
 *    created by `file_body`.@n
 *   `file_body` is the string which is stripped directory name, prefix and suffix
 *    from the target file_path.@n
 *   The default searching direcotry is ""(i.e. current directory),
 *    and can be set by fpga_set_config_file_dir().@n
 *   The prefix and suffix are as follows:
 * @sa LIBFPGABS_RECONFIG_TANDEM_PREFIX
 * @sa LIBFPGABS_RECONFIG_SUFFIX@n
 *   This API needs executable `mcap` tool on the env `PATH`,
 *    so please move `mcap` tool to the location in the `PATH`(e.g. /usr/local/bin, /usr/bin)
 *    or set the directory of mcap when execute App as follows:
~~~{.c}
$ sudo env PATH=$PATH:<directory of `mcap`> <App>
~~~
 */
int fpga_config_region(
        uint32_t dev_id,
        const char *file_body,
        fpga_region_t region);

/**
 * @brief API which update searching directory for configuration file
 * @param[in] dir_path
 *   Target directory path
 * @retval 0
 *   Success
 * @retval -INVALID_ARGUMENT
 *   e.g.) `dir_path` is null, `dir_path` is too long
 *
 * @details
 *   Update the searching directory for configuration file.@n
 *   Only 1 searching path can be set at the same time.@n
 *   When the length of `dir_path` is 0, the directory will be current directory.@n
 *   When the last character of `dir_path` is not '/', setting path will be complemented,
 *    so the length of the path whose last character is not '/' can be less than 199
 *    and the other cab be less than 198.
 */
int fpga_set_config_file_dir(
        const char *dir_path);

/**
 * @brief API which update searching directory for configuration file
 * @param[out] dir_path
 *   pointer variable to get directory path
 * @retval 0
 *   Success
 * @retval -INVALID_ARGUMENT
 *   e.g.) `dir_path` is null
 * @retval -FAILURE_MEMORY_ALLOC
 *   Failed to allocate memory
 *
 * @details
 *   Get the searching directory for configuration file.@n
 *   The path will return with memory allocated by this library through `dir_path`,
 *    so please explicitly free it through free().@n
 *   The value of `*dir_path` is undefined when this API fails.
 */
int fpga_get_config_file_dir(
        char **dir_path);

/**
 * @brief API which update FPGA's configuration
 * @param[in] dev_id
 *   FPGA's device id got by fpga_dev_init()
 * @retval 0
 *   Success
 * @retval -INVALID_ARGUMENT
 *   e.g.) `dev_id` is invalid
 * @retval -FAILURE_IOCTL
 *   e.g.) Maybe xpcie_device.h used in driver and lib differ
 *
 * @details
 *   Update FPGA's information about FPGA's block address map and DMA channel num.
 */
int fpga_update_bitstream_info(
        uint32_t dev_id);

#ifdef __cplusplus
}
#endif

#endif  // LIBFPGA_INCLUDE_LIBFPGABS_H_
