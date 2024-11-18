/*************************************************
* Copyright 2024 NTT Corporation, FUJITSU LIMITED
* Licensed under the 3-Clause BSD License, see LICENSE for details.
* SPDX-License-Identifier: BSD-3-Clause
*************************************************/
/**
 * @file libfpgactl.h
 * @brief Header file for Basic control FPGA APIs
 */

#ifndef LIBFPGA_INCLUDE_LIBFPGACTL_H_
#define LIBFPGA_INCLUDE_LIBFPGACTL_H_

#include <stdlib.h>

#include <xpcie_device.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * This library's name : Normaly defined by Makefile
 */
#ifndef LIBRARY_NAME
#define LIBRARY_NAME                  "LIBFPGA"
#endif

/**
 * (TBD)This library's type : Normaly defined by Makefile
 */
#ifndef LIBRARY_TYPE
#define LIBRARY_TYPE                  0xFF
#endif

/**
 * This library's Major version : Normaly defined by Makefile
 */
#ifndef LIBRARY_VERSION1
#define LIBRARY_VERSION1              0xFF
#endif

/**
 * This library's Minor version : Normaly defined by Makefile
 */
#ifndef LIBRARY_VERSION2
#define LIBRARY_VERSION2              0xFF
#endif

/**
 * This library's revision : Normaly defined by Makefile
 */
#ifndef LIBRARY_REVISION_H
#define LIBRARY_REVISION_H            0xFF
#endif

/**
 * This library's patch : Normaly defined by Makefile
 */
#ifndef LIBRARY_REVISION_L
#define LIBRARY_REVISION_L            0xFF
#endif

/**
 * Max num which this library can manage
 */
#define FPGA_MAX_DEVICES              16

/**
 * Directory name for xpcie driver's device file
 */
#define FPGA_DEVICE_DIR               "/dev/"

/**
 * Prefix for xpcie driver's device file
 */
#ifndef FPGA_UNUSE_SERIAL_ID
#define FPGA_DEVICE_PREFIX            "/dev/xpcie_"
#else
#define FPGA_DEVICE_PREFIX            "/dev/xpcie"
#endif

/**
 * Max length for various file names in this library
 */
#define FPGA_FILE_PATH_MAX            255

/**
 * Max length for vendor name as PCI device
 */
#define FPGA_VENDOR_NAME_LEN          64

/**
 * Default Name of config file for fpga_get_device_config()
 */
#define FPGA_CONFIG_JSON_PATH         "bitstream_id-config-table.json"


// Definition for FPGA Bitstream
#define FPGA_MAJOR_VERSION_TYPE(major)    ((major) & 0x000000FF)
#define FPGA_MAJOR_VERSION_INVEXTIF       0x00000011  /**< Invert external interface ID */
#define FPGA_MAJOR_VERSION_WOPTU          0x0000001C  /**< WithoutPTU+WithDirect */
#define FPGA_MAJOR_VERSION_DEFAULT        0x00000014  /**< WithPTU+WithDirect */
#define FPGA_MAJOR_VERSION_WOPTU_WODIRECT 0x000000DC  /**< WithoutPTU+WithoutDirect */

/**
 * Macro to get the num of target device's ptu module
 * @param[in] dev
 *   fpga_device_t *dev
 */
#define KERNEL_NUM_PTU(dev)           ((dev)->map.ptu.num)

/**
 * Macro to get the num of target device's func module
 * @param[in] dev
 *   fpga_device_t *dev
 */
#define KERNEL_NUM_FUNC(dev)          ((dev)->map.func.num)

/**
 * Macro to get the num of target device's chain module
 * @param[in] dev
 *   fpga_device_t *dev
 */
#define KERNEL_NUM_CHAIN(dev)         ((dev)->map.chain.num)

/**
 * Macro to get the num of target device's direct module
 * @param[in] dev
 *   fpga_device_t *dev
 */
