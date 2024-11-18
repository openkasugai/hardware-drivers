/*************************************************
* Copyright 2024 NTT Corporation, FUJITSU LIMITED
* Licensed under the GPL-2.0  License, see LICENSE for details.
* SPDX-License-Identifier: GPL-2.0-or-later
*************************************************/

#include <linux/delay.h>
#include <asm/mwait.h>

#include <libxpcie.h>

#if defined(ENABLE_MODULE_GLOBAL)
#include "global/libxpcie_global.h"
#endif

#if defined(ENABLE_MODULE_CHAIN)
#include "chain/libxpcie_chain.h"
#endif

#if defined(ENABLE_MODULE_DIRECT)
#include "direct/libxpcie_direct.h"
#endif

#if defined(ENABLE_MODULE_LLDMA)
#include "lldma/libxpcie_lldma.h"
#endif

#if defined(ENABLE_MODULE_PTU)
#include "ptu/libxpcie_ptu.h"
#endif

#if defined(ENABLE_MODULE_CONV)
#include "conv/libxpcie_conv.h"
#endif

#if defined(ENABLE_MODULE_FUNC)
#include "func/libxpcie_func.h"
#endif

#if defined(ENABLE_MODULE_CMS)
#include "cms/libxpcie_cms.h"
#endif


void
xpcie_fpga_print_build_options(void)
{
  xpcie_info(" Options="
#if defined(ENABLE_MEM_MANAGE)
    "ENABLE_MEM_MANAGE;"
#endif
#if defined(ENABLE_MODULE_GLOBAL)
    "ENABLE_MODULE_GLOBAL;"
#endif
#if defined(ENABLE_MODULE_CHAIN)
    "ENABLE_MODULE_CHAIN;"
#endif
#if defined(ENABLE_MODULE_DIRECT)
    "ENABLE_MODULE_DIRECT;"
#endif
#if defined(ENABLE_MODULE_LLDMA)
    "ENABLE_MODULE_LLDMA;"
#endif
#if defined(ENABLE_MODULE_PTU)
    "ENABLE_MODULE_PTU;"
#endif
#if defined(ENABLE_MODULE_CONV)
    "ENABLE_MODULE_CONV;"
#endif
#if defined(ENABLE_MODULE_FUNC)
    "ENABLE_MODULE_FUNC;"
#endif
#if defined(ENABLE_MODULE_CMS)
    "ENABLE_MODULE_CMS;"
#endif
#if defined(ENABLE_REFCOUNT_GLOBAL)
    "ENABLE_REFCOUNT_GLOBAL;"
#endif
#if defined(ENABLE_REFCOUNT_CHAIN)
    "ENABLE_REFCOUNT_CHAIN;"
#endif
#if defined(ENABLE_REFCOUNT_DIRECT)
    "ENABLE_REFCOUNT_DIRECT;"
#endif
#if defined(ENABLE_REFCOUNT_LLDMA)
    "ENABLE_REFCOUNT_LLDMA;"
#endif
#if defined(ENABLE_REFCOUNT_PTU)
    "ENABLE_REFCOUNT_PTU;"
#endif
#if defined(ENABLE_REFCOUNT_CONV)
    "ENABLE_REFCOUNT_CONV;"
#endif
#if defined(ENABLE_REFCOUNT_FUNC)
    "ENABLE_REFCOUNT_FUNC;"
#endif
#if defined(ENABLE_REFCOUNT_CMS)
    "ENABLE_REFCOUNT_CMS;"
#endif
#if defined(ENABLE_SETTING_IN_DRIVER)
    "ENABLE_SETTING_IN_DRIVER;"
#endif
#if defined(EXTIFINV)
    "EXTIFINV;"
#endif
#if defined(XPCIE_TRACE_LOG)
    "XPCIE_TRACE_LOG;"
#endif
#if defined(XPCIE_REGISTER_LOG)
    "XPCIE_REGISTER_LOG;"
#endif
#if defined(XPCIE_UNUSE_SERIAL_ID)
    "XPCIE_UNUSE_SERIAL_ID;"
#endif
#if defined(XPCIE_REGISTER_NO_LOCK)
    "XPCIE_REGISTER_NO_LOCK;"
#endif
#if defined(XPCIE_REGISTER_LOG_SUPPRESS_CHECK_REALLY_WRITE)
    "XPCIE_REGISTER_LOG_SUPPRESS_CHECK_REALLY_WRITE;"
#endif
  );
}

