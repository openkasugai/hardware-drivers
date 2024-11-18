/*************************************************
* Copyright 2024 NTT Corporation, FUJITSU LIMITED
* Licensed under the GPL-2.0  License, see LICENSE for details.
* SPDX-License-Identifier: GPL-2.0-or-later
*************************************************/

#include <linux/init.h>
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/uaccess.h>

#include <asm/mwait.h>

#include <libxpcie.h>


#define MINOR_BASE  0                               /**< Base number of Minor number */
#define MINOR_NUM XPCIE_MAX_DEVICE_NUM              /**< Available num of FPGA */

#define DRIVER_NAME XPCIE_DEVICE_NAME               /**< Driver's name definition */

#define PCI_VENDOR_ID_XILINX              0x10ee    /**< Xilinx : Vendor_id */
#define PCI_DEVICE_ID_XILINX_PCIE_DEFAULT 0x903f    /**< Xilinx : PCI Device ID */

#define XPCIE_DEV_LINK_WIDTH    16                  /**< Expected link width */

/**
 * static global variable: Table which manages by this driver.
 */
static const struct pci_device_id xpcie_pci_id_table[] = {
  { PCI_DEVICE(PCI_VENDOR_ID_XILINX, PCI_DEVICE_ID_XILINX_PCIE_DEFAULT) },
  { 0, },
};

/**
 * static global variable: Class information of this driver
 */
static struct class *xpcie_cdev_class = NULL;

/**
 * static global variable: Major number of this driver
 */
static unsigned int xpcie_cdev_major;

/**
 * static global variable: Device list initialized by this driver
 */
static struct list_head fpga_dev_list;

/**
 * static global variable: Num of FPGAs initialized by this driver
 */
static int    fpga_num_devs = 0;

/**
 * Mutex for `fpga_dev_list`
 */
DEFINE_MUTEX(fpga_dev_list_mutex);


fpga_dev_info_t*
xpcie_fpga_get_device_by_minor(
  uint32_t minor_num)
{
  fpga_dev_info_t *dev = NULL;
  struct list_head *pos;

  mutex_lock(&fpga_dev_list_mutex);
  list_for_each(pos, &fpga_dev_list) {
    dev = container_of(pos, fpga_dev_info_t, list);
    if (dev && dev->dev_id == minor_num)
      break;
  }
  mutex_unlock(&fpga_dev_list_mutex);

  return dev;
}


uint64_t
xpcie_fpga_get_baseaddr(
  uint8_t minor_num)
{
  fpga_dev_info_t *dev = NULL;

  // Search mathcing device from dev_list
  dev = xpcie_fpga_get_device_by_minor(minor_num);
  if (dev == NULL || dev->dev_id != minor_num) {
    xpcie_err("xpcie_fpga_get_baseaddr error! NO such a device found!(%s%d)",
      XPCIE_DEVICE_NAME, minor_num);
    return 0;
  }
  return (uint64_t)dev->base_addr_hw;
}


/**
 * @brief Function which re-trainging pci device speed
 */