#define KERNEL_NUM_DIRECT(dev)        ((dev)->map.direct.num)

/**
 * Macro to get the num of target device's conv module
 * @param[in] dev
 *   fpga_device_t *dev
 */
#define KERNEL_NUM_CONV(dev)          ((dev)->map.conv.num)

/**
 * Will be depricated: Macro to get the num of target device's framework kernel
 * @param[in] dev
 *   fpga_device_t *dev
 */
#define KERNEL_NUM_FRAME(dev)         (KERNEL_NUM_CHAIN((dev)))

/**
 * Will be depricated: Macro to get the num of target device's framework_sub kernel
 * @param[in] dev
 *   fpga_device_t *dev
 */
#define KERNEL_NUM_SUB(dev)           (KERNEL_NUM_CONV((dev)))

/**
 * Will be depricated: Macro to get the largest num of target device's kernels
 */
#define LIBFPGA_KERNEL_MAX(dev, kernel_type) \
            ((kernel_type) == PTU   ? KERNEL_NUM_PTU((dev))\
          : ((kernel_type) == FUNC  ? KERNEL_NUM_FUNC((dev))\
          : ((kernel_type) == FRAME ? KERNEL_NUM_FRAME((dev))\
                                    : KERNEL_NUM_SUB((dev)))))

/**
 * Will be depricated: Macro to get the num of target device's kernel
 */
#define LIBFPGA_KERNEL_MAX_ALL        4


/**
 * @struct fpga_device_t
 * @brief FPGA device information for management sturct
 * @var fpga_device_t::name
 *      FPGA's serial_id
 * @var fpga_device_t::fd
 *      FPGA's file descriptor got by open()
 * @var fpga_device_t::dev_id
 *      FPGA's minor number in driver
 * @var fpga_device_t::status
 *      Not used.
 * @var fpga_device_t::task_id
 *      Not used.
 * @var fpga_device_t::info
 *      FPGA's pci device inforamtion
 * @var fpga_device_t::map
 *      FPGA's module offset and module num
 */
typedef struct fpga_device {
  char *name;
  int fd;
  uint32_t dev_id;
  uint32_t status;
  uint16_t task_id;
  fpga_card_info_t info;
  fpga_address_map_t map;
} fpga_device_t;

/**
 * @struct fpga_bs_t
 * @brief FPGA's bitstream information struct
 * @var fpga_bs_t::parent
 *      parent bitstream-id
 * @var fpga_bs_t::child
 *      child bitstream-id
 */
typedef struct fpga_bs {
  uint32_t parent;
  uint32_t child;
} fpga_bs_t;

/**
 * @struct pcie_bus_t
 * @brief FPGA's PCIe bass information struct
 * @var pcie_bus_t::domain
 *      PCIe bus domain
 * @var pcie_bus_t::bus
 *      PCIe bus bus number
 * @var pcie_bus_t::device
 *      PCIe bus device number
 * @var pcie_bus_t::function
 *      PCIe bus function number
 */
typedef struct pcie_bus {
  int domain;
  int bus;
  int device;
  int function;
} pcie_bus_t;

/**
 * @struct fpga_device_user_info_t
 * @brief FPGA device information for user sturct
 * @var fpga_device_user_info_t::device_file_path
 *      Full path of device file
 * @var fpga_device_user_info_t::tabussk_id
 *      FPGA's vendor name got by libpci
 * @var fpga_device_user_info_t::device_type
 *      FPGA's card name(e.g. Alveo U250)
 * @var fpga_device_user_info_t::device_index
 *      FPGA's minor number in driver
 * @var fpga_device_user_info_t::pcie_bus
 *      PCIe bus information
 * @var fpga_device_user_info_t::bitstream_id
 *      FPGA's bitstream_id
 */