/**
 * @params[in] info : fpga_modules_info_t
 * @params[in] name : const char*
 */
#define xpcie_fpga_common_print_module_info(info, name) \
  xpcie_trace(" * [%-6s] : base(%#010llx),length(%#010x/1lane),lane_num(%d)",\
    (name), (info).base, (info).len, (info).num)

/**
 * @params[in] name : const char*
 */
#define xpcie_fpga_common_print_error_module_base_address(name) \
  xpcie_trace(" * [%-6s] : Failed to get module base address", (name))


int
xpcie_fpga_common_get_module_info(
  fpga_dev_info_t *dev)
{
  xpcie_trace("%s: dev_id(%u)", __func__, dev->dev_id);

#if defined(ENABLE_MODULE_GLOBAL)
  if (xpcie_fpga_common_get_global_module_info(dev)) {
    xpcie_fpga_common_print_error_module_base_address("GLOBAL");
    goto failed;
  }
  xpcie_fpga_common_print_module_info(dev->mods.global, "GLOBAL");
#endif

#if defined(ENABLE_MODULE_CHAIN)
  if (xpcie_fpga_common_get_chain_module_info(dev)) {
    xpcie_fpga_common_print_error_module_base_address("CHAIN");
    goto failed;
  }
  xpcie_fpga_common_print_module_info(dev->mods.chain, "CHAIN");
#endif

#if defined(ENABLE_MODULE_DIRECT)
  if (xpcie_fpga_common_get_direct_module_info(dev)) {
    xpcie_fpga_common_print_error_module_base_address("DIRECT");
    goto failed;
  }
  xpcie_fpga_common_print_module_info(dev->mods.direct, "DIRECT");
#endif

#if defined(ENABLE_MODULE_LLDMA)
  if (xpcie_fpga_common_get_lldma_module_info(dev)) {
    xpcie_fpga_common_print_error_module_base_address("LLDMA");
    goto failed;
  }
  xpcie_fpga_common_print_module_info(dev->mods.lldma, "LLDMA");
#endif

#if defined(ENABLE_MODULE_PTU)
  if (xpcie_fpga_common_get_ptu_module_info(dev)) {
    xpcie_fpga_common_print_error_module_base_address("PTU");
    goto failed;
  }
  xpcie_fpga_common_print_module_info(dev->mods.ptu, "PTU");
#endif

#if defined(ENABLE_MODULE_CONV)
  if (xpcie_fpga_common_get_conv_module_info(dev)) {
    xpcie_fpga_common_print_error_module_base_address("CONV");
    goto failed;
  }
  xpcie_fpga_common_print_module_info(dev->mods.conv, "CONV");
#endif

#if defined(ENABLE_MODULE_FUNC)
  if (xpcie_fpga_common_get_func_module_info(dev)) {
    xpcie_fpga_common_print_error_module_base_address("FUNC");
    goto failed;
  }
  xpcie_fpga_common_print_module_info(dev->mods.func, "FUNC");
#endif

#if defined(ENABLE_MODULE_CMS)
  if (xpcie_fpga_common_get_cms_module_info(dev)) {
    xpcie_fpga_common_print_error_module_base_address("CMS");
    goto failed;
  }
  xpcie_fpga_common_print_module_info(dev->mods.cms, "CMS");
#endif

  return 0;

failed:

  xpcie_info(" * Failed to get base address as Module FPGA.");

  return -EFAULT;
}


