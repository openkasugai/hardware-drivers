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

#if defined(ENABLE_MODULE_CMS)
#include "cms/libxpcie_cms.h"
#endif


/**
 * @brief Function which execute ioctl commands for driver basically
 */
static long
xpcie_fpga_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
  struct xpcie_file_private *private = filp->private_data;
  fpga_dev_info_t *dev = private->dev;
  long ret;

  ret = 0;
  switch (cmd) {
  // Set reference count of target regions
  case XPCIE_DEV_DRIVER_SET_REFCOUNT:
    {
      fpga_ioctl_refcount_t ioctl_data;
      if (copy_from_user(&ioctl_data, (void __user *)arg, sizeof(fpga_ioctl_refcount_t))) {
        ret = -EFAULT;
        break;
      }
      ret = xpcie_fpga_control_refcount(dev, ioctl_data.cmd, ioctl_data.region, NULL);
    }
    break;

  // Get reference count of target regions
  case XPCIE_DEV_DRIVER_GET_REFCOUNT:
    {
      fpga_ioctl_refcount_t ioctl_data;
      if (copy_from_user(&ioctl_data, (void __user *)arg, sizeof(ioctl_data))) {
        ret = -EFAULT;
        break;
      }
      ret = xpcie_fpga_control_refcount(dev, ioctl_data.cmd, ioctl_data.region, &ioctl_data.refcount);
      if (copy_to_user((void __user *)arg, &ioctl_data, sizeof(ioctl_data))) {
        ret = -EFAULT;
        break;
      }
    }
    break;

  // Set register accesability
  case XPCIE_DEV_DRIVER_SET_REG_LOCK:
    {
      uint32_t flag;
      if (copy_from_user(&flag, (void __user *)arg, sizeof(uint32_t))) {
        ret = -EFAULT;
        break;
      }
      switch (flag) {
      case XPCIE_DEV_REG_ENABLE:
        private->is_avail_rw = true;
        break;
      case XPCIE_DEV_REG_DISABLE:
        private->is_avail_rw = false;
        break;
      default:
        ret = -EINVAL;
        break;
      }
    }
    break;

  // Get minor number
  case XPCIE_DEV_DRIVER_GET_DEVICE_ID:
    if (copy_to_user((void __user *)arg, &dev->dev_id, sizeof(uint32_t))) {
      ret = -EFAULT;
    }
    break;

  // Get driver version
  case XPCIE_DEV_DRIVER_GET_VERSION:
    {
      uint32_t data = 0;
      data |= (uint8_t)DRIVER_MAJOR_VER << 24;
      data |= (uint8_t)DRIVER_MINOR_VER << 16;
      data |= (uint8_t)DRIVER_REVISION  << 8;
      data |= (uint8_t)DRIVER_PATCH;
      if (copy_to_user((void __user *)arg, &data, sizeof(data))) {
        ret = -EFAULT;
        break;
      }
    }
    break;

  // Get FPGA's register block map's type
  case XPCIE_DEV_DRIVER_GET_FPGA_TYPE:
    if (copy_to_user((void __user *)arg, &dev->mods.ctrl_type, sizeof(enum FPGA_CONTROL_TYPE))) {
      ret = -EFAULT;
    }
    break;

  // Get FPGA's register block map's offset and length
  case XPCIE_DEV_DRIVER_GET_FPGA_ADDR_MAP:
    {
      fpga_address_map_t map;

      // get base address information
      xpcie_fpga_copy_base_address_for_user(dev, &map);

      if (copy_to_user((void __user *)arg, &map, sizeof(map))) {
        ret = -EFAULT;
      }
    }
    break;

  // Update information for FPGA
  case XPCIE_DEV_DRIVER_SET_FPGA_UPDATE:
    {
      // Get FPGA type(block register map)
      if (xpcie_fpga_get_control_type(dev)) {
        xpcie_err("FPGA(dev_id(%u)) is UNKNOWN REGISTER MAP...", dev->dev_id);
        return -EFAULT;
      }
      xpcie_info("Driver Update FPGA's information");
    }
    break;

  case XPCIE_DEV_DRIVER_GET_DEVICE_INFO:
    {
      fpga_card_info_t info;

      // bitstream_id information
      info.bitstream_id = dev->bitstream_id;

      // pci device information
      info.pci_device_id = dev->pci_dev->device;
      info.pci_vendor_id = dev->pci_dev->vendor;

      // pci bus information
      info.pci_domain = pci_domain_nr(dev->pci_dev->bus);
      info.pci_bus    = dev->pci_dev->bus->number;
      info.pci_dev    = PCI_SLOT(dev->pci_dev->devfn);
      info.pci_func   = PCI_FUNC(dev->pci_dev->devfn);

      // card_name copy
      strcpy(info.card_name, dev->card_name);

      if (copy_to_user((void __user *)arg, &info, sizeof(info))) {
        ret = -EFAULT;
        break;
      }
    }
    break;

  default:
    private->is_valid_command = false;
    ret = -EINVAL;
    break;
  }

  return ret;
}