typedef struct fpga_device_user_info {
  char device_file_path[FPGA_FILE_PATH_MAX];
  char vendor[FPGA_VENDOR_NAME_LEN];
  char device_type[FPGA_CARD_NAME_LEN];
  int  device_index;
  pcie_bus_t  pcie_bus;
  fpga_bs_t  bitstream_id;
} fpga_device_user_info_t;


/**
 * @brief API which initialize FPGA
 * @param[in] name
 *   FPGA's file_path or serial_id
 * @param[out] dev_id
 *   pointer variable to get dev_id
 * @retval 0
 *   Success
 * @retval -INVALID_ARGUMENT
 *   `name` is null, `dev_id` is null
 * @retval -FAILURE_DEVICE_OPEN
 *   e.g.) Devie file may not exist
 * @retval -FAILURE_MEMORY_ALLOC
 *   Failed to allocate memory
 * @retval -FULL_ELEMENT
 *   This library can manage less than FPGA_MAX_DEVICES
 * @retval -FAILURE_IOCTL
 *   e.g.) xpcie_device.h used in driver and lib differ
 * @retval -NOT_INITIALIZED
 *   e.g.) FPGA's bitstream may be broken
 *
 * @details
 *   Open device file by `name`, and validate FPGA's information by ioctls.
 *    When FPGAs are valid, set data into management device list, and return `dev_id`.
 *   To check validation, this API uses fpga_update_info().@n
 *   When FPGA whose name is `name` is already initialized, return 0 and print log as warning level.@n
 *   `*dev_id` will be not allocated by this API, so allocate by user.@n
 *   The value of `*dev_id` is undefined when this API fails.
 */
int fpga_dev_init(
        const char *name,
        uint32_t *dev_id);

/**
 * @brief API which initialize FPGA without dev_id
 * @param[in] name
 *   FPGA's file_path or serial_id
 * @retval 0
 *   Success
 * @retval -INVALID_ARGUMENT
 *   `name` is null
 * @retval -FAILURE_DEVICE_OPEN
 *   e.g.) Devie file may not exist
 * @retval -FAILURE_MEMORY_ALLOC
 *   Failed to allocate memory
 * @retval -FULL_ELEMENT
 *   This library can manage less than FPGA_MAX_DEVICES
 * @retval -FAILURE_IOCTL
 *   e.g.) xpcie_device.h used in driver and lib differ
 * @retval -NOT_INITIALIZED
 *   e.g.) FPGA's bitstream may be broken
 *
 * @details
 *   Execute fpga_dev_init() without returning `dev_id`
 * @sa fpga_dev_init()
 */
int fpga_dev_simple_init(
        const char *name);

/**
 * @brief API which initialize FPGA by command line arguments
 * @param[in] argc
 *   command line's argument
 * @param[in] argv
 *   command line's argument
 * @return num of options which handled by this API
 * @retval -ALREADY_INITIALIZED
 *   FPGA is already initialized
 * @retval -FAILURE_MEMORY_ALLOC
 *   Failed to allocate memory
 * @retval -INVALID_ARGUMENT
 *   Options are invalid
 * @retval -NO_DEVICES
 *   There are no FPGAs opened.
 *
 * @details
 *   usage: `<APP> -d /dev/xpcie_<serial_id>,/dev/xpcie_<serial_id>`@n
 *   usage: `<APP> -d /dev/xpcie_<serial_id>,<serial_id>`@n
 *   usage: `<APP> -d <serial_id>,<serial_id>`@n
 *   options :
 * - -d, --device : Specify FPGA device file path or serial_id.
 *                  When initializing several FPGAs by this option,
 *                  use only once this and <serial_id>'s delimiter should be comma','.
 *                  (i.e. `<APP> -d <serial_id> -d <serial_id>` is invalid)
 * .
 * @details
 *   Call fpga_dev_simple_init() by name parsed by `-d` option.@n
 *   This API will succeed only when No FPGAs are opened by this library's APIs(fpga_get_num()==0).@n
 *   When you try to use argc and argv for other APIs after using this API,
 *    you should skip this APIs options by using return value ret.(e.g. `argc-=ret`, `argv+=ret`)@n
 *   This API will do best effort to open all devices selected by `-d` option,
 *    but if it failed to open some of them, return 0 because the others are succeeded to be opened.
 */
