/*************************************************
* Copyright 2024 NTT Corporation, FUJITSU LIMITED
* Licensed under the GPL-2.0  License, see LICENSE for details.
* SPDX-License-Identifier: GPL-2.0-or-later
*************************************************/
/**
 * @file libxpcie.h
 * @brief Header file for common functions for FPGAs with LLDMA.@n
 *        There are functions which don't access register directly.
 */

#ifndef __LIBXPCIE_H__
#define __LIBXPCIE_H__

#include <linux/kernel.h>
#include <linux/version.h>
#include <linux/list.h>
#include <linux/pci.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/spinlock.h>
#include <asm/barrier.h>

#include "xpcie_device.h"

// General
#define XPCIE_DEVICE_NAME           "xpcie"   /**< Driver's device file name */
#define XPCIE_MAX_DEVICE_NUM        8         /**< Max Num of devices initialized by this driver */
#define PCI_VSEC_ID_XILINX          0x0001    /**< Xilinx's Vendor SpECific id */
#define PCI_BITSTREAM_ID_OFFSET     0xC       /**< Offset for Bitstream_id in PCI configuration register */

// CMS
#define SERIAL_ID_LEN               32        /**< Max string len of this device's serial id */

// Function chain
#define FPGA_UPDATE_POLLING_MAX     100       /**< Max Polling for checking function chain update */

// LLDMA
#define XPCIE_MAX_QUEUE_PAIR        32        /**< LLDMA channel num per 1 direction */
#define XPCIE_QUEUE_SIZE            255       /**< Command queue's depth */
#define FPGA_DRAIN_POLLING_MS       3000      /**< LLDMA channel's drain polling max time.3[s] */
#define FPGA_Q_STAT_FREE            0         /**< Command queue is not assgined */
#define FPGA_Q_STAT_USED            1         /**< Command queue is assgined */