void
xpcie_fpga_copy_base_address_for_user(
  fpga_dev_info_t *dev,
  fpga_address_map_t *map)
{
  xpcie_trace("%s: dev(%s)", __func__, dev->serial_id);
  // global
  map->global.base = dev->mods.global.base;
  map->global.len = dev->mods.global.len;
  map->global.num = dev->mods.global.num;

  // chain
  map->chain.base = dev->mods.chain.base;
  map->chain.len = dev->mods.chain.len;
  map->chain.num = dev->mods.chain.num;

  // direct
  map->direct.base = dev->mods.direct.base;
  map->direct.len = dev->mods.direct.len;
  map->direct.num = dev->mods.direct.num;

  // lldma
  map->lldma.base = dev->mods.lldma.base;
  map->lldma.len = dev->mods.lldma.len;
  map->lldma.num = dev->mods.lldma.num;

  // ptu
  map->ptu.base = dev->mods.ptu.base;
  map->ptu.len = dev->mods.ptu.len;
  map->ptu.num = dev->mods.ptu.num;

  // conv
  map->conv.base = dev->mods.conv.base;
  map->conv.len = dev->mods.conv.len;
  map->conv.num = dev->mods.conv.num;

  // func
  map->func.base = dev->mods.func.base;
  map->func.len = dev->mods.func.len;
  map->func.num = dev->mods.func.num;

  // cms
  map->cms.base = dev->mods.cms.base;
  map->cms.len = dev->mods.cms.len;
  map->cms.num = dev->mods.cms.num;
}


int
xpcie_fpga_dev_init(fpga_dev_info_t *dev)
{
  int ret;
  struct pci_dev *pdev = dev->pci_dev;
#if defined(ENABLE_MODULE_LLDMA)
  int chid;
#endif

  xpcie_info("%s", __func__);
  INIT_LIST_HEAD(&dev->list);
  mutex_init(&dev->queue_mutex);
  spin_lock_init(&dev->lock);

  // Get Base Address of registers from pci structure.
  dev->base_addr_hw = pci_resource_start(pdev, 0);
  if (dev->base_addr_hw < 0) {
    xpcie_alert("Init: Base Address not set");
    return -ENODEV;
  }
  dev->base_addr_len = pci_resource_len(pdev, 0);

  /* Remap the I/O register block so that it can be safely accessed.
   * I/O register block start at dev->base_addr_hw.
   * It is cast to char because that is the way linux does it.
   * Reference "/usr/src/Linux-2.4/Documentation/IO-mapping.txt".
   */
  dev->base_addr = ioremap(dev->base_addr_hw, dev->base_addr_len);
  if (dev->base_addr == NULL) {
    xpcie_alert("Init: could not remap memory for I/O");
    return -ENODEV;
  }
  xpcie_info("Dev_id=%d", dev->dev_id);
  xpcie_info("Address length =%#018llx", dev->base_addr_len);
  xpcie_info("Base hw address=%#018llx", (uint64_t)dev->base_addr_hw);
  xpcie_info("Ioremap address=%#018llx", (uint64_t)dev->base_addr);

  // Get parent bitstream_id
  pci_read_config_dword(
    pdev,
    pci_find_vsec_capability(pdev, pdev->vendor, PCI_VSEC_ID_XILINX)
      + PCI_BITSTREAM_ID_OFFSET,
    &dev->bitstream_id.parent);
  xpcie_info("ParentBitstream=%08x", dev->bitstream_id.parent);

  // Check type of Regiser Map@All
  ret = xpcie_fpga_get_control_type(dev);
  if (ret) {
    xpcie_err("Failed to get FPGA's Address Map in (%d)...", ret);
    return -ENODEV;
  }

#if defined(ENABLE_MODULE_GLOBAL)
  // Get child bitstream_id
  dev->bitstream_id.child = xpcie_fpga_global_get_major_version(dev);
#endif

#if defined(ENABLE_MODULE_LLDMA)
  // Allocate and initialize command queue at kernel memory
  xpcie_trace("%s : queue_que_init start", __func__);
  for (chid = 0; chid < XPCIE_MAX_QUEUE_PAIR; chid++) {
    ret = queue_que_init(&dev->enqueues[chid], XPCIE_QUEUE_SIZE);
    if (ret < 0) {
      xpcie_err("%s error! que = enqueue, num = %d", __func__, chid);
      return ret;
    }
    ret = queue_que_init(&dev->dequeues[chid], XPCIE_QUEUE_SIZE);
    if (ret < 0) {
      xpcie_err("%s error! que = dequeue, num = %d", __func__, chid);
      return ret;
    }
  }
  xpcie_trace("%s : queue_que_init done(%d)", __func__, XPCIE_MAX_QUEUE_PAIR);
#endif

#if defined(ENABLE_SETTING_IN_DRIVER)
#if defined(ENABLE_MODULE_LLDMA)
  // Set DMA's chain buffer@LLDMA
  xpcie_fpga_set_lldma_buffer(dev, true);
#endif  // ENABLE_MODULE_LLDMA

#if defined(ENABLE_MODULE_CMS)
  // Reset CMS's module@CMS
  xpcie_fpga_set_cms_unrest(dev, 1);
#endif  // ENABLE_MODULE_CMS
#endif  // ENABLE_SETTING_IN_DRIVER

  // Initailize function chain table
  memset(dev->fch_dev_table, 0xFF, sizeof(dev->fch_dev_table));

  // Get serial_id and card_name@CMS
#if !defined(XPCIE_UNUSE_SERIAL_ID) && defined(ENABLE_MODULE_CMS)
  ret = xpcie_fpga_get_mailbox(dev, dev->serial_id, dev->card_name);
  if (ret < 0) {
    xpcie_err("Failed to get mailbox!");
    return ret;
  }
#else
  strcpy(dev->serial_id, "<Implementing>");
  strcpy(dev->card_name, "<Implementing>");
#endif  // ENABLE_MODULE_CMS !XPCIE_UNUSE_SERIAL_ID
  xpcie_info("SERIAL_ID=%s", dev->serial_id);
  xpcie_info("CARD_NAME=%s", dev->card_name);

  return 0;
}