int fpga_init(
        int argc,
        char **argv);

/**
 * @brief API which get dev_id from name
 * @param[in] name
 *   FPGA's file_path or serial_id
 * @param[out] dev_id
 *   pointer variable to get dev_id
 * @retval 0
 *   Success
 * @retval -INVALID_ARGUMENT
 *   `name` is null, `dev_id` is null
 * @retval -NOT_INITIALIZED
 *   e.g.) mathcing FPGA is not initialized by initialize API
 *
 * @details
 *   Get `dev_id` from management device list and return it with using `name` for matching.@n
 *   `*dev_id` will be not allocated by this API, so allocate by user.@n
 *   The value of `*dev_id` is undefined when this API fails.
 */
int fpga_get_dev_id(
        const char *name,
        uint32_t *dev_id);

/**
 * @brief API which get num of opening FPGAs
 * @param
 *   void
 * @return num of opening FPGAs
 * @details
 *   Get `dev_id` from management device list and return it.
 */
int fpga_get_num(void);

/**
 * @brief API which update FPGA's information
 * @param[in] dev_id
 *   FPGA's device id got by fpga_dev_init()
 * @retval 0
 *   Success
 * @retval -INVALID_ARGUMENT
 *   `dev_id` is invalid value
 * @retval -FAILURE_IOCTL
 *   e.g.) Maybe xpcie_device.h used in driver and lib differ
 * @retval -NOT_INITIALIZED
 *   e.g.) FPGA's bitstream may be broken
 *
 * @details
 *   Get FPGA's information as PCI device and config information(block register map)
 *    and print some of the information.
 */
int fpga_update_info(
        uint32_t dev_id);

/**
 * @brief API which scan all FPGAs on the host
 * @param void
 * @retval (>=0)
 *   num of scanned FPGAs
 * @retval -FAILURE_OPEN
 *   e.g.) Failed to open the directory(`FPGA_DEVICE_DIR`)
 * @retval -LIBFPGA_FATAL_ERROR
 *   The num scanned and opening FPGAs may differ.
 *
 * @details
 *   Scan All device file in the direcotry(`FPGA_DEVICE_DIR`)
 *    named as (`FPGA_DEVICE_PREFIX`<serial_id>).@n
 *   When FPGAs not initialized are scanned, initialize them,
 *    and when FPGAs already initialized are scanned, call fpga_update_info().
 * @sa fpga_dev_simple_init()
 * @sa fpga_update_info()
 */
int fpga_scan_devices(void);

/**
 * @brief API which finalize FPGA
 * @param[in] dev_id
 *   FPGA's device id got by fpga_dev_init()
 * @retval 0
 *   Success
 * @retval -INVALID_ARGUMENT
 *   `dev_id` is invalid value
 *
 * @details
 *   Free and clear memory for the target in the management device list and close FPGA.@n
 *   This API does not call other finalize APIs for its each modules.
 */
int fpga_dev_finish(
        uint32_t dev_id);

/**
 * @brief API which finalize all opening FPGAs
 * @param void
 * @retval 0
 *   Success
 * @retval -NOT_INITIALIZED
 *   No FPGAs are opening
 *
 * @details
 *   Call fpga_dev_finish() for all dev_id
 * @sa fpga_dev_finish()
 */
int fpga_finish(void);