// Log for this driver
#define xpcie_info(fmt, args...)  \
  pr_info("%s: " fmt "\n", XPCIE_DEVICE_NAME, ##args)
#define xpcie_notice(fmt, args...)  \
  pr_notice("%s: " fmt "\n", XPCIE_DEVICE_NAME, ##args)
#define xpcie_warn(fmt, args...)  \
  pr_warn("%s: " fmt "\n", XPCIE_DEVICE_NAME, ##args)
#define xpcie_alert(fmt, args...) \
  pr_alert("%s: " fmt "\n", XPCIE_DEVICE_NAME, ##args)
#define xpcie_err(fmt, args...)   \
  pr_err("%s: " fmt "\n", XPCIE_DEVICE_NAME, ##args)
#define xpcie_debug(fmt, args...)   \
  pr_debug("%s: " fmt "\n", XPCIE_DEVICE_NAME, ##args)

#ifdef XPCIE_TRACE_LOG
#define xpcie_trace(fmt, args...)  \
  pr_info("%s: " fmt "\n", XPCIE_DEVICE_NAME, ##args)
#else
#define xpcie_trace(...) do{}while(0)
#endif


/**
 * @struct fpga_queue_enqdeq_t
 * @brief Struct for management command queue
 */
typedef struct fpga_queue_enqdeq {
  uint32_t status;                              /**< FPGA_Q_STAT_FREE/FPGA_Q_STAT_USED */
  fpga_queue_t *que;                            /**< Command queue for user space */
  void    *qp_mem_addr;                         /**< Allocated address for command queue */
  uint64_t qp_mem_size;                         /**< Size of command queue for mmap */
  char     connector_id[CONNECTOR_ID_NAME_MAX]; /**< command queue's key */
} fpga_queue_enqdeq_t;

/**
 * @struct fpga_module_info_t
 * @brief Struct for Module's address and reference count information
 */
typedef struct fpga_module_info {
  uint64_t base;                        /**< base_address */
  uint32_t len;                         /**< length per a lane */
  uint32_t num;                         /**< lane num */
  int refcount[XPCIE_KERNEL_LANE_MAX];  /**< used or not per a lane */
} fpga_module_info_t;

/**
 * @struct fpga_modules_info_t
 * @brief Struct for Device's address and reference count information
 */
typedef struct fpga_modules_info {
  fpga_module_info_t global;        /**< Global module */
  fpga_module_info_t chain;         /**< Chain module */
  fpga_module_info_t direct;        /**< Direct module */
  fpga_module_info_t lldma;         /**< LLDMA module */
  fpga_module_info_t ptu;           /**< PTU module */
  fpga_module_info_t conv;          /**< Conversion module */
  fpga_module_info_t func;          /**< Function module */
  fpga_module_info_t cms;           /**< CMS Space */
  enum FPGA_CONTROL_TYPE ctrl_type; /**< Control type */
} fpga_modules_info_t;

/**
 * @struct extifid_cid_t
 */
typedef struct extifid_cid {
  int extif_id;
  int cid;
} extifid_cid_t;

/**
 * @struct fpga_dev_info_t
 * @brief Struct for FPGA's general information
 */
typedef struct fpga_dev_info {
  struct list_head list;      /**< FPGA list which this device is included */
  struct cdev cdev;           /**< Information as character device */

  uint32_t dev_id;            /**< Minor number */
  uint32_t status;            /**< Not used */
  uint32_t num_devs;          /**< Device num initialized by this driver */

  struct pci_dev *pci_dev;    /**< Information as PCI device */
  struct pci_dev *upstream;   /**< Information of upstream PCI device */

  uint8_t *base_addr;         /**< Base register address (for memory-mapped I/O) */
  uint64_t base_addr_len;     /**< Base register address length */
  phys_addr_t base_addr_hw;   /**< Base register address (hardware address) */

  spinlock_t lock;            /**< lock for exclusive register access */

  spinlock_t lock_refcount;   /**< lock for exclusive refcount access */
  int refcount;               /**< (Not used)reference count of this device */

  xpcie_fpga_bitstream_id_t bitstream_id;  /**< Bitstream of this device */

  struct mutex queue_mutex;   /**< mutex for command queue access */
  fpga_queue_enqdeq_t enqueues[XPCIE_MAX_QUEUE_PAIR]; /**< Command queue's status for DMA RX */
  fpga_queue_enqdeq_t dequeues[XPCIE_MAX_QUEUE_PAIR]; /**< Command queue's status for DMA TX */

  /** Function chain's status table with lane, fchid, cid(0:ingr/1:egr) as the index */
  extifid_cid_t fch_dev_table[XPCIE_KERNEL_LANE_MAX][XPCIE_FUNCTION_CHAIN_MAX][2];  //

  char card_name[FPGA_CARD_NAME_LEN]; /**< FPGA's card name got by CMS */
  char serial_id[SERIAL_ID_LEN];      /**< FPGA's serial id got by CMS */

  int available_dma_channel_num;      /**< the num of FPGA's DMA channel */

  fpga_modules_info_t mods;           /**< FPGA's module's address map */
} fpga_dev_info_t;

/**
 * @struct xpcie_file_private
 * @brief Struct for information binded with file descripter got by open()
 */
struct xpcie_file_private {
  fpga_dev_info_t *dev;   /**< Opening device */
  int chid;               /**< assigned chid for DMA */
  int que_kind;           /**< assigned queue dir for DMA */
  bool is_get_queue;      /**< true:get queue false:not get queue */
  bool is_valid_command;  /**< true:there were valid ioctl command/false: else */
  bool is_avail_rw;       /**< true:available to read()/write(), false:else */
};


// General
/**
 * @brief Function which initialize FPGA when insmod
 */
int xpcie_fpga_dev_init(
        fpga_dev_info_t *);

/**
 * @brief Function which finalize FPGA when rmmod
 */
void xpcie_fpga_dev_close(
        fpga_dev_info_t *);

/**
 * @brief Function which control refcount status of device region
 */
int xpcie_fpga_control_refcount(
        fpga_dev_info_t *dev,
        xpcie_refcount_cmd_t cmd,
        xpcie_region_t region,
        int *refcount);

/**
 * @brief Function which print driver build options
 */
void xpcie_fpga_print_build_options(void);

/**
 * @brief Function which get fops of xpcie driver
 */
struct file_operations *xpcie_fpga_get_cdev_fops(void);

/**
 * @brief Function which get device by minor number
 */
fpga_dev_info_t* xpcie_fpga_get_device_by_minor(
        uint32_t minor_num);

/**
 * @brief Function which get base address by minor number
 */
uint64_t xpcie_fpga_get_baseaddr(
        uint8_t minor_num);

/**
 * @brief Function which get control type
 */
int xpcie_fpga_get_control_type(
        fpga_dev_info_t *dev);

/**
 * @brief Function which set address map into user argument
 */
void xpcie_fpga_copy_base_address_for_user(
        fpga_dev_info_t *dev,
        fpga_address_map_t *map);

/**
 * @brief Common: Get module's address map as modulized FPGA
 */
int xpcie_fpga_common_get_module_info(
        fpga_dev_info_t *dev);


/**
 * @brief Function which Read register
 * @param[in] dev
 *   Target device
 * @param[in] offset
 *   Target register offset from the head of device
 * @return
 *   Read register value
 */
static inline uint32_t
reg_read32(
  fpga_dev_info_t *dev,
  uint32_t offset)
{
  uint32_t value;
  spin_lock(&dev->lock);
  value = *(const volatile uint32_t *)(dev->base_addr + offset);
  rmb();
  spin_unlock(&dev->lock);
#ifdef XPCIE_REGISTER_LOG
  xpcie_info("read32  : dev_id: %02d, offset: 0x%08x, value: 0x%08x", dev->dev_id, offset, value);
#endif
  return value;
}

/**
 * @brief (No available)Function which Read register(64bit)
 * @param[in] dev
 *   Target device
 * @param[in] offset
 *   Target register offset from the head of device
 * @return
 *   Read register value
 */
static inline uint64_t
reg_read64(
  fpga_dev_info_t *dev,
  uint32_t offset)
{
  uint32_t value;
  spin_lock(&dev->lock);
  value = *(const volatile uint64_t *)(dev->base_addr + offset);
  rmb();
  spin_unlock(&dev->lock);
#ifdef XPCIE_REGISTER_LOG
  xpcie_info("read64  : dev_id: %02d, offset: 0x%08x, value: 0x%08x", dev->dev_id, offset, value);
#endif
  return value;
}

/**
 * @brief (No available)Function which Read register
 * @param[in] dev
 *   Target device
 * @param[in] offset
 *   Target register offset from the head of device
 * @param[in] value
 *   Set value
 */
static inline void
reg_write32(
  fpga_dev_info_t *dev,
  uint32_t offset,
  uint32_t value)
{
#ifdef XPCIE_REGISTER_LOG
  xpcie_info("write32 : dev_id: %02d, offset: 0x%08x, value: 0x%08x", dev->dev_id, offset, value);
#endif
  wmb();
  spin_lock(&dev->lock);
  *(volatile uint32_t *)(dev->base_addr + offset) = value;
  spin_unlock(&dev->lock);
#ifdef XPCIE_REGISTER_LOG
#ifndef XPCIE_REGISTER_LOG_SUPPRESS_CHECK_REALLY_WRITE
  reg_read32(dev, offset);
#endif
#endif
}

/** Macro for Common Register Write */
#define __reg_write(mod, dev, offset, lane, value) \
  reg_write32((dev), (dev)->mods.mod.base + (dev)->mods.mod.len * (lane) + (offset), (value))
/** Macro for Common Register Read */
#define __reg_read(mod, dev, offset, lane) \
  reg_read32((dev), (dev)->mods.mod.base + (dev)->mods.mod.len * (lane) + (offset))

/** Macro for Chain module's Register Write */
#define chain_reg_write(dev, offset, lane, value)   __reg_write(chain, dev, offset, lane, value)
/** Macro for Chain module's Register Read */
#define chain_reg_read(dev, offset, lane)           __reg_read(chain, dev, offset, lane)

/** Macro for Direct module's Register Write */
#define direct_reg_write(dev, offset, lane, value)  __reg_write(direct, dev, offset, lane, value)
/** Macro for Direct module's Register Read */
#define direct_reg_read(dev, offset, lane)          __reg_read(direct, dev, offset, lane)

/** Macro for LLDMA module's Register Write */
#define lldma_reg_write(dev, offset, value)          __reg_write(lldma, dev, offset, 0, value)
/** Macro for LLDMA module's Register Read */
#define lldma_reg_read(dev, offset)                  __reg_read(lldma, dev, offset, 0)

/** Macro for CMS module's Register Write */
#define cms_reg_write(dev, offset, value)           __reg_write(cms, dev, offset, 0, value)
/** Macro for CMS module's Register Read */
#define cms_reg_read(dev, offset)                   __reg_read(cms, dev, offset, 0)

#endif  /* __LIBXPCIE_H__ */