void
xpcie_fpga_dev_close(fpga_dev_info_t *dev)
{
#if defined(ENABLE_MODULE_LLDMA)
  int chid;
#endif

  xpcie_info("%s : Dev_id = %d", __func__, dev->dev_id);

#if defined(ENABLE_MODULE_LLDMA)
  // Stop all dma channels when rmmod
  for (chid = 0; chid < dev->available_dma_channel_num; chid++) {
    xpcie_fpga_stop_queue(dev, chid, DMA_HOST_TO_DEV);
    xpcie_fpga_stop_queue(dev, chid, DMA_DEV_TO_HOST);
  }
#endif  // ENABLE_MODULE_LLDMA

#if defined(ENABLE_MODULE_LLDMA)
  // Free command queue's memory
  xpcie_trace("%s : queue_que_free start", __func__);
  for (chid = 0; chid < XPCIE_MAX_QUEUE_PAIR; chid++) {
    queue_que_free(&dev->enqueues[chid]);
    queue_que_free(&dev->dequeues[chid]);
  }
  xpcie_trace("%s : queue_que_free done(%d)", __func__, XPCIE_MAX_QUEUE_PAIR);
#endif

  if (dev->base_addr) {
    iounmap(dev->base_addr);
  }
  dev->base_addr = NULL;
}


int
xpcie_fpga_get_control_type(fpga_dev_info_t *dev)
{
  int ret = 0;
  xpcie_trace("%s: minor_num(%d)", __func__, dev->dev_id);

  dev->mods.ctrl_type = FPGA_CONTROL_UNKNOWN;

  ret = xpcie_fpga_common_get_module_info(dev);
  if (ret == 0)
    dev->mods.ctrl_type = FPGA_CONTROL_MODULE;

  xpcie_info(" FPGA[%02d]'s MAP is being considered as %s", dev->dev_id,
    dev->mods.ctrl_type == FPGA_CONTROL_MODULE        ? "module"
                                                      : "unknown");

  return ret;
}


/**
 * @brief Function which print reference count's information for module
 * @param[in] mod
 *   Target module's information
 * @param[in] name
 *   String for print
 */
static inline void
__print_refcount_module(
  fpga_module_info_t *mod,
  char *name
) {
  int lane;
  for (lane = 0; lane < mod->num; lane++)
    xpcie_info(" REFCNT[%s][%d] : %d", name, lane, mod->refcount[lane]);
}


/**
 * @brief Function which print reference count's information for device
 * @param[in] dev
 *   Target FPGA's information
 */
