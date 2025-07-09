/*************************************************
* Copyright 2025 NTT Corporation
* Licensed under the GPL-2.0  License, see LICENSE for details.
* SPDX-License-Identifier: GPL-2.0-or-later
*************************************************/

#include <mem_manage.h>
#include <mem_manage_ioctl.h>
#include <linux/module.h>
#include <linux/version.h>
#include <linux/pci.h>
#ifdef AMD_PLATFORM
#include <linux/scatterlist.h>
#endif
#include <linux/device.h>
//#include <linux/slab.h>
//#include <linux/fs.h>
//#include <linux/io.h>
//#include <linux/uaccess.h>
//#include <linux/vmacache.h>
//#include <linux/sched/mm.h>
//#include <asm/current.h>
#ifdef NVIDIA_PLATFORM
#include <nv-p2p.h>
#endif
#ifdef AMD_PLATFORM
#include <amd_rdma.h>
#endif

#define MAX_CONT_NUM 4

#define NV_ALIGN_SIZE_LOG 8 // 256byte align
#define CPU_ALIGN_SIZE_LOG 12 // 4KB align

#define NV_BOUND_SHIFT   16
#define NV_BOUND_SIZE    ((u64)1 << NV_BOUND_SHIFT)
#define NV_BOUND_OFFSET  (NV_BOUND_SIZE-1)
#define NV_BOUND_MASK    (~NV_BOUND_OFFSET)

#define NVIDIA_VENDOR_ID 0x10de
#define NVIDIA_V100_ID 0x1db6

#define AMD_VENDOR_ID 0x1002
#define AMD_RADEON6600_XT_ID 0x73ff

#define PIN_USER_PAGES_KERNEL_VERSION (KERNEL_VERSION(5,6,0))
#define GUP_PIN_COUNTING_BIAS_KERNEL_VERSION (KERNEL_VERSION(5,7,0))

#define DEV_TYPE_UNKNOWN 0
#define DEV_TYPE_NV 1
#define DEV_TYPE_AMD 2
#define DEV_TYPE_CPU 3

typedef struct local_gpu_mem_list_ {
  struct list_head list_head;
  struct file* file;
  unsigned id;
  int ref_count;
} local_gpu_mem_list_t;

typedef struct physical_region_ {
  uint64_t addr;
  uint64_t size;
  uint64_t valid_size;
} physical_region_t;

typedef struct global_gpu_mem_list_ {
  struct list_head list_head;
  unsigned id;
  off_t offset;
  uint32_t dev_type;
#ifdef NVIDIA_PLATFORM
  struct nvidia_p2p_page_table* page_table;
#endif
#ifdef AMD_PLATFORM
  struct amd_p2p_info* p2p_info;
#endif
  physical_region_t *phys_region_ext;
  struct page **host_pages;
  unsigned long host_page_num;
  void* virt_addr;
  uint64_t phys_addrs[MAX_CONT_NUM];
  uint64_t phys_sizes[MAX_CONT_NUM];
  uint64_t phys_valid_sizes[MAX_CONT_NUM];
  uint32_t phys_num;
  uint64_t token;
  size_t size;
  size_t page_size;
  int ref_count;
  bool freed;
  int using;
  local_gpu_mem_list_t* lmem;
} global_gpu_mem_list_t;

typedef struct local_data_ {
  struct list_head gpu_mem_list;
  struct mutex mutex;
} local_data_t;

#if (LINUX_VERSION_CODE < KERNEL_VERSION(5, 0, 0))
typedef int vma_fault_ret_t;
#else
typedef vm_fault_t vma_fault_ret_t;
#endif

static DEFINE_MUTEX(global_mutex);
static LIST_HEAD(global_gpu_mem_list);
static unsigned g_map_id = 0;

#if DEBUG_LVL > 2
static int global_list_num = 0;
#endif

static int free_mem(global_gpu_mem_list_t* gpu_mem, bool self);
static int free_mem_by_id(unsigned id, bool self);
static int delete_gpu_lmem(local_gpu_mem_list_t* lmem);
static int delete_mem_core(global_gpu_mem_list_t* gpu_mem);

int mem_manage_open(struct inode *inode, struct file *file) {
  local_data_t *data = NULL;
#if DEBUG_LVL > 1
  printk("mem manage open\n");
#endif

  data = kzalloc(sizeof(local_data_t), GFP_KERNEL);
  if (data == NULL) {
#if DEBUG_LVL > 0
    printk(KERN_ERR "open: kzalloc\n");
#endif
    return -ENOMEM;
  }
  INIT_LIST_HEAD(&data->gpu_mem_list);
  mutex_init(&data->mutex);

  file->private_data = data;

  return 0;
}

int mem_manage_close(struct inode *inode, struct file *file) {
  if (file->private_data) {
    local_data_t *data = (local_data_t*)(file->private_data);
    while (!list_empty(&data->gpu_mem_list)) {
      int ret;
      local_gpu_mem_list_t *head;
      head = list_first_entry(&data->gpu_mem_list, local_gpu_mem_list_t, list_head);
#if DEBUG_LVL > 1
      printk("NV: close %u: ref_count %d\n", head->id, head->ref_count);
#endif
      ret = free_mem_by_id(head->id, true);
      if (ret) {
#if DEBUG_LVL > 0
        printk("failed to release nv mem: %d\n", head->id);
#endif
        break;
      }
      delete_gpu_lmem(head);
    }
    if (list_empty(&data->gpu_mem_list)) {
      kfree(file->private_data);
    }
  }
#if DEBUG_LVL > 1
  printk("mem manage close\n");
#endif
  return 0;
}

long mem_manage_ioctl(struct file *filp, unsigned int cmd, unsigned long arg) {
  switch (cmd) {
#ifdef NVIDIA_PLATFORM
  case (MEM_MANAGE_IOCTL_GET_NV_DEV_ADDR):
    return mem_manage_get_nv_dev_addr(filp, (void __user *)arg);
  case (MEM_MANAGE_IOCTL_PUT_NV_DEV_ADDR):
    return mem_manage_put_nv_dev_addr((void __user *)arg);
#endif
#ifdef AMD_PLATFORM
  case (MEM_MANAGE_IOCTL_GET_AMD_DEV_ADDR):
    return mem_manage_get_amd_dev_addr(filp, (void __user *)arg);
  case (MEM_MANAGE_IOCTL_PUT_AMD_DEV_ADDR):
    return mem_manage_put_amd_dev_addr((void __user *)arg);
#endif
  case (MEM_MANAGE_IOCTL_GET_HOST_PHYS_ADDR):
    return mem_manage_get_host_paddr(filp, (void __user *)arg);
  case (MEM_MANAGE_IOCTL_PUT_HOST_PHYS_ADDR):
    return mem_manage_put_host_paddr((void __user *)arg);

  case (MEM_MANAGE_IOCTL_GET_PADDR_NUM):
    return mem_manage_get_paddr_num((void __user *)arg);
  case (MEM_MANAGE_IOCTL_GET_PADDR):
    return mem_manage_get_paddr((void __user *)arg);
  case (MEM_MANAGE_IOCTL_GET_DEV_TYPE):
    return mem_manage_get_dev_type((void __user *)arg);
  default:
    break;
  }
  return STATUS_ERROR;
}