static void
retrain_device_speed(struct fpga_dev_info *dev)
{
  struct pci_dev *pdev = dev->pci_dev;
  u16 link_status, speed, width;
  int pos, upos;
  u16 status_reg, control_reg, link_cap_reg;
  u16 status, control;  /* for upstream */
  u32 link_cap;
  int training, timeout;
  bool need_train = false;

  pos = pci_find_capability(pdev, PCI_CAP_ID_EXP);
  if (!pos) {
    xpcie_err("cannot find PCI Express capability!\n");
    return;
  }

  upos = pci_find_capability(dev->upstream, PCI_CAP_ID_EXP);
  status_reg   = upos + PCI_EXP_LNKSTA;
  control_reg  = upos + PCI_EXP_LNKCTL;
  link_cap_reg = upos + PCI_EXP_LNKCAP;
  pci_read_config_word(dev->upstream, status_reg, &status);
  pci_read_config_word(dev->upstream, control_reg, &control);
  pci_read_config_dword(dev->upstream, link_cap_reg, &link_cap);

  pci_read_config_word(pdev, pos + PCI_EXP_LNKSTA, &link_status);
  pci_read_config_dword(dev->upstream, link_cap_reg, &link_cap);
  speed = link_status & PCI_EXP_LNKSTA_CLS;
  width = (link_status & PCI_EXP_LNKSTA_NLW) >> PCI_EXP_LNKSTA_NLW_SHIFT;

  switch (speed) {
  case PCI_EXP_LNKSTA_CLS_2_5GB:
    xpcie_info("Link speed is 2.5 GT/s with %d lanes.", width);
    need_train = true;
    break;
  case PCI_EXP_LNKSTA_CLS_5_0GB:
    xpcie_info("Link speed is 5.0 GT/s with %d lanes.", width);
    if (width != XPCIE_DEV_LINK_WIDTH)
      need_train = true;
    break;
  case PCI_EXP_LNKSTA_CLS_8_0GB:
    xpcie_info("Link speed is 8.0 GT/s with %d lanes.", width);
    if (width != XPCIE_DEV_LINK_WIDTH)
      need_train = true;
    break;
  default:
    xpcie_warn("Not sure what's going on. Retraining.\n");
    goto retrain;
  }
  if (need_train)
    xpcie_info("  need to retrain.\n");
  else
    return;

retrain:
  // Perform link training
  training = 1;
  timeout  = 0;
  pci_read_config_word(dev->upstream, control_reg, &control);
  pci_write_config_word(dev->upstream, control_reg,
        control | PCI_EXP_LNKCTL_RL);

  while (training && timeout < 50) {
    pci_read_config_word(dev->upstream, status_reg, &status);
    training = (status & PCI_EXP_LNKSTA_LT);
    msleep(1); /* 1ms */
    timeout++;
  }
  if (training) {
    xpcie_info("Error: Link training timed out.\n");
    xpcie_info("PCIe link not established.\n");
  } else {
    xpcie_info("Link training completed in %d ms.\n", timeout);
  }

  // Verify that it's a 8 GT/s link now
  pci_read_config_word(pdev, pos + PCI_EXP_LNKSTA, &link_status);
  pci_read_config_dword(dev->upstream, link_cap_reg, &link_cap);
  speed = link_status & PCI_EXP_LNKSTA_CLS;
  width = (link_status & PCI_EXP_LNKSTA_NLW) >> PCI_EXP_LNKSTA_NLW_SHIFT;

  if (speed == PCI_EXP_LNKSTA_CLS_8_0GB) {
    xpcie_info("Link operating at 8 GT/s with %d lanes", width);
  } else if (speed == PCI_EXP_LNKSTA_CLS_5_0GB) {
    xpcie_info("Link operating at 5 GT/s with %d lanes", width);
  } else {
    xpcie_warn("** WARNING: Link training failed.\n");
    xpcie_info("Link speed is 2.5 GT/s with %d lanes.", width);
  }
}


/**
 * @brief Function which probe PCI device as xpcie driver
 */
static int
xpcie_pci_probe(struct pci_dev *pdev, const struct pci_device_id *ent)
{
  struct list_head *pos;
  fpga_dev_info_t *dev;
  int err;

  dev_info(&pdev->dev, "found FPGA\n");
  err = pci_enable_device(pdev);
  if (err < 0) {
    xpcie_alert("Init: device not enabled");
    return err;
  }

  dev = vmalloc(sizeof(fpga_dev_info_t));
  if (dev == NULL) {
    xpcie_alert("Init: could not allocate memory for fpga_dev");
    return -ENOMEM;
  }
  memset(dev, 0, sizeof(fpga_dev_info_t));

  // Initialize fpga_dev_info structure and FPGA device
  dev->dev_id  = fpga_num_devs;
  dev->pci_dev = pdev;
  err = xpcie_fpga_dev_init(dev);
  if (err < 0) {
    goto error;
  }

  // Update num of devices of this device and devices in fpga_dev_list
  dev->num_devs = 0;
  mutex_lock(&fpga_dev_list_mutex);
  list_for_each(pos, &fpga_dev_list) {
    fpga_dev_info_t *odev = container_of(pos, fpga_dev_info_t, list);

    odev->num_devs++;

    dev->num_devs++;
  }
  mutex_unlock(&fpga_dev_list_mutex);

  // Perform link training
  dev->upstream = pci_upstream_bridge(pdev);
  if (dev->upstream == NULL) {
    xpcie_alert("upstream error");
    goto error;
  }
  retrain_device_speed(dev);

  {
  u16 val;
  pci_read_config_word(pdev, pci_pcie_cap(pdev)+PCI_EXP_DEVCTL, &val);
  xpcie_info("PCIe DEVCTL=%#x", val);
  }

  // Create and register chrdev for the device
  cdev_init(&dev->cdev, xpcie_fpga_get_cdev_fops());
  dev->cdev.owner = THIS_MODULE;

  err = cdev_add(&dev->cdev, MKDEV(xpcie_cdev_major, dev->dev_id), 1);
  if (err < 0) {
    xpcie_err("cdev_add for fpga %d failed.", dev->dev_id);
    goto error;
  }

#if !defined(XPCIE_UNUSE_SERIAL_ID) && defined(ENABLE_MODULE_CMS)
  // Register device to /sys/class/<DRIVER_NAME>/<DRIVER_NAME>_<serial_id>
  device_create(xpcie_cdev_class, NULL, MKDEV(xpcie_cdev_major, dev->dev_id),
          NULL, "%s_%s", DRIVER_NAME, dev->serial_id);
#else
  // Register device to /sys/class/<DRIVER_NAME>/<DRIVER_NAME><minor_num>
  device_create(xpcie_cdev_class, NULL, MKDEV(xpcie_cdev_major, dev->dev_id),
          NULL, "%s%d", DRIVER_NAME, dev->dev_id);
#endif

  mutex_lock(&fpga_dev_list_mutex);
  list_add(&dev->list, &fpga_dev_list);
  fpga_num_devs++;
  mutex_unlock(&fpga_dev_list_mutex);

  return 0;

error:
  xpcie_fpga_dev_close(dev);
  vfree(dev);

  return err;
}