static inline void
__print_refcount_dev(
  fpga_dev_info_t *dev
) {
#if defined(ENABLE_REFCOUNT_GLOBAL) && defined(ENABLE_MODULE_GLOBAL)
  __print_refcount_module(&dev->mods.global,  "GLOBAL");
#endif
#if defined(ENABLE_REFCOUNT_CHAIN) && defined(ENABLE_MODULE_CHAIN)
  __print_refcount_module(&dev->mods.chain,  "CHAIN ");
#endif
#if defined(ENABLE_REFCOUNT_DIRECT) && defined(ENABLE_MODULE_DIRECT)
  __print_refcount_module(&dev->mods.direct,  "DIRECT");
#endif
#if defined(ENABLE_REFCOUNT_LLDMA) && defined(ENABLE_MODULE_LLDMA)
  __print_refcount_module(&dev->mods.lldma,    "LLDMA ");
#endif
#if defined(ENABLE_REFCOUNT_PTU) && defined(ENABLE_MODULE_PTU)
  __print_refcount_module(&dev->mods.ptu,     "PTU   ");
#endif
#if defined(ENABLE_REFCOUNT_CONV) && defined(ENABLE_MODULE_CONV)
  __print_refcount_module(&dev->mods.conv,    "CONV  ");
#endif
#if defined(ENABLE_REFCOUNT_FUNC) && defined(ENABLE_MODULE_FUNC)
  __print_refcount_module(&dev->mods.func,    "FUNC  ");
#endif
#if defined(ENABLE_REFCOUNT_CMS) && defined(ENABLE_MODULE_CMS)
  __print_refcount_module(&dev->mods.cms, "CMS   ");
#endif
}


/**
 * @brief Function which print the command's result
 * @param[in] dev
 *   Target FPGA's information
 * @param[in] result
 *   The result of the command
 * @param[in] cmd
 *   The command
 * @param[in] region
 *   The target region of the command
 */
static inline void
__print_refcount_result(
  fpga_dev_info_t *dev,
  int result,
  xpcie_refcount_cmd_t cmd,
  xpcie_region_t region
) {
  switch (result) {
  case 0:
    if (cmd == XPCIE_DEV_REFCOUNT_RST)
      xpcie_err(" %s[region:%d] reset reference count by forced", dev->serial_id, region);
    break;
  case -XPCIE_DEV_REFCOUNT_USING:
    xpcie_err(" %s[region:%d] is now being used", dev->serial_id, region);
    break;
  case -XPCIE_DEV_REFCOUNT_WRITING:
    xpcie_err(" %s[region:%d] is now being written bitstream", dev->serial_id, region);
    break;
  case -EBUSY:
    xpcie_err(" %s[region:%d]'s refcount is already 0", dev->serial_id, region);
    break;
  case -EFAULT:
    xpcie_err(" %s[region:%d]'s refcount is ERROR!!!", dev->serial_id, region);
    break;
  case -ENODEV:
    xpcie_err(" %s[region:%d]'s refcount is NOT exist", dev->serial_id, region);
    break;
  case -EINVAL:
  default:
    xpcie_err(" %s[region:%d] unknown command received...(ret:%d,cmd:%u)", dev->serial_id, region, result, cmd);
    break;
  }
#ifdef XPCIE_TRACE_LOG
  __print_refcount_dev(dev);
#endif
}


/**
 * @brief Function which add reference count to the list
 * @param[out] refcount_list
 *   Target array to control(the last should be null for sentinel)
 * @param[in] refcount_add
 *   The value to add
 */
static inline void
__add_refcount_list(
  int *refcount_list[],
  int refcount_add
) {
  int index;
  int *refcount;
  for (index = 0;(refcount = refcount_list[index]); index++)
    (*refcount) = (*refcount) + refcount_add;
}


/**
 * @brief Function which set reference count to the list
 * @param[out] refcount_list
 *   Target array to control(the last should be null for sentinel)
 * @param[in] refcount_set
 *   The value to set
 */
static inline void
__set_refcount_list(
  int *refcount_list[],
  int refcount_set
) {
  int index;
  int *refcount;
  for (index = 0;(refcount = refcount_list[index]); index++)
    (*refcount) = refcount_set;
}


/**
 * @brief Function which get the sum of reference counts from the list
 * @param[in] refcount_list
 *   Target array to control(the last should be null for sentinel)
 * @param[out] refcount_get
 *   pointer variable to get the sum of reference count in the list
 */
static inline void
__get_refcount_list(
  int *refcount_list[],
  int *refcount_get
) {
  int index;
  int *refcount;
  *refcount_get = 0;
  for (index = 0;(refcount = refcount_list[index]); index++)
    (*refcount_get) += (*refcount);
}