/**
 * @brief Function for ioctl()
 */
static long
xpcie_cdev_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
  struct xpcie_file_private *private = filp->private_data;
  long ret;

  private->is_valid_command = true;
  ret = xpcie_fpga_ioctl(filp, cmd, arg);
  if (private->is_valid_command)
    goto xpcie_cdev_ioctl_finish;

#if defined(ENABLE_MODULE_LLDMA)
  private->is_valid_command = true;
  ret = xpcie_fpga_ioctl_lldma(filp, cmd, arg);
  if (private->is_valid_command)
    goto xpcie_cdev_ioctl_finish;
#endif

#if defined(ENABLE_MODULE_CHAIN)
  private->is_valid_command = true;
  ret = xpcie_fpga_ioctl_chain(filp, cmd, arg);
  if (private->is_valid_command)
    goto xpcie_cdev_ioctl_finish;
#endif

#if defined(ENABLE_MODULE_DIRECT)
  private->is_valid_command = true;
  ret = xpcie_fpga_ioctl_direct(filp, cmd, arg);
  if (private->is_valid_command)
    goto xpcie_cdev_ioctl_finish;
#endif

#if defined(ENABLE_MODULE_CMS)
  private->is_valid_command = true;
  ret = xpcie_fpga_ioctl_cms(filp, cmd, arg);
  if (private->is_valid_command)
    goto xpcie_cdev_ioctl_finish;
#endif

#if defined(ENABLE_MODULE_GLOBAL)
  private->is_valid_command = true;
  ret = xpcie_fpga_ioctl_global(filp, cmd, arg);
  if (private->is_valid_command)
    goto xpcie_cdev_ioctl_finish;
#endif

xpcie_cdev_ioctl_finish:
  xpcie_trace("%s: cmd(%s), ret(%ld)", __func__, XPCIE_DEV_COMMAND_NAME(cmd), ret);
  if(unlikely(ret < 0)){
    xpcie_err("xpcie_cdev_ioctl error! cmd = %#x, ret = %ld", cmd, ret);
  }

  return ret;
}


/**
 * @brief Function for open()
 */
static int
xpcie_cdev_open(struct inode *inode, struct file *filp)
{
  struct xpcie_file_private *private;
  fpga_dev_info_t *dev = NULL;

  // Search the matching device from dev_list
  dev = xpcie_fpga_get_device_by_minor(iminor(inode));
  if (dev == NULL || dev->dev_id != iminor(inode)) {
    xpcie_err("xpcie_cdev_open error! NO DEVICE!");
    return -ENODEV;
  }

  // Initialize Data binded with file descriptor
  private = vmalloc(sizeof(struct xpcie_file_private));
  if (private == NULL) {
    xpcie_err("xpcie_cdev_open error! cannot allocate memory!");
    return -ENOMEM;
  }
  private->dev = dev;
  private->chid = -1;
  private->que_kind = -1;
  private->is_get_queue = false;
  private->is_valid_command = false;
#ifndef XPCIE_REGISTER_NO_LOCK
  private->is_avail_rw = false;
#else
  private->is_avail_rw = true;
#endif

  filp->private_data = private;

  return 0;
}