/**
 * @brief Function which remove PCI device as xpcie driver
 */
static void
xpcie_pci_remove(struct pci_dev *pdev)
{
  struct list_head *pos;
  fpga_dev_info_t *dev;
  bool found = false;

  mutex_lock(&fpga_dev_list_mutex);
  list_for_each(pos, &fpga_dev_list) {
    dev = container_of(pos, fpga_dev_info_t, list);
    if (dev->pci_dev == pdev) {
      list_del(&dev->list);
      found = true;
      break;
    }
  }
  mutex_unlock(&fpga_dev_list_mutex);

  if (found) {
    device_destroy(xpcie_cdev_class, MKDEV(xpcie_cdev_major, dev->dev_id));
    cdev_del(&dev->cdev);
    xpcie_fpga_dev_close(dev);
    vfree(dev);
  }
}


/**
 * @brief Function which change Device file's device mode
 */
static int
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(6, 4, 0))
xpcie_cdev_class_uevent(const struct device *dev, struct kobj_uevent_env *env)
#else
xpcie_cdev_class_uevent(struct device *dev, struct kobj_uevent_env *env)
#endif
{
  int ret = 0;
  ret += add_uevent_var(env, "DEVMODE=%#o", 0666);
  return ret;
}


/**
 * static global variable: Operations definition of this driver as pci device
 */
static struct pci_driver xpcie_pci_driver = {
  .name     = XPCIE_DEVICE_NAME,
  .id_table = xpcie_pci_id_table,
  .probe    = xpcie_pci_probe,
  .remove   = xpcie_pci_remove,
};


/**
 * @brief Function which Called when insmod
 */
static int
XPCIe_init_module(void)
{
  dev_t dev;
  int ret;

  xpcie_info("%s Driver Ver:(type:%02x)%02x.%02x.%02x-%02x",
    XPCIE_DEVICE_NAME,
    DRIVER_TYPE,
    DRIVER_MAJOR_VER,
    DRIVER_MINOR_VER,
    DRIVER_REVISION,
    DRIVER_PATCH);
  xpcie_fpga_print_build_options();

  // Allocate free Major Number for this module
  ret = alloc_chrdev_region(&dev, MINOR_BASE, MINOR_NUM, DRIVER_NAME);
  if (ret != 0) {
    xpcie_err("alloc_chrdev_region() failed = %d", ret);
    return ret;
  }
  xpcie_cdev_major = MAJOR(dev);

  dev = MKDEV(xpcie_cdev_major, MINOR_BASE);
  // Register class for this module
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(6, 4, 0))
  xpcie_cdev_class = class_create(DRIVER_NAME);
#else
  xpcie_cdev_class = class_create(THIS_MODULE, DRIVER_NAME);
#endif
  if (IS_ERR(xpcie_cdev_class)) {
    xpcie_err("cannot create class for this module");
    unregister_chrdev_region(dev, MINOR_NUM);
    return -ENODEV;
  }
  xpcie_cdev_class->dev_uevent = xpcie_cdev_class_uevent;

  INIT_LIST_HEAD(&fpga_dev_list);
  ret = pci_register_driver(&xpcie_pci_driver);
  if (ret != 0) {
    xpcie_err("pci_register_driver() failed = %d", ret);
    class_destroy(xpcie_cdev_class);
    unregister_chrdev_region(dev, MINOR_NUM);
    return ret;
  }
  if (list_empty(&fpga_dev_list)) {
    xpcie_alert("XPCIE DMA Device not found.");
    pci_unregister_driver(&xpcie_pci_driver);
    class_destroy(xpcie_cdev_class);
    unregister_chrdev_region(dev, MINOR_NUM);
    return -ENODEV;
  }

  return 0;
}


/**
 * @brief Function which Called when rmmod
 */
static void
XPCIe_exit_module(void)
{
  pci_unregister_driver(&xpcie_pci_driver);
  class_destroy(xpcie_cdev_class);
  unregister_chrdev_region(MKDEV(xpcie_cdev_major, MINOR_BASE), MINOR_NUM);
}

module_init(XPCIe_init_module);
module_exit(XPCIe_exit_module);

MODULE_LICENSE("GPL");