int mem_manage_mmap(struct file *file, struct vm_area_struct *vma) {
  int ret;
  local_data_t *data;
  if (!file->private_data) {
    return -ENOMEM;
  }
  data = (local_data_t*)(file->private_data);
#if (LINUX_VERSION_CODE < KERNEL_VERSION(6,3,0))
  vma->vm_flags |= VM_SHARED;
#else
  vm_flags_set(vma, VM_SHARED);
#endif

  ret = mem_manage_map_gpu_dev_addr(file, vma);
  if (ret != STATUS_NOT_FOUND) return ret;

  return ret;
}

static void mem_manage_gpu_vma_open(struct vm_area_struct *vma) {
  local_gpu_mem_list_t* lmem;
  lmem = (local_gpu_mem_list_t*)vma->vm_private_data;
  if (lmem) {
    ++lmem->ref_count;
  }
#if DEBUG_LVL > 1
  printk("GPU: vma_open (%u), %u, %llx local ref++ %d\n", lmem->ref_count, lmem->id,
         virt_to_phys((void*)(vma->vm_start)), lmem->ref_count);
#endif
}

static void mem_manage_gpu_vma_close(struct vm_area_struct *vma) {
  local_gpu_mem_list_t* lmem;
  lmem = (local_gpu_mem_list_t*)vma->vm_private_data;
  if (!lmem) return;

  --lmem->ref_count;
#if DEBUG_LVL > 1
  printk("GPU: vma_close (%u), %u: virt %lx phys %llx %lx local ref-- %d\n",
         lmem->ref_count, lmem->id, vma->vm_start,
         virt_to_phys((void*)(vma->vm_start)), vma->vm_pgoff << PAGE_SHIFT, lmem->ref_count);
#endif
  if (lmem->ref_count) return;

  mem_manage_unmap_gpu_dev_addr(vma);
}

static int mem_manage_gpu_vma_remap(struct vm_area_struct *vma) {
#if DEBUG_LVL > 1
  printk("GPU: vma_remap\n");
#endif
  return STATUS_SUCCESS;
}

static vma_fault_ret_t mem_manage_gpu_vma_fault(struct vm_fault *vmf) {
#if DEBUG_LVL > 1
  printk("GPU: vma_fault: flag: %x, pgoff: %lx, address: %lx, pte: %llx, page: %llx\n",
         vmf->flags, vmf->pgoff, vmf->address, (uint64_t)(vmf->pte),
         (uint64_t)(vmf->page));
#endif
  return VM_FAULT_NOPAGE;
}

static struct vm_operations_struct vm_gpu_ops = {
  .open = mem_manage_gpu_vma_open,
  .close = mem_manage_gpu_vma_close,
  .mremap = mem_manage_gpu_vma_remap,
  .fault = mem_manage_gpu_vma_fault,
};

static int release_mem(global_gpu_mem_list_t* gpu_mem) {
#if DEBUG_LVL > 2
  printk("release_mem: %llx %lx: using: %d, ref_count: %d\n",
         (uint64_t)gpu_mem->virt_addr, gpu_mem->size, gpu_mem->using, gpu_mem->ref_count);
#endif
  mutex_lock(&global_mutex);
  gpu_mem->using--;
  if (gpu_mem->using == 0 && gpu_mem->ref_count == 0) {
    delete_mem_core(gpu_mem);
  }
  mutex_unlock(&global_mutex);
  return STATUS_SUCCESS;
}

static int add_gpu_mem(global_gpu_mem_list_t* gpu_mem) {
  mutex_lock(&global_mutex);
  if (!list_empty(&global_gpu_mem_list)) {
    global_gpu_mem_list_t *head;
    head = list_first_entry(&global_gpu_mem_list,
                            global_gpu_mem_list_t, list_head);
    if (head->id == g_map_id) {
      mutex_unlock(&global_mutex);
      return STATUS_ERROR;
    }
  }
  gpu_mem->id = g_map_id++;
  if (gpu_mem->lmem) gpu_mem->lmem->id = gpu_mem->id;
  list_add_tail(&gpu_mem->list_head, &global_gpu_mem_list);
#if DEBUG_LVL > 2
  global_list_num++;
  printk("add_gpu_mem: %d\n", global_list_num);
#endif
  gpu_mem->using++;
  mutex_unlock(&global_mutex);
  return STATUS_SUCCESS;
}

static global_gpu_mem_list_t* find_mem_by_id(unsigned id) {
  global_gpu_mem_list_t *list = NULL;
  mutex_lock(&global_mutex);
  list_for_each_entry(list, &global_gpu_mem_list, list_head) {
    if (list->id == id) {
      list->using++;
      mutex_unlock(&global_mutex);
      return list;
    }
  }
  mutex_unlock(&global_mutex);
  return NULL;
}

static global_gpu_mem_list_t* find_mem_by_token(uint64_t token) {
  global_gpu_mem_list_t *list = NULL;
  mutex_lock(&global_mutex);
  list_for_each_entry(list, &global_gpu_mem_list, list_head) {
    if (list->token == token) {
      list->using++;
      mutex_unlock(&global_mutex);
      return list;
    }
  }
  mutex_unlock(&global_mutex);
  return NULL;
}

static int delete_mem(unsigned id, bool force) {
  int ret = STATUS_SUCCESS;
  global_gpu_mem_list_t *gpu_mem;
  gpu_mem = find_mem_by_id(id);
#if DEBUG_LVL > 1
  if (gpu_mem) {
    printk("delete gpu mem: %u %lx %llx global ref %d\n",
           gpu_mem->id, (uintptr_t)gpu_mem->virt_addr,
           gpu_mem->token, gpu_mem->ref_count);
  }
#endif
  if (gpu_mem) {
    local_gpu_mem_list_t* lmem = NULL;

    mutex_lock(&global_mutex);
    if (gpu_mem->lmem) {
      lmem = gpu_mem->lmem;
      gpu_mem->lmem = NULL;
    }
    mutex_unlock(&global_mutex);

    if (lmem) ret = delete_gpu_lmem(lmem);
    if (!ret) ret = free_mem(gpu_mem, false);
  }

  return ret;
}

static global_gpu_mem_list_t* find_mem_by_ptr(void* ptr) {
  global_gpu_mem_list_t *list = NULL, *ret = NULL;
  mutex_lock(&global_mutex);
  list_for_each_entry(list, &global_gpu_mem_list, list_head) {
    if ((uintptr_t)list == (uintptr_t)ptr) {
      ret = list;
      ret->using++;
      break;
    }
  }
  mutex_unlock(&global_mutex);
  return ret;
}

static global_gpu_mem_list_t* find_mem(uint64_t pgoff, size_t size) {
  global_gpu_mem_list_t *list = NULL, *ret = NULL;
  mutex_lock(&global_mutex);
  list_for_each_entry(list, &global_gpu_mem_list, list_head) {
    size_t page_offset = list->offset & (PAGE_SIZE - 1);
    size_t aligned_size = (list->size + page_offset + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);
#if DEBUG_LVL > 1
    uint32_t i;
#ifdef NVIDIA_PLATFORM
    if (list->page_table) {
      printk("NV: [%u] token: %llx, size: %lx\n", list->id, list->token, list->size);
      for (i = 0; i<list->phys_num; i++) {
        printk("        [%u]: paddr: %llx, size: %llx\n", i,
               list->phys_addrs[i] + (i==0 ? list->offset : 0), list->phys_sizes[i]);
      }
    }
#endif
#ifdef AMD_PLATFORM
    if (list->p2p_info) {
      printk("AMD: [%u] token: %llx, size: %lx\n", list->id, list->token, list->size);
      for (i = 0; i<list->phys_num; i++) {
        printk("        [%u]: paddr: %llx, size: %llx\n", i,
               list->phys_addrs[i] + (i==0 ? list->offset : 0), list->phys_sizes[i]);
      }
    }
#endif
    if (list->dev_type == DEV_TYPE_CPU) {
      printk("CPU: [%u] token: %llx, size: %lx\n", list->id, list->token, list->size);
      for (i = 0; i<list->phys_num; i++) {
        if (i<MAX_CONT_NUM) {
          printk("        [%u]: paddr: %llx, size: %llx\n", i,
                 list->phys_addrs[i] + (i==0 ? list->offset : 0), list->phys_sizes[i]);
        } else {
          physical_region_t* region = list->phys_region_ext + i - MAX_CONT_NUM;
          printk("        [%u]: paddr: %llx, size: %llx\n", i,
                 region->addr, region->size);
        }
      }
    }
#endif
    if (list->token == pgoff && aligned_size == size) {
      ret = list;
      ret->using++;
      break;
    }
  }
  mutex_unlock(&global_mutex);
  return ret;
}