/**
 * @brief Function for close()
 */
static int
xpcie_cdev_release(struct inode *inode, struct file *filp)
{
  struct xpcie_file_private *private = filp->private_data;

  // When command queue is assigned, stop dma channel and put command queue
#if defined(ENABLE_MODULE_LLDMA)
  if (private->is_get_queue) {
    xpcie_fpga_stop_queue(private->dev, private->chid, private->que_kind);
    xpcie_fpga_put_queue_info(private->dev, private->chid, private->que_kind);
  }
#endif  // ENABLE_MODULE_LLDMA

  vfree(private);
  filp->private_data = NULL;

  return 0;
}


/**
 * @brief Function for read()
 */
static ssize_t
xpcie_cdev_read(struct file *filp, char *buf, size_t count, loff_t *f_pos)
{
  struct xpcie_file_private *private = filp->private_data;
  fpga_dev_info_t *dev = private->dev;
  uint64_t data;

  if (unlikely(!private->is_avail_rw)) {
    return -EBUSY;
  }

  if ((*f_pos % sizeof(uint32_t)) != 0) {
    return -EINVAL;
  }
  if (count == sizeof(uint32_t)) {
    data = reg_read32(dev, *f_pos);
  } else if (count == sizeof(uint64_t)) {
    data = reg_read64(dev, *f_pos);
  } else {
    return -EINVAL;
  }
  if (copy_to_user((void __user *)buf, &data, count)) {
    return -EFAULT;
  }

  return count;
}


/**
 * @brief Function for write()
 */
static ssize_t
xpcie_cdev_write(
  struct file *filp, const char *buf, size_t count, loff_t *f_pos)
{
  struct xpcie_file_private *private = filp->private_data;
  fpga_dev_info_t *dev = private->dev;
  uint64_t data;

  if (unlikely(!private->is_avail_rw)) {
    return -EBUSY;
  }

  if ((*f_pos % sizeof(uint32_t)) != 0 || count > sizeof(uint64_t)) {
    return -EINVAL;
  }
  if (copy_from_user(&data, (void __user *)buf, count)) {
    return -EFAULT;
  }
  if (count == sizeof(uint32_t)) {
    reg_write32(dev, *f_pos, (uint32_t)data);
  } else if (count == sizeof(uint64_t)) {
    reg_write32(dev, *f_pos + sizeof(uint32_t), (uint32_t)(data & 0xFFFFFFFF));
    reg_write32(dev, *f_pos, (uint32_t)((data >> 32) & 0xFFFFFFFF));
  } else {
    return -EINVAL;
  }

  return count;
}


static void
xpcie_vma_open(struct vm_area_struct *vma)
{
//  struct xpcie_file_private *private = vma->vm_private_data;
}


static void
xpcie_vma_close(struct vm_area_struct *vma)
{
//  struct xpcie_file_private *private = vma->vm_private_data;
  vma->vm_private_data = NULL;
}


#if (LINUX_VERSION_CODE < KERNEL_VERSION(5, 11, 0) || LINUX_VERSION_CODE >= KERNEL_VERSION(5, 13, 0))
static int xpcie_vma_mremap(struct vm_area_struct *vma)
#else
static int xpcie_vma_mremap(struct vm_area_struct *vma, unsigned long flags) // Only when 5.11/5.12
#endif
{
  xpcie_info("%s", __func__);
  return 0;
}