/**
 * @brief API which get name of all opening FPGAs
 * @param[out] device_list
 *   pointer variable to get all opening device name as list
 * @retval 0
 *   Success
 * @retval -INVALID_ARGUMENT
 *   `device_list` is null
 * @retval -NOT_INITIALIZED
 *   FPGA is not opening by initialize API
 * @retval -FAILURE_MEMORY_ALLOC
 *   Failed to allocate memory
 *
 * @details
 *   Get a list of all FPGAs' name initialized by this library's API.@n
 *   The list will return with memory allocated by this library through `device_list`,
 *    so please explicitly free through fpga_release_device_list().@n
 *   The value of `*device_list` is undefined when this API fails.@n
 *   If the num of the list's elements and opening devices differ,
 *    print log("Fatal error:"), but there are no problems to control FPGA, so return 0.
 */
int fpga_get_device_list(
        char ***device_list);

/**
 * @brief API which free device name's list
 * @param[in] device_list
 *   list got by fpga_get_device_list()
 * @retval 0
 *   Success
 * @retval -INVALID_ARGUMENT
 *   `device_list` is null
 *
 * @details
 *   Free memory allocated by fpga_get_device_list().
 */
int fpga_release_device_list(
        char **device_list);

/**
 * @brief API which get device information shaped for user
 * @param[in] dev_id
 *   FPGA's device id got by fpga_dev_init()
 * @param[out] info
 *   pointer variable to get FPGA's information
 * @retval 0
 *   Success
 * @retval -INVALID_ARGUMENT
 *   `dev_id` is invalid, `info` is null
 *
 * @details
 *   Get FPGA's information by shaping data got by fpga_update_info() for user.@n
 *   `*info` will be not allocated by this API, so allocate by user.@n
 *   The value of `*info` is undefined when this API fails.
 * @remarks
 *   verified by libpci:3.7.0-6
 */
int fpga_get_device_info(
        uint32_t dev_id,
        fpga_device_user_info_t *info);

/**
 * @brief API which get FPGA's configuration information
 * @param[in] name
 *   FPGA's file_path or serial_id
 * @param[out] config_json
 *   pointer variable to get config json text
 * @retval 0
 *   Success
 * @retval -INVALID_ARGUMENT
 *   `name` is null, `config_json` is null
 * @retval -NOT_INITIALIZED
 *   e.g.) mathcing FPGA is not initialized by initialize API
 * @retval -FAILURE_OPEN
 *   Failed to open a configuration file
 * @retval -FAILURE_MEMORY_ALLOC
 *   Failed to allocate memory
 * @retval -FAILURE_READ
 *   Failed to read a configuration file
 * @return (see also __fpga_json_get_device_config())
 *
 * @details
 *   Get configuration information of FPGA from configuration file with matching bitstream_id.@n
 *   The json text will return with memory allocated by this library through `config_json`,
 *    so please explicitly free through fpga_release_device_config().@n
 *   The value of `*config_json` is undefined when this API fails.
 */
int fpga_get_device_config(
        const char *name,
        char **config_json);

/**
 * @brief API which get FPGA's configuration information
 * @param[in] config_json
 *   config json text got by fpga_get_device_config()
 * @retval 0
 *   Success
 * @retval -INVALID_ARGUMENT
 *   `config_json` is null
 *
 * @details
 *   Free memory allocated by fpga_get_device_config().
 */
int fpga_release_device_config(
        char *config_json);

/**
 * @brief API which set FPGA's configuration file path
 * @param[in] file_path
 *   FPGA's configuration file path
 * @retval 0
 *   Success
 * @retval -INVALID_ARGUMENT
 *   `file_path` is null, `file_path` >= `FPGA_FILE_PATH_MAX`
 *
 * @details
 *   Set FPGA's configuration file path used by fpga_get_device_config().@n
 *   Default value is `FPGA_CONFIG_JSON_PATH`.
 */
int fpga_set_device_config_path(
        const char *file_path);

/**
 * @brief API which get FPGA's configuration file path
 * @param[out] file_path
 *   pointer variable to get FPGA's configuration file path
 * @retval 0
 *   Success
 * @retval -INVALID_ARGUMENT
 *   `file_path` is null
 * @retval -FAILURE_MEMORY_ALLOC
 *   Failed to allocate memory
 *
 * @details
 *   Get FPGA's configuration file path used by fpga_get_device_config().@n
 *   The path will return with memory allocated by this library through `file_path`,
 *    so please explicitly free `*file_path`.@n
 *   The value of `*file_path` is undefined when this API fails.
 */