static global_gpu_mem_list_t* alloc_mem(uint64_t pgoff, size_t size) {
  global_gpu_mem_list_t *ret;
  ret = find_mem(pgoff, size);
  mutex_lock(&global_mutex);
  if (ret) {
    ++ret->ref_count;
#if DEBUG_LVL > 1
    printk("alloc gpu mem: %lx %llx global ref++ %d\n",
           (uintptr_t)ret->virt_addr, ret->token, ret->ref_count);
#endif
  }
  mutex_unlock(&global_mutex);
  return ret;
}

static int free_mem(global_gpu_mem_list_t* gpu_mem, bool self) {
  mutex_lock(&global_mutex);
  if (gpu_mem->ref_count == 0) {
#if DEBUG_LVL > 1
#ifdef NVIDIA_PLATFORM
    if (gpu_mem->page_table) {
      printk("NV: global mem ref count is already 0.\n");
    }
#endif
#ifdef AMD_PLATFORM
    if (gpu_mem->p2p_info) {
      printk("AMD: global mem ref count is already 0.\n");
    }
#endif
    if (gpu_mem->dev_type == DEV_TYPE_CPU && gpu_mem->phys_num > 0) {
      printk("CPU: global mem ref count is already 0.\n");
    }
#endif
  } else {
    gpu_mem->ref_count--;
    if (self) {
#if DEBUG_LVL > 2
      printk("set freed: %llx %lx\n", (uint64_t)gpu_mem->virt_addr, gpu_mem->size);
#endif
      gpu_mem->freed = true;
    }
  }
  mutex_unlock(&global_mutex);
  return release_mem(gpu_mem);
}

static int delete_mem_core(global_gpu_mem_list_t* gpu_mem) {
#if (DEBUG_LVL > 1) || !(LINUX_VERSION_CODE < GUP_PIN_COUNTING_BIAS_KERNEL_VERSION)
  uint32_t i;
#endif
  if (gpu_mem->ref_count == 0) {
#ifdef NVIDIA_PLATFORM
    if (gpu_mem->page_table && !gpu_mem->freed) {
      nvidia_p2p_put_pages(0, 0, (uint64_t)gpu_mem->virt_addr, gpu_mem->page_table);
      gpu_mem->page_table = NULL;
#if DEBUG_LVL > 1
      for (i = 0; i<gpu_mem->phys_num; i++) {
        printk("NV: id: %u paddr[%u]: %llx delete global ref-- %d\n", gpu_mem->id, i,
               gpu_mem->phys_addrs[i] + (i==0 ? gpu_mem->offset : 0), gpu_mem->ref_count);
      }
#endif
    }
#endif
#ifdef AMD_PLATFORM
    if (gpu_mem->p2p_info && !gpu_mem->freed) {
      rdma_ops->put_pages(&gpu_mem->p2p_info);
#if DEBUG_LVL > 1
      for (i = 0; i<gpu_mem->phys_num; i++) {
        printk("AMD: id: %u paddr[%u]: %llx delete global ref-- %d\n", gpu_mem->id, i,
               gpu_mem->phys_addrs[i] + (i==0 ? gpu_mem->offset : 0), gpu_mem->ref_count);
      }
#endif
      gpu_mem->p2p_info = NULL;
    }
#endif
    if (gpu_mem->dev_type == DEV_TYPE_CPU && gpu_mem->phys_num > 0) {
#if DEBUG_LVL > 1
      printk("unpinning [pid = %d] 0x%lx (start=0x%lx), page_num(%d) ref count = %d\n", current->pid, gpu_mem->virt_addr, (uint64_t)gpu_mem->virt_addr & ~4095, gpu_mem->host_page_num, page_ref_count(compound_head(gpu_mem->host_pages[0])));
#endif
#if (LINUX_VERSION_CODE < PIN_USER_PAGES_KERNEL_VERSION)
      put_user_pages(gpu_mem->host_pages, gpu_mem->host_page_num);
#else
#if (LINUX_VERSION_CODE < GUP_PIN_COUNTING_BIAS_KERNEL_VERSION)
      unpin_user_pages(gpu_mem->host_pages, gpu_mem->host_page_num);
#else
      if (page_ref_count(compound_head(gpu_mem->host_pages[0])) < GUP_PIN_COUNTING_BIAS) {
        for (i = 0; i < gpu_mem->host_page_num; i++) {
          put_page(gpu_mem->host_pages[i]);
        }
      } else {
        unpin_user_pages(gpu_mem->host_pages, gpu_mem->host_page_num);
      }
#endif
#endif
#if DEBUG_LVL > 1
      printk("unpinned [pid = %d] 0x%lx (start=0x%lx), page_num(%d) ref count = %d\n", current->pid, gpu_mem->virt_addr, (uint64_t)gpu_mem->virt_addr & ~4095, gpu_mem->host_page_num, page_ref_count(compound_head(gpu_mem->host_pages[0])));
#endif

      vfree(gpu_mem->host_pages);
#if DEBUG_LVL > 1
      for (i = 0; i<gpu_mem->phys_num; i++) {
        if (i< MAX_CONT_NUM) {
          printk("CPU: id: %u paddr[%u]: %llx delete global ref-- %d\n", gpu_mem->id, i,
                 gpu_mem->phys_addrs[i] + (i==0 ? gpu_mem->offset : 0), gpu_mem->ref_count);
        } else {
          physical_region_t* region = gpu_mem->phys_region_ext + i - MAX_CONT_NUM;
          printk("CPU: id: %u paddr[%u]: %llx delete global ref-- %d\n", gpu_mem->id, i,
                 region->addr, gpu_mem->ref_count);
        }
      }
#endif
      if (gpu_mem->phys_num > MAX_CONT_NUM) {
        vfree(gpu_mem->phys_region_ext);
      }
    }
    list_del(&gpu_mem->list_head);
    vfree(gpu_mem);
#if DEBUG_LVL > 2
    global_list_num--;
    printk("release_gpu_mem: %d\n", global_list_num);
#endif
  }
  return STATUS_SUCCESS;
}

static int free_mem_by_id(unsigned id, bool self) {
  int ret = STATUS_NOT_FOUND;
  global_gpu_mem_list_t *gpu_mem = NULL;
  gpu_mem = find_mem_by_id(id);
  if (gpu_mem) {
    ret = free_mem(gpu_mem, self);
  }
  return ret;
}

static int add_gpu_lmem(local_gpu_mem_list_t* lmem) {
  local_data_t* data;
  if (!lmem->file) return STATUS_ERROR;
  data = (local_data_t*)lmem->file->private_data;
  if (!data) return STATUS_ERROR;

  mutex_lock(&data->mutex);
  list_add_tail(&lmem->list_head, &data->gpu_mem_list);
  mutex_unlock(&data->mutex);
  return STATUS_SUCCESS;
}