#if (LINUX_VERSION_CODE < KERNEL_VERSION(4, 17, 0))
static int xpcie_vma_fault(struct vm_fault *vmf)
#else
static vm_fault_t xpcie_vma_fault(struct vm_fault *vmf)
#endif
{
  struct vm_area_struct *vma = vmf->vma;
  struct xpcie_file_private *private = vma->vm_private_data;
  fpga_dev_info_t *dev = private->dev;
  struct page *page = NULL;
  unsigned long offset = vmf->pgoff << PAGE_SHIFT;

  xpcie_info("%s", __func__);
  switch (private->que_kind) {
  case DMA_HOST_TO_DEV:
    page = virt_to_page(dev->enqueues[private->chid].qp_mem_addr + offset);
    break;
  case DMA_DEV_TO_HOST:
    page = virt_to_page(dev->dequeues[private->chid].qp_mem_addr + offset);
    break;
  default:
    xpcie_warn("Invalid direction(%d)!", private->que_kind);
    return -EFAULT;
  }
  vmf->page = page;
  get_page(vmf->page);

  return 0;
}


/**
 * static global variable: Operations definition of this driver as vm
 */
static struct vm_operations_struct xpcie_remap_vm_ops = {
  .open   = xpcie_vma_open,
  .close  = xpcie_vma_close,
  .mremap = xpcie_vma_mremap,
  .fault  = xpcie_vma_fault,
};


/**
 * @brief Function for mmap()
 */
static int
xpcie_cdev_mmap(struct file *filp, struct vm_area_struct *vma)
{
  struct xpcie_file_private *private = filp->private_data;
  fpga_dev_info_t *dev = private->dev;
  ssize_t map_size;
  void *base_addr;
  int ret;

  // Fail when XPCIE_DEV_LLDMA_ALLOC_QUEUE or XPCIE_DEV_LLDMA_BIND_QUEUE
  //  is called by ioctl by the same file descriptor.
  if (private->chid < 0) {
    xpcie_warn("Queue not assigned!");
    return -ENODEV;
  }

  // Get command queue's address and size
  switch (private->que_kind) {
  case DMA_HOST_TO_DEV:
    base_addr = dev->enqueues[private->chid].que;
    map_size = dev->enqueues[private->chid].qp_mem_size;
    break;
  case DMA_DEV_TO_HOST:
    base_addr = dev->dequeues[private->chid].que;
    map_size = dev->dequeues[private->chid].qp_mem_size;
    break;
  default:
    xpcie_warn("Invalid direction(%d)!", private->que_kind);
    return -EFAULT;
  }
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(6, 3, 0))
  {
    vm_flags_t vm_flags;
    memset(&vm_flags, 0, sizeof(vm_flags));
    vm_flags |= VM_SHARED;
    vm_flags |= VM_IO;
    vm_flags_init(vma, vm_flags);
  }
#else
  vma->vm_flags |= VM_SHARED;
  vma->vm_flags |= VM_IO;
#endif

  // remap kernel memory to userspace
  ret = remap_pfn_range(vma, vma->vm_start,
        __phys_to_pfn(virt_to_phys(base_addr)),
        map_size, vma->vm_page_prot);
  if (ret) {
    xpcie_warn("mmap failed!");
    return -ENODEV;
  }

  vma->vm_private_data = (void *)private;
  vma->vm_ops = &xpcie_remap_vm_ops;
  xpcie_vma_open(vma);

#ifdef XPCIE_TRACE_LOG
  xpcie_info("%s: addr(%#llx), pfn(%#llx), size(%#lx)", __func__, (uint64_t)vma->vm_start,\
    (uint64_t)__phys_to_pfn(virt_to_phys(base_addr)), map_size);
#endif

  return 0;
}


/**
 * static global variable: Operations definition of this driver as character device
 */
static struct file_operations xpcie_cdev_fops = {
  .open    = xpcie_cdev_open,
  .release = xpcie_cdev_release,
  .read    = xpcie_cdev_read,
  .write   = xpcie_cdev_write,
  .mmap    = xpcie_cdev_mmap,
  .unlocked_ioctl = xpcie_cdev_ioctl,
};


struct file_operations* xpcie_fpga_get_cdev_fops(void)
{
  return &xpcie_cdev_fops;
}