/**
 * @brief Function which return true if any refcount matches
 * @param[in] refcount_list
 *   Target array to control(the last should be null for sentinel)
 * @param[in] refcount_cmp
 *   The matching value with the list
 * @param[in] clb
 *   The function which check matching refcount
 */
static inline bool
__is_refcount_any(
  int *refcount_list[],
  int refcount_cmp,
  bool(*clb)(int,int)
) {
  int index;
  int *refcount;
  for (index = 0;(refcount = refcount_list[index]); index++)
    if (clb(*refcount, refcount_cmp))
      return true;
  return false;
}


/**
 * @brief Function which return true if all refcount matches
 * @param[in] refcount_list
 *   Target array to control(the last should be null for sentinel)
 * @param[in] refcount_cmp
 *   The matching value with the list
 * @param[in] clb
 *   The function which check matching refcount
 */
static inline bool
__is_refcount_all(
  int *refcount_list[],
  int refcount_cmp,
  bool(*clb)(int,int)
) {
  int index;
  int *refcount;
  for (index = 0;(refcount = refcount_list[index]); index++)
    if (clb(*refcount, refcount_cmp))
      return false;
  return true;
}


/**
 * @brief Callback Function which return true if parameter match
 * @param[in] refcount
 *   Reference count(base)
 * @param[in] refcount_cmp
 *   Reference count
 */
static inline bool __clb_is_same(int refcount, int refcount_cmp) {
  return refcount == refcount_cmp;
}


/**
 * @brief Callback Function which return true if parameter doesn't match
 * @param[in] refcount
 *   Reference count(base)
 * @param[in] refcount_cmp
 *   Reference count
 */
static inline bool __clb_is_differ(int refcount, int refcount_cmp) {
  return refcount != refcount_cmp;
}


/**
 * @brief Callback Function which return true if parameter is less
 * @param[in] refcount
 *   Reference count(base)
 * @param[in] refcount_cmp
 *   Reference count
 */
static inline bool __clb_is_less(int refcount, int refcount_cmp) {
  return refcount < refcount_cmp;
}


/**
 * @brief Function which check any refcount is the same
 * @param[in] refcount_list
 *   Target array to control(the last should be null for sentinel)
 * @param[in] refcount_cmp
 *   The matching value with the list
 */
static inline bool __is_refcount_any_same(int *refcount_list[], int refcount_cmp) {
  return __is_refcount_any(refcount_list, refcount_cmp, __clb_is_same);
}


/**
 * @brief Function which check all refcount is the same
 * @param[in] refcount_list
 *   Target array to control(the last should be null for sentinel)
 * @param[in] refcount_cmp
 *   The matching value with the list
 */
static inline bool __is_refcount_all_same(int *refcount_list[], int refcount_cmp) {
  return __is_refcount_all(refcount_list, refcount_cmp, __clb_is_differ);
}


/**
 * @brief Function which check any refcount is less than comparing data
 * @param[in] refcount_list
 *   Target array to control(the last should be null for sentinel)
 * @param[in] refcount_cmp
 *   The matching value with the list
 */
static inline bool __is_refcount_any_less(int *refcount_list[], int refcount_cmp) {
  return __is_refcount_any(refcount_list, refcount_cmp, __clb_is_less);
}


/**
 * @brief Function which check any refcount is different
 * @param[in] refcount_list
 *   Target array to control(the last should be null for sentinel)
 * @param[in] refcount_cmp
 *   The matching value with the list
 */
static inline bool __is_refcount_any_differ(int *refcount_list[], int refcount_cmp) {
  return __is_refcount_any(refcount_list, refcount_cmp, __clb_is_differ);
}


/**
 * @brief Function which check all refcount is different
 * @param[in] refcount_list
 *   Target array to control(the last should be null for sentinel)
 * @param[in] refcount_cmp
 *   The matching value with the list
 */
static inline bool __is_refcount_all_differ(int *refcount_list[], int refcount_cmp) {
  return __is_refcount_all(refcount_list, refcount_cmp, __clb_is_same);
}


/**
 * @brief Function which control refcount status
 * @param[in] refcount_list
 *   Target array to control(the last should be null for sentinel)
 * @param[in] cmd
 *   Command to control refcount
 * @param[out] refcount_get
 *   pointer variable to get refcount
 */