static int delete_gpu_lmem(local_gpu_mem_list_t* lmem) {
  local_data_t* data;
  if (!lmem->file) return STATUS_ERROR;
  data = (local_data_t*)lmem->file->private_data;
  if (!data) return STATUS_ERROR;

  mutex_lock(&data->mutex);
  list_del(&lmem->list_head);
  vfree(lmem);
  mutex_unlock(&data->mutex);
  return STATUS_SUCCESS;
}

static int create_base_local_mem(global_gpu_mem_list_t* gpu_mem, struct file* filp) {
  local_gpu_mem_list_t* lmem;

  lmem = (local_gpu_mem_list_t*)vmalloc(sizeof(local_gpu_mem_list_t));
  if (!lmem) {
    return STATUS_ERROR;
  }
  memset(lmem, 0, sizeof(local_gpu_mem_list_t));
  lmem->id = gpu_mem->id;
  lmem->ref_count = 1;
  lmem->file = filp;
  if (add_gpu_lmem(lmem)) {
    vfree(lmem);
    return STATUS_ERROR;
  }

  gpu_mem->lmem = lmem;
  gpu_mem->ref_count++;

  return STATUS_SUCCESS;
}

#ifdef NVIDIA_PLATFORM
static void mem_manage_nv_free_callback(void* data) {
#if DEBUG_LVL > 1
  uint32_t i;
#endif
  global_gpu_mem_list_t* gpu_mem;
  struct nvidia_p2p_page_table* page_table = NULL;

  if (!find_mem_by_ptr(data)) return;
  gpu_mem = (global_gpu_mem_list_t*)data;

  mutex_lock(&global_mutex);
  if (gpu_mem->page_table) {
    page_table = gpu_mem->page_table;
    gpu_mem->page_table = NULL;
  }
  mutex_unlock(&global_mutex);

#if DEBUG_LVL > 1
  for (i = 0; i<gpu_mem->phys_num; i++) {
    printk("NV: paddr[%u]: %llx, size: %llx free ref_count: %d\n", i,
           gpu_mem->phys_addrs[i] + (i==0 ? gpu_mem->offset : 0),
           gpu_mem->phys_sizes[i], gpu_mem->ref_count);
  }
#endif
  if (page_table) nvidia_p2p_free_page_table(page_table);
  release_mem(gpu_mem);
}
#endif

#ifdef AMD_PLATFORM
static void mem_manage_amd_free_callback(void* data) {
#if DEBUG_LVL > 1
  uint32_t i;
#endif
  global_gpu_mem_list_t* gpu_mem;
  struct amd_p2p_info* p2p_info = NULL;

  if (!find_mem_by_ptr(data)) return;
  gpu_mem = (global_gpu_mem_list_t*)data;

  mutex_lock(&global_mutex);
  if (gpu_mem->p2p_info) {
    p2p_info = gpu_mem->p2p_info;
    gpu_mem->p2p_info = NULL;
  }
  mutex_unlock(&global_mutex);

#if DEBUG_LVL > 1
  for (i = 0; i<gpu_mem->phys_num; i++) {
    printk("AMD: paddr[%u]: %llx, size: %lx free\n", i,
           gpu_mem->phys_addrs[i] + (i==0 ? gpu_mem->offset : 0), gpu_mem->phys_sizes[i]);
  }
#endif
  if (p2p_info) rdma_ops->put_pages(&p2p_info);
  release_mem(gpu_mem);
}
#endif

#ifdef NVIDIA_PLATFORM
long mem_manage_get_nv_dev_addr(struct file* filp, void __user* arg) {
  mem_manage_nv_addr info;
  uint64_t vstart;
  size_t pin_size;
  uint64_t paddr = 0;
  uint64_t paddr_1st = 0;
  uint64_t paddr_next = 0;
  global_gpu_mem_list_t* gpu_mem;
  global_gpu_mem_list_t* gpu_mem_prev;
  unsigned i;
  int ret;
  uint64_t pofs = 0;
  uint64_t psize = 0;
  uint64_t remain_size = 0;
  if (copy_from_user(&info, arg, sizeof(mem_manage_nv_addr)) != 0) {
    return STATUS_ERROR;
  }
  if (!info.dev_addr || !info.size) return STATUS_ERROR;

  gpu_mem = (global_gpu_mem_list_t*)vmalloc(sizeof(global_gpu_mem_list_t));
  if (!gpu_mem) return STATUS_ERROR;

  memset(gpu_mem, 0, sizeof(global_gpu_mem_list_t));
  vstart = (uint64_t)info.dev_addr & NV_BOUND_MASK;
  pin_size = (uint64_t)info.dev_addr + info.size - vstart;
  gpu_mem->virt_addr = info.dev_addr;
  gpu_mem->size = info.size;
  gpu_mem->offset = (uint64_t)info.dev_addr - vstart;
#if DEBUG_LVL > 1
  printk("vaddr: %llx, vstart: %llx, offset: %lx\n",
         (uint64_t)info.dev_addr, vstart, gpu_mem->offset);
#endif
  ret = nvidia_p2p_get_pages(0, 0, vstart, pin_size, &gpu_mem->page_table,
                             mem_manage_nv_free_callback, gpu_mem);
  if (ret) {
    printk("p2p get pages failed: %d\n", ret);
    if (copy_to_user(arg, &info, sizeof(mem_manage_nv_addr)) != 0) {
      printk("copy_to_user: failed\n");
    }
    vfree(gpu_mem);
    return STATUS_FAILED;
  }
  if (gpu_mem->page_table->entries == 0) {
    vfree(gpu_mem);
    return STATUS_ERROR;
  }

  switch ((enum nvidia_p2p_page_size_type)gpu_mem->page_table->page_size) {
  case NVIDIA_P2P_PAGE_SIZE_4KB:
    gpu_mem->page_size = 4096;
    break;
  case NVIDIA_P2P_PAGE_SIZE_64KB:
    gpu_mem->page_size = 65536;
    break;
  case NVIDIA_P2P_PAGE_SIZE_128KB:
    gpu_mem->page_size = 131072;
    break;
  case NVIDIA_P2P_PAGE_SIZE_COUNT:
    // no break
  default:
#if DEBUG_LVL > 0
    printk("cannot get nvidia page size: %d\n", gpu_mem->page_table->page_size);
#endif
    nvidia_p2p_put_pages(0, 0, (uint64_t)info.dev_addr, gpu_mem->page_table);
    vfree(gpu_mem);
    return STATUS_ERROR;
  }
  gpu_mem->phys_num = 0;
  pofs = gpu_mem->offset;
  remain_size = info.size;
  for (i=0; i<gpu_mem->page_table->entries; ++i) {
    if (paddr == 0) {
      paddr = gpu_mem->page_table->pages[i]->physical_address;
      paddr_1st = paddr;
      psize = gpu_mem->page_size;
      paddr_next = paddr + gpu_mem->page_size;
    } else if (gpu_mem->page_table->pages[i]->physical_address != paddr_next) {
      gpu_mem->phys_sizes[gpu_mem->phys_num] = psize;
      gpu_mem->phys_valid_sizes[gpu_mem->phys_num] = psize - pofs;
      gpu_mem->phys_addrs[gpu_mem->phys_num++] = paddr;
      remain_size -= psize - pofs;
#if DEBUG_LVL > 1
      printk(" [%2d]: paddr %llx, %llx, psize %llx, %llx\n", i, paddr, paddr + pofs, psize, psize - pofs);
#endif
      pofs = 0;
      if (gpu_mem->phys_num >= MAX_CONT_NUM) {
        nvidia_p2p_put_pages(0, 0, (uint64_t)info.dev_addr, gpu_mem->page_table);
#if DEBUG_LVL > 0
        printk("paddrs are not contiguous much.");
#endif
        return STATUS_ERROR;
      }
      paddr = gpu_mem->page_table->pages[i]->physical_address;
      paddr_next = paddr + gpu_mem->page_size;
      psize = gpu_mem->page_size;
    } else {
      psize += gpu_mem->page_size;
      paddr_next += gpu_mem->page_size;
    }
  }
  info.token = (paddr_1st + gpu_mem->offset) >> NV_ALIGN_SIZE_LOG;

  gpu_mem_prev = find_mem_by_token(info.token);
  if (gpu_mem_prev) {
    release_mem(gpu_mem_prev);
    nvidia_p2p_put_pages(0, 0, (uint64_t)info.dev_addr, gpu_mem->page_table);
    vfree(gpu_mem);
    return STATUS_ERROR;
  }

  gpu_mem->phys_sizes[gpu_mem->phys_num] = psize;
  if (psize > remain_size) psize = remain_size;
  gpu_mem->phys_valid_sizes[gpu_mem->phys_num] = psize;
  gpu_mem->phys_addrs[gpu_mem->phys_num++] = paddr;
#if DEBUG_LVL > 1
  printk(" [%2u]: paddr %llx, %llx, psize %llx, %llx\n",
         gpu_mem->page_table->entries-1, paddr, paddr + pofs,
         gpu_mem->phys_sizes[gpu_mem->phys_num-1], psize);
#endif
  gpu_mem->token = info.token;
  gpu_mem->dev_type = DEV_TYPE_NV;

  if (create_base_local_mem(gpu_mem, filp)) {
    nvidia_p2p_put_pages(0, 0, (uint64_t)info.dev_addr, gpu_mem->page_table);
    vfree(gpu_mem);
    return STATUS_ERROR;
  }

  if (add_gpu_mem(gpu_mem)) {
    nvidia_p2p_put_pages(0, 0, (uint64_t)info.dev_addr, gpu_mem->page_table);
#if DEBUG_LVL > 0
    printk("failed to add gpu mem: %lx %llx\n", (uintptr_t)info.dev_addr, info.token);
#endif
    vfree(gpu_mem);
    return STATUS_ERROR;
  }
  info.id = gpu_mem->id;

  release_mem(gpu_mem);

  if (copy_to_user(arg, &info, sizeof(mem_manage_nv_addr)) != 0) {
    return STATUS_ERROR;
  }
  return STATUS_SUCCESS;
}
#endif