int fpga_get_device_config_path(
        char **file_path);

/**
 * @brief API which reset FPGA soft
 * @param[in] dev_id
 *   FPGA's device id got by fpga_dev_init()
 * @retval 0
 *   Success
 * @retval -INVALID_ARGUMENT
 *   `dev_id` is invalid value
 * @retval -FAILURE_IOCTL
 *   e.g.) xpcie_device.h used in driver and lib differ
 *
 * @details
 *   Execute soft_reset for target FPGA.
 */
int fpga_soft_reset(
        uint32_t dev_id);

/**
 * @brief API which enable read()/write()
 * @param[in] dev_id
 *   FPGA's device id got by fpga_dev_init()
 * @retval 0
 *   Success
 * @retval -INVALID_ARGUMENT
 *   `dev_id` is invalid value
 * @retval -FAILURE_IOCTL
 *   e.g.) xpcie_device.h used in driver and lib differ
 *
 * @details
 *   Enable to read(),write(),pread(),pwrite() for FPGA's device file.@n
 *   To prevent accessing register from a invalid process,
 *    the driver has a lock for read(),write() system calls() in defalut per file descripter.@n
 *   This API can control the lock, and after this calling this API,
 *    some of the library APIs(e.g. libfunction_filter_resize) can use pread()/pwrite()
 *    to access register directly.
 */
int fpga_enable_regrw(
        uint32_t dev_id);

/**
 * @brief API which disable read()/write()
 * @param[in] dev_id
 *   FPGA's device id got by fpga_dev_init()
 * @retval 0
 *   Success
 * @retval -INVALID_ARGUMENT
 *   `dev_id` is invalid value
 * @retval -FAILURE_IOCTL
 *   e.g.) xpcie_device.h used in driver and lib differ
 *
 * @details
 *   Disable to read(),write(),pread(),pwrite() for FPGA's device file.@n
 *   Normaly, this API will be not called.
 * @sa fpga_enable_regrw()
 */
int fpga_disable_regrw(
        uint32_t dev_id);

/**
 * @brief API which enable read()/write() for all opening FPGAs
 * @param void
 * @retval 0
 *   Success
 * @retval -INVALID_ARGUMENT
 *   `dev_id` is invalid value
 * @retval -FAILURE_IOCTL
 *   e.g.) xpcie_device.h used in driver and lib differ
 *
 * @details
 *   Enable to read(),write(),pread(),pwrite() for all opening FPGAs' device file.@n
 *   Call fpga_enable_regrw() with all `dev_id`.
 * @sa fpga_enable_regrw()
 */
int fpga_enable_regrw_all(void);

/**
 * @brief API which disable read()/write() for all opening FPGAs
 * @param void
 * @retval 0
 *   Success
 * @retval -INVALID_ARGUMENT
 *   `dev_id` is invalid value
 * @retval -FAILURE_IOCTL
 *   e.g.) xpcie_device.h used in driver and lib differ
 *
 * @details
 *   Disable to read(),write(),pread(),pwrite() for all opening FPGAs' device file.@n
 *   Call fpga_disable_regrw() with all `dev_id`.@n
 *   Normaly, this API will be not called.
 * @sa fpga_disable_regrw()
 */
int fpga_disable_regrw_all(void);

/**
 * @brief Function which get FPGA device management information
 * @details
 *   In future, fpga_device_t will be typedefed with void* type, as is often the case in common libraries. 
 */
fpga_device_t *fpga_get_device(
        uint32_t dev_id);

#ifdef __cplusplus
}
#endif

#endif  // LIBFPGA_INCLUDE_LIBFPGACTL_H_