static inline int
__control_refcount_status(
  int *refcount_list[],
  xpcie_refcount_cmd_t cmd,
  int *refcount_get
) {
  if (!refcount_list[0])
    return -ENODEV;
  switch (cmd) {
  case XPCIE_DEV_REFCOUNT_INC:
    if (__is_refcount_any_same(refcount_list, -1))
      return -XPCIE_DEV_REFCOUNT_WRITING;
    if (__is_refcount_any_less(refcount_list, 0))
      return -EFAULT;
    __add_refcount_list(refcount_list, 1);
    break;
  case XPCIE_DEV_REFCOUNT_DEC:
    if (__is_refcount_any_same(refcount_list, -1))
      return -XPCIE_DEV_REFCOUNT_WRITING;
    if (__is_refcount_any_less(refcount_list, 0))
      return -EFAULT;
    if (__is_refcount_any_same(refcount_list, 0))
      return -EBUSY;
    __add_refcount_list(refcount_list, -1);
    break;
  case XPCIE_DEV_REFCOUNT_WRITE:
    if (__is_refcount_any_same(refcount_list, -1))
      return -XPCIE_DEV_REFCOUNT_WRITING;
    if (__is_refcount_any_less(refcount_list, 0))
      return -EFAULT;
    if (__is_refcount_any_differ(refcount_list, 0))
      return -XPCIE_DEV_REFCOUNT_USING;
    __set_refcount_list(refcount_list, -1);
    break;
  case XPCIE_DEV_REFCOUNT_CLEAR:
    if (__is_refcount_any_differ(refcount_list, -1))
      return -XPCIE_DEV_REFCOUNT_USING;
    __set_refcount_list(refcount_list, 0);
    break;
  case XPCIE_DEV_REFCOUNT_RST:
    __set_refcount_list(refcount_list, 0);
    break;
  case XPCIE_DEV_REFCOUNT_GET:
    __get_refcount_list(refcount_list, refcount_get);
    break;
  default:
    return -EINVAL;
  }
  return 0;
}


/**
 * @brief Function which control refcount status of device
 * @param[in,out] dev
 *   pointer variable to set refcount of FPGA device
 * @param[in] cmd
 *   Command to control refcount
 * @param[out] refcount_get
 *   pointer variable to get refcount
 */
static inline int
__control_refcount_dev(
  fpga_dev_info_t *dev,
  xpcie_refcount_cmd_t cmd,
  int *refcount_get
) {
  int lane = -1;
  int index = 0;
  int *refcount_list[29]; // 5(mod)*4(lane)+8(mod)*1(lane)+1(sentinel)

  // create refcount_list of dev
#if defined(ENABLE_REFCOUNT_GLOBAL) && defined(ENABLE_MODULE_GLOBAL)
  for (lane = 0; lane < dev->mods.global.num; lane++)
    refcount_list[index++] = &dev->mods.global.refcount[lane];
#endif
#if defined(ENABLE_REFCOUNT_CHAIN) && defined(ENABLE_MODULE_CHAIN)
  for (lane = 0; lane < dev->mods.chain.num; lane++)
    refcount_list[index++] = &dev->mods.chain.refcount[lane];
#endif
#if defined(ENABLE_REFCOUNT_DIRECT) && defined(ENABLE_MODULE_DIRECT)
  for (lane = 0; lane < dev->mods.direct.num; lane++)
    refcount_list[index++] = &dev->mods.direct.refcount[lane];
#endif
#if defined(ENABLE_REFCOUNT_LLDMA) && defined(ENABLE_MODULE_LLDMA)
  for (lane = 0; lane < dev->mods.lldma.num; lane++)
    refcount_list[index++] = &dev->mods.lldma.refcount[lane];
#endif
#if defined(ENABLE_REFCOUNT_PTU) && defined(ENABLE_MODULE_PTU)
  for (lane = 0; lane < dev->mods.ptu.num; lane++)
    refcount_list[index++] = &dev->mods.ptu.refcount[lane];
#endif
#if defined(ENABLE_REFCOUNT_CONV) && defined(ENABLE_MODULE_CONV)
  for (lane = 0; lane < dev->mods.conv.num; lane++)
    refcount_list[index++] = &dev->mods.conv.refcount[lane];
#endif
#if defined(ENABLE_REFCOUNT_FUNC) && defined(ENABLE_MODULE_FUNC)
  for (lane = 0; lane < dev->mods.func.num; lane++)
    refcount_list[index++] = &dev->mods.func.refcount[lane];
#endif
#if defined(ENABLE_REFCOUNT_CMS) && defined(ENABLE_MODULE_CMS)
  for (lane = 0; lane < dev->mods.cms.num; lane++)
    refcount_list[index++] = &dev->mods.cms.refcount[lane];
#endif
  refcount_list[index] = NULL;

  if (lane == -1)
    xpcie_err(" This driver may not be able to control device refcount...");

  // control refcount by lane
  return __control_refcount_status(refcount_list, cmd, refcount_get);
}