#ifdef AMD_PLATFORM
long mem_manage_get_amd_dev_addr(struct file* filp, void __user* arg) {
  mem_manage_amd_addr info;
  uint64_t vstart;
  size_t pin_size;
  uint64_t paddr = 0;
  uint64_t paddr_next = 0;
  global_gpu_mem_list_t* gpu_mem;
  struct pci_dev *gpu_dev = NULL;
  struct scatterlist *sg;
  unsigned i;
  int ret;
  if (copy_from_user(&info, arg, sizeof(mem_manage_amd_addr)) != 0) {
    return STATUS_ERROR;
  }
  if (!info.dev_addr || !info.size) return STATUS_ERROR;

  gpu_mem = (global_gpu_mem_list_t*)vmalloc(sizeof(global_gpu_mem_list_t));
  if (!gpu_mem) return STATUS_ERROR;

  memset(gpu_mem, 0, sizeof(global_gpu_mem_list_t));
  vstart = (uint64_t)info.dev_addr;
  pin_size = info.size;
  gpu_mem->virt_addr = info.dev_addr;
  gpu_mem->size = info.size;
  gpu_mem->offset = (uint64_t)info.dev_addr - vstart;
#if DEBUG_LVL > 1
  printk("vaddr: %llx, vstart: %llx, offset: %lx\n",
         (uint64_t)info.dev_addr, vstart, gpu_mem->offset);
#endif
  gpu_dev = pci_get_device(AMD_VENDOR_ID, AMD_RADEON6600_XT_ID, NULL);
  if (!rdma_ops || !gpu_dev) {
#if DEBUG_LVL > 1
    printk("rdma_ops is empty\n");
#endif
    if (copy_to_user(arg, &info, sizeof(mem_manage_amd_addr)) != 0) {
      printk("copy_to_user: failed\n");
    }
    vfree(gpu_mem);
    return STATUS_FAILED;
  }
  ret = rdma_ops->get_pages(vstart, pin_size, NULL, &(gpu_dev->dev), &gpu_mem->p2p_info,
			    mem_manage_amd_free_callback, gpu_mem);
  if (ret) {
    if (copy_to_user(arg, &info, sizeof(mem_manage_amd_addr)) != 0) {
      printk("copy_to_user: failed\n");
    }
    vfree(gpu_mem);
    return STATUS_FAILED;
  }
  if (gpu_mem->p2p_info->pages->nents == 0) {
    vfree(gpu_mem);
    return STATUS_ERROR;
  }

  if (rdma_ops->get_page_size(vstart, pin_size, NULL, &gpu_mem->page_size) != 0){
#if DEBUG_LVL > 0
    printk("cannot get amd page size\n");
#endif
    return STATUS_ERROR;
  }
  sg = gpu_mem->p2p_info->pages->sgl;
  for (i = 0; i < gpu_mem->p2p_info->pages->nents; i++, sg = sg_next(sg)) {
    if (paddr == 0){
      paddr = sg->dma_address + sg->offset;
      paddr_next = paddr + sg->offset + sg->length;
    } else if (paddr_next != sg->dma_address) {
#if DEBUG_LVL > 0
      printk("paddrs are not contiguous.");
#endif
      return STATUS_ERROR;
    } else {
      paddr_next += sg->length + sg->offset;
    }
  }
  gpu_mem->phys_addrs[0] = paddr;
  info.token = paddr >> NV_ALIGN_SIZE_LOG;
  gpu_mem->token = info.token;
  gpu_mem->dev_type = DEV_TYPE_AMD;

  if (create_base_local_mem(gpu_mem, filp)) {
    rdma_ops->put_pages(&gpu_mem->p2p_info);
    vfree(gpu_mem);
    return STATUS_ERROR;
  }
  if (add_gpu_mem(gpu_mem)) {
    rdma_ops->put_pages(&gpu_mem->p2p_info);
    vfree(gpu_mem);
    return STATUS_ERROR;
  }
  info.id = gpu_mem->id;

  release_mem(gpu_mem);

  if (copy_to_user(arg, &info, sizeof(mem_manage_amd_addr)) != 0) {
    return STATUS_ERROR;
  }
  return STATUS_SUCCESS;
}
#endif

long mem_manage_get_host_paddr(struct file* filp, void __user *arg) {
  mem_manage_host_addr info;

  global_gpu_mem_list_t *gpu_mem;

  int pinned_pages;
  uint64_t current_addr;
  uint64_t start_addr;
  uint64_t end_addr;

  unsigned int gup_flags;
  unsigned long nr_pages;
  unsigned int cont_pages;
  unsigned int remain_size;

  int i;
  int ret;
  phys_addr_t paddr;
  phys_addr_t paddr_1st;
  uint64_t paddr_next;

  if (copy_from_user(&info, arg, sizeof(mem_manage_host_addr)) != 0)
    return STATUS_ERROR;
  if (!info.dev_addr || !info.size)
    return STATUS_ERROR;

  gpu_mem = (global_gpu_mem_list_t *)vmalloc(sizeof(global_gpu_mem_list_t));
  if (!gpu_mem)
    return STATUS_ERROR;

  memset(gpu_mem, 0, sizeof(global_gpu_mem_list_t));
  pinned_pages = 0;
  gpu_mem->dev_type = DEV_TYPE_CPU;
  gpu_mem->virt_addr = (void *)info.dev_addr;
  gpu_mem->offset = (uint64_t)gpu_mem->virt_addr & 4095;
  start_addr = (uint64_t)gpu_mem->virt_addr & ~4095;
  end_addr = (uint64_t)gpu_mem->virt_addr + info.size;
  gpu_mem->size = info.size;
  gpu_mem->page_size = PAGE_SIZE;
  gpu_mem->host_pages = NULL;

  gup_flags = info.force ? FOLL_WRITE | FOLL_FORCE : FOLL_WRITE;
  nr_pages = 1 + (end_addr - start_addr - 1) / PAGE_SIZE;
#if DEBUG_LVL > 1
  printk("vaddr: %llx, vstart: %llx, offset: %lx, pages: %lx\n",
         (uint64_t)info.dev_addr, start_addr, gpu_mem->offset, nr_pages);
#endif
  gpu_mem->host_pages = (struct page**)vmalloc(sizeof(struct page*) * nr_pages);
  gpu_mem->host_page_num = nr_pages;
  current_addr = start_addr;
#if (LINUX_VERSION_CODE < PIN_USER_PAGES_KERNEL_VERSION)
  ret = get_user_pages(current_addr, nr_pages, gup_flags, gpu_mem->host_pages, NULL);
#else
#if (LINUX_VERSION_CODE < KERNEL_VERSION(6,5,0))
  ret = pin_user_pages(current_addr, nr_pages, gup_flags, gpu_mem->host_pages, NULL);
#else
  ret = pin_user_pages(current_addr, nr_pages, gup_flags, gpu_mem->host_pages);
#endif
#endif
  if (ret < 0) {
#if DEBUG_LVL > 0
#if (LINUX_VERSION_CODE < PIN_USER_PAGES_KERNEL_VERSION)
    printk("get_user_pages failed: %d\n", ret);
#else
    printk("pin_user_pages failed: %d\n", ret);
#endif
#endif
    vfree(gpu_mem);
    return STATUS_ERROR;
  }
#if DEBUG_LVL > 1
  printk("pinned [pid = %d] 0x%lx (start=0x%lx), page_num(%d) ref count = %d\n", current->pid, gpu_mem->virt_addr, (uint64_t)gpu_mem->virt_addr & ~4095, gpu_mem->host_page_num, page_ref_count(compound_head(gpu_mem->host_pages[0])));
#endif

  paddr_1st = page_to_phys(gpu_mem->host_pages[0]);
  paddr_next = 0;
  cont_pages = 0;
  for (i=0; i<nr_pages; i++) {
    paddr = page_to_phys(gpu_mem->host_pages[i]);
    if (paddr_next == 0 || (uint64_t)paddr != paddr_next) {
      if (paddr_next) cont_pages++;
      paddr_next = (uint64_t)paddr + PAGE_SIZE;
    } else {
      paddr_next += PAGE_SIZE;
    }
  }
  if (paddr_next) cont_pages++;
#if DEBUG_LVL > 1
  printk("contiguous pages: %d\n", cont_pages);
#endif
  if (cont_pages > MAX_CONT_NUM) {
    gpu_mem->phys_region_ext =
        (physical_region_t*)vmalloc(sizeof(physical_region_t)*(cont_pages-MAX_CONT_NUM));
    if (!gpu_mem->phys_region_ext) {
#if (LINUX_VERSION_CODE < PIN_USER_PAGES_KERNEL_VERSION)
      put_user_pages(gpu_mem->host_pages, gpu_mem->host_page_num);
#else
      unpin_user_pages(gpu_mem->host_pages, gpu_mem->host_page_num);
#endif
      vfree(gpu_mem->host_pages);
      vfree(gpu_mem);
      return STATUS_ERROR;
    }
  }
  paddr_next = 0;
  remain_size = gpu_mem->size;
  for (i=0; i<nr_pages; i++) {
    off_t cur_offset;
    paddr = page_to_phys(gpu_mem->host_pages[i]);
    if (paddr_next == 0 || (uint64_t)paddr != paddr_next) {
      if (paddr_next) {
        if (pinned_pages < MAX_CONT_NUM) {
          remain_size -= gpu_mem->phys_valid_sizes[pinned_pages];
        } else {
          remain_size -= gpu_mem->phys_region_ext[pinned_pages-MAX_CONT_NUM].valid_size;
        }
        pinned_pages++;
      }
      if (pinned_pages < MAX_CONT_NUM) {
        gpu_mem->phys_sizes[pinned_pages] = PAGE_SIZE;
        gpu_mem->phys_addrs[pinned_pages] = paddr;
      } else {
        gpu_mem->phys_region_ext[pinned_pages-MAX_CONT_NUM].size = PAGE_SIZE;
        gpu_mem->phys_region_ext[pinned_pages-MAX_CONT_NUM].addr = paddr;
      }
      paddr_next = (uint64_t)paddr + PAGE_SIZE;
    } else {
      if (pinned_pages < MAX_CONT_NUM) {
        gpu_mem->phys_sizes[pinned_pages] += PAGE_SIZE;
      } else {
        gpu_mem->phys_region_ext[pinned_pages-MAX_CONT_NUM].size += PAGE_SIZE;
      }
      paddr_next += PAGE_SIZE;
    }
    cur_offset = pinned_pages ? 0 : gpu_mem->offset;
    if (pinned_pages < MAX_CONT_NUM) {
      gpu_mem->phys_valid_sizes[pinned_pages] = gpu_mem->phys_sizes[pinned_pages] - cur_offset;
      if (gpu_mem->phys_valid_sizes[pinned_pages] > remain_size) {
        gpu_mem->phys_valid_sizes[pinned_pages] = remain_size;
      }
    } else {
      gpu_mem->phys_region_ext[pinned_pages-MAX_CONT_NUM].valid_size =
          gpu_mem->phys_region_ext[pinned_pages-MAX_CONT_NUM].size;
      if (gpu_mem->phys_region_ext[pinned_pages-MAX_CONT_NUM].valid_size > remain_size) {
        gpu_mem->phys_region_ext[pinned_pages-MAX_CONT_NUM].valid_size = remain_size;
      }
    }
#if DEBUG_LVL > 1
    printk("paddr[%u]: 0x%llx, size: 0x%llx, valid_size: 0x%llx\n",
           pinned_pages,
           pinned_pages < MAX_CONT_NUM ? gpu_mem->phys_addrs[pinned_pages] :
           gpu_mem->phys_region_ext[pinned_pages-MAX_CONT_NUM].addr,
           pinned_pages < MAX_CONT_NUM ? gpu_mem->phys_sizes[pinned_pages] :
           gpu_mem->phys_region_ext[pinned_pages-MAX_CONT_NUM].size,
           pinned_pages < MAX_CONT_NUM ? gpu_mem->phys_valid_sizes[pinned_pages] :
           gpu_mem->phys_region_ext[pinned_pages-MAX_CONT_NUM].valid_size);
#endif
  }
  if (pinned_pages < MAX_CONT_NUM) {
    if (gpu_mem->phys_sizes[pinned_pages]) pinned_pages++;
  } else {
    if (gpu_mem->phys_region_ext[pinned_pages-MAX_CONT_NUM].size) pinned_pages++;
  }
  gpu_mem->phys_num = pinned_pages;
  if (pinned_pages > 0) {
    gpu_mem->token = gpu_mem->phys_addrs[0] + gpu_mem->offset;
    info.token = gpu_mem->token;
  }

  if (create_base_local_mem(gpu_mem, filp)) {
#if (LINUX_VERSION_CODE < PIN_USER_PAGES_KERNEL_VERSION)
    put_user_pages(gpu_mem->host_pages, gpu_mem->host_page_num);
#else
    unpin_user_pages(gpu_mem->host_pages, gpu_mem->host_page_num);
#endif
    vfree(gpu_mem->host_pages);
    if (gpu_mem->phys_num > MAX_CONT_NUM) {
      vfree(gpu_mem->phys_region_ext);
    }
    vfree(gpu_mem);
    return STATUS_ERROR;
  }
  if (add_gpu_mem(gpu_mem)) {
#if (LINUX_VERSION_CODE < PIN_USER_PAGES_KERNEL_VERSION)
    put_user_pages(gpu_mem->host_pages, gpu_mem->host_page_num);
#else
    unpin_user_pages(gpu_mem->host_pages, gpu_mem->host_page_num);
#endif
    vfree(gpu_mem->host_pages);
    if (gpu_mem->phys_num > MAX_CONT_NUM) {
      vfree(gpu_mem->phys_region_ext);
    }
    vfree(gpu_mem);
    return STATUS_ERROR;
  }

  info.id = gpu_mem->id;

  release_mem(gpu_mem);

  if (copy_to_user(arg, &info, sizeof(mem_manage_host_addr)) != 0) {
    return STATUS_ERROR;
  }
  return STATUS_SUCCESS;
}