/**
 * @brief Function which control refcount status of device lane
 * @param[in,out] dev
 *   pointer variable to set refcount of FPGA device
 * @param[in] lane
 *   Target lane
 * @param[in] cmd
 *   Command to control refcount
 * @param[out] refcount_get
 *   pointer variable to get refcount
 */
static inline int
__control_refcount_lane(
  fpga_dev_info_t *dev,
  int lane,
  xpcie_refcount_cmd_t cmd,
  int *refcount_get
) {
  int *refcount_list[6]; // 5(mod)+1(sentinel)
  int index = 0;

  // create refcount_list of lane
#if defined(ENABLE_REFCOUNT_CHAIN) && defined(ENABLE_MODULE_CHAIN)
  if (lane >= dev->mods.chain.num)
    return -ENODEV;
  refcount_list[index++] = &dev->mods.chain.refcount[lane];
#endif
#if defined(ENABLE_REFCOUNT_DIRECT) && defined(ENABLE_MODULE_DIRECT)
  if (lane >= dev->mods.direct.num)
    return -ENODEV;
  refcount_list[index++] = &dev->mods.direct.refcount[lane];
#endif
#if defined(ENABLE_REFCOUNT_PTU) && defined(ENABLE_MODULE_PTU)
  if (lane >= dev->mods.ptu.num)
    return -ENODEV;
  refcount_list[index++] = &dev->mods.ptu.refcount[lane];
#endif
#if defined(ENABLE_REFCOUNT_CONV) && defined(ENABLE_MODULE_CONV)
  if (lane >= dev->mods.conv.num)
    return -ENODEV;
  refcount_list[index++] = &dev->mods.conv.refcount[lane];
#endif
#if defined(ENABLE_REFCOUNT_FUNC) && defined(ENABLE_MODULE_FUNC)
  if (lane >= dev->mods.func.num)
    return -ENODEV;
  refcount_list[index++] = &dev->mods.func.refcount[lane];
#endif
  refcount_list[index] = NULL;

  // control refcount by lane
  return __control_refcount_status(refcount_list, cmd, refcount_get);
}


/**
 * @brief Function which control refcount status of device region
 * @param[in,out] mod
 *   pointer variable to set refcount of FPGA module
 * @param[in] lane
 *   Target lane
 * @param[in] cmd
 *   Command to control refcount
 * @param[out] refcount_get
 *   pointer variable to get refcount
 */
static inline int
__control_refcount_region(
  fpga_module_info_t *mod,
  int lane,
  xpcie_refcount_cmd_t cmd,
  int *refcount_get
) {
  // create refcount_list of region
  int *refcount_list[] = {
    (lane < mod->num) ? &mod->refcount[lane] : NULL,
    NULL};

  // control refcount by region
  return __control_refcount_status(refcount_list, cmd, refcount_get);
}


int
xpcie_fpga_control_refcount(
  fpga_dev_info_t *dev,
  xpcie_refcount_cmd_t cmd,
  xpcie_region_t region,
  int *refcount_get
) {
  int ret = 0;

  xpcie_trace("%s:(cmd(%u), region(%u))", __func__, cmd, region);

  spin_lock(&dev->lock_refcount);
  switch (region) {
  case XPCIE_DEV_REGION_ALL:
    ret = __control_refcount_dev(dev, cmd, refcount_get);
    break;
  default:
    spin_unlock(&dev->lock_refcount);
    xpcie_err(" %s unknown region received...(%u)", dev->serial_id, region);
    return -EINVAL;
  }
  __print_refcount_result(dev, ret, cmd, region);
  spin_unlock(&dev->lock_refcount);

  return ret;
}