#ifdef NVIDIA_PLATFORM
long mem_manage_put_nv_dev_addr(void __user* arg) {
  mem_manage_nv_addr info;
  if (copy_from_user(&info, arg, sizeof(mem_manage_nv_addr)) != 0) {
    return STATUS_ERROR;
  }
  if (delete_mem(info.id, info.force)) return STATUS_ERROR;
  return STATUS_SUCCESS;
}
#endif

#ifdef AMD_PLATFORM
long mem_manage_put_amd_dev_addr(void __user* arg) {
  mem_manage_amd_addr info;
  if (copy_from_user(&info, arg, sizeof(mem_manage_amd_addr)) != 0) {
    return STATUS_ERROR;
  }

  if (delete_mem(info.id, info.force)) return STATUS_ERROR;
  return STATUS_SUCCESS;
}
#endif

long mem_manage_put_host_paddr(void __user *arg) {
  mem_manage_host_addr info;
  if (copy_from_user(&info, arg, sizeof(mem_manage_host_addr)) != 0)
    return STATUS_ERROR;

  if (delete_mem(info.id, info.force)) return STATUS_ERROR;
  return STATUS_SUCCESS;
}

long mem_manage_get_phys_addr_num(uint64_t token, uint32_t* num) {
  global_gpu_mem_list_t* mem_info = NULL;
  if (!token || !num) return STATUS_ERROR;

  mem_info = find_mem_by_token(token);
  if (!mem_info) return STATUS_ERROR;

  *num = mem_info->phys_num;

  release_mem(mem_info);

  return STATUS_SUCCESS;
}

long mem_manage_get_phys_addr_num_gpl(uint64_t token, uint32_t* num) {
    return mem_manage_get_phys_addr_num(token, num);
}

EXPORT_SYMBOL(mem_manage_get_phys_addr_num);
EXPORT_SYMBOL_GPL(mem_manage_get_phys_addr_num_gpl);

long mem_manage_get_paddr_num(void __user* arg) {
  mem_manage_paddr info;
  long ret;
  if (copy_from_user(&info, arg, sizeof(mem_manage_paddr)) != 0) {
    return STATUS_ERROR;
  }
  ret = mem_manage_get_phys_addr_num(info.token, &info.id);
  if (ret < STATUS_SUCCESS) return ret;

  if (copy_to_user(arg, &info, sizeof(mem_manage_paddr)) != 0) {
    return STATUS_ERROR;
  }
  return STATUS_SUCCESS;
}

long mem_manage_get_phys_addr(uint64_t token, uint32_t id, uint64_t* paddr, uint32_t* size) {
  global_gpu_mem_list_t* mem_info = NULL;
  if (!token || !paddr || !size) return STATUS_ERROR;

  mem_info = find_mem_by_token(token);
  if (!mem_info) return STATUS_ERROR;

#if DEBUG_LVL > 1
  {
    int i;
    printk("get_phys_addr: %llx, vaddr=%llx, num=%u, id=%u\n",
           (uint64_t)mem_info, (uint64_t)(mem_info->virt_addr), mem_info->phys_num, id);
    for (i=0; i<mem_info->phys_num; i++) {
      if (i< MAX_CONT_NUM) {
        printk("  [%d]: paddr: %llx, size: %llx, %llx\n", i,
               mem_info->phys_addrs[i], mem_info->phys_sizes[i], mem_info->phys_valid_sizes[i]);
      } else {
        int ii = i - MAX_CONT_NUM;
        printk("  [%d]: paddr: %llx, size: %llx, %llx\n", i,
               mem_info->phys_region_ext[ii].addr, mem_info->phys_region_ext[ii].size,
               mem_info->phys_region_ext[ii].valid_size);
      }
    }
  }
#endif
  if (id >= mem_info->phys_num) {
    release_mem(mem_info);
    return STATUS_ERROR;
  }
  if (id<MAX_CONT_NUM) {
    *paddr = mem_info->phys_addrs[id];
    *size = mem_info->phys_valid_sizes[id];
  } else {
    physical_region_t* region = mem_info->phys_region_ext + id - MAX_CONT_NUM;
    *paddr = region->addr;
    *size = region->valid_size;
  }
  if (id == 0) *paddr += mem_info->offset;

  release_mem(mem_info);
  return STATUS_SUCCESS;
}

long mem_manage_get_phys_addr_gpl(uint64_t token, uint32_t id, uint64_t* paddr, uint32_t* size) {
    return mem_manage_get_phys_addr(token, id, paddr, size);
}

EXPORT_SYMBOL(mem_manage_get_phys_addr);
EXPORT_SYMBOL_GPL(mem_manage_get_phys_addr_gpl);

long mem_manage_get_paddr(void __user* arg) {
  mem_manage_paddr info;
  long ret;
  if (copy_from_user(&info, arg, sizeof(mem_manage_paddr)) != 0) {
    return STATUS_ERROR;
  }
  ret = mem_manage_get_phys_addr(info.token, info.id, &info.paddr, &info.size);
  if (ret < STATUS_SUCCESS) return ret;

  if (copy_to_user(arg, &info, sizeof(mem_manage_paddr)) != 0) {
    return STATUS_ERROR;
  }
  return STATUS_SUCCESS;
}

long mem_manage_get_dev_type(void __user* arg) {
  mem_manage_dev_type info;
  global_gpu_mem_list_t* mem_info = NULL;
  if (copy_from_user(&info, arg, sizeof(mem_manage_dev_type)) != 0) {
    return STATUS_ERROR;
  }
  if (!info.token) return STATUS_ERROR;

  mem_info = find_mem_by_token(info.token);
  if (!mem_info) return STATUS_ERROR;
  info.dev_type = mem_info->dev_type;

  release_mem(mem_info);
  if (copy_to_user(arg, &info, sizeof(mem_manage_dev_type)) != 0) {
    return STATUS_ERROR;
  }
  return STATUS_SUCCESS;
}

int mem_manage_map_gpu_dev_addr(struct file *file, struct vm_area_struct *vma) {
  global_gpu_mem_list_t* gpu_mem;
  local_gpu_mem_list_t* lmem;
  int ret;
  uint64_t size = vma->vm_end - vma->vm_start;
  uint64_t cur_vaddr, remain;
  uint32_t i;

#if DEBUG_LVL > 1
  printk("GPU: map start: %lx %llx %llx %lx\n",
         vma->vm_pgoff, size, (uint64_t)(vma->vm_page_prot.pgprot), vma->vm_flags);
#endif
  gpu_mem = alloc_mem(vma->vm_pgoff, size);
  if (!gpu_mem) {
#if DEBUG_LVL > 1
    printk("GPU: map gpu_mem: %lx\n", (uintptr_t)gpu_mem);
#endif
    return STATUS_NOT_FOUND;
  }
#if DEBUG_LVL > 1
#ifdef NVIDIA_PLATFORM
  if (gpu_mem->page_table) {
    printk("NV: map gpu_mem: %lx\n", (uintptr_t)gpu_mem);
  }
#endif
#ifdef AMD_PLATFORM
  if (gpu_mem->p2p_info) {
    printk("AMD: map gpu_mem: %lx\n", (uintptr_t)gpu_mem);
  }
#endif
  if (gpu_mem->dev_type == DEV_TYPE_CPU && gpu_mem->phys_num > 0) {
    printk("CPU: map gpu_mem: %lx\n", (uintptr_t)gpu_mem);
  }
#endif
  lmem = (local_gpu_mem_list_t*)vmalloc(sizeof(local_gpu_mem_list_t));
  if (!lmem) {
    free_mem(gpu_mem, false);
    return STATUS_ERROR;
  }
  memset(lmem, 0, sizeof(local_gpu_mem_list_t));

  vma->vm_ops = &vm_gpu_ops;
#if (LINUX_VERSION_CODE < KERNEL_VERSION(6,3,0))
  vma->vm_flags |= VM_IO;
#else
  vm_flags_set(vma, VM_IO);
#endif

  vma->vm_pgoff = __phys_to_pfn(gpu_mem->phys_addrs[0] + gpu_mem->offset);
  cur_vaddr = vma->vm_start;
  remain = size;
  for (i=0; i<gpu_mem->phys_num; ++i) {
    uint64_t cur_size, cur_paddr;
    if (remain == 0) {
      printk("MemManage mmap: failed by remain size 0, idx: %d, phys_valid_size: 0x%llx\n",
             i, gpu_mem->phys_valid_sizes[i]);
      release_mem(gpu_mem);
      return STATUS_ERROR;
    }
    if (i < MAX_CONT_NUM) {
      cur_size = gpu_mem->phys_sizes[i] - (i==0 ? (gpu_mem->offset & ~(PAGE_SIZE - 1)) : 0);
      cur_paddr = gpu_mem->phys_addrs[i] + ((i == 0) ? (gpu_mem->offset & ~(PAGE_SIZE - 1)) : 0);
    } else {
      physical_region_t* region = gpu_mem->phys_region_ext + i - MAX_CONT_NUM;
      cur_size = region->size;
      cur_paddr = region->addr;
    }
    cur_size = (remain < cur_size) ? remain : cur_size;

#if DEBUG_LVL > 1
    printk("remap_pfn_range: vaddr: 0x%llx, paddr: 0x%llx, size: 0x%llx\n",
           cur_vaddr, cur_paddr, cur_size);
#endif
    ret = remap_pfn_range(vma, cur_vaddr,
                          __phys_to_pfn(cur_paddr), cur_size, vma->vm_page_prot);
    if (ret) {
#if DEBUG_LVL > 0
      printk("mmap: %llx <- %llx + %llx\n", cur_vaddr, cur_paddr, cur_size);
      break;
#endif
    }
    cur_vaddr += cur_size;
    remain -= cur_size;
  }

#if DEBUG_LVL > 1
#ifdef NVIDIA_PLATFORM
  if (gpu_mem->page_table) {
    printk("NV: remaped: %lx %lx %llx %lx %lx\n",
	   vma->vm_start,
	   vma->vm_pgoff, size, vma->vm_page_prot.pgprot, vma->vm_flags);
  }
#endif
#ifdef AMD_PLATFORM
  if (gpu_mem->p2p_info) {
    printk("AMD: remaped: %lx %lx %llx %lx %lx\n",
	   vma->vm_start,
	   vma->vm_pgoff, size, vma->vm_page_prot.pgprot, vma->vm_flags);
  }
#endif
  if (gpu_mem->dev_type == DEV_TYPE_CPU && gpu_mem->phys_num > 0) {
    printk("CPU: remaped: %lx %lx %llx %lx %lx\n",
	   vma->vm_start,
	   vma->vm_pgoff, size, vma->vm_page_prot.pgprot, vma->vm_flags);
  }
#endif
  if (ret) {
#if DEBUG_LVL > 0
    printk("map failed : %d\n", ret);
#endif
    free_mem(gpu_mem, false);
    vfree(lmem);
    return STATUS_ERROR;
  }

#if DEBUG_LVL > 1
#ifdef NVIDIA_PLATFORM
  if (gpu_mem->page_table) {
    printk("NV: mapped phys: %llx %llx %lx\n",
	   virt_to_phys((void*)(vma->vm_start)),
	   virt_to_bus((void*)(vma->vm_start)),
	   vma->vm_pgoff << PAGE_SHIFT);
  }
#endif
#ifdef AMD_PLATFORM
  if (gpu_mem->p2p_info) {
    printk("AMD: mapped phys: %llx %llx %lx\n",
	   virt_to_phys((void*)(vma->vm_start)),
	   virt_to_bus((void*)(vma->vm_start)),
	   vma->vm_pgoff << PAGE_SHIFT);
  }
#endif
  if (gpu_mem->dev_type == DEV_TYPE_CPU && gpu_mem->phys_num > 0) {
    printk("CPU: mapped phys: %llx %llx %lx\n",
	   virt_to_phys((void*)(vma->vm_start)),
	   virt_to_bus((void*)(vma->vm_start)),
	   vma->vm_pgoff << PAGE_SHIFT);
  }
#endif

  lmem->id = gpu_mem->id;
  lmem->ref_count = 0;
  lmem->file = file;
  add_gpu_lmem(lmem);

  vma->vm_private_data = (void*)lmem;
  vma->vm_ops = &vm_gpu_ops;
  mem_manage_gpu_vma_open(vma);
#if DEBUG_LVL > 1
  printk("GPU: map success\n");
#endif
  release_mem(gpu_mem);
  return STATUS_SUCCESS;
}

int mem_manage_unmap_gpu_dev_addr(struct vm_area_struct *vma) {
  int ret;
  local_gpu_mem_list_t* lmem = (local_gpu_mem_list_t*)vma->vm_private_data;
  if (!lmem) return STATUS_ERROR;

  ret = free_mem_by_id(lmem->id, false);
  if (ret) {
    return ret;
  }
#if DEBUG_LVL > 2
  printk("unmap_gpu_dev_addr delete_gpu_lmem\n");
#endif
  return delete_gpu_lmem(lmem);
}
