/*************************************************
* Copyright 2024 NTT Corporation, FUJITSU LIMITED
* Licensed under the 3-Clause BSD License, see LICENSE for details.
* SPDX-License-Identifier: BSD-3-Clause
*************************************************/

#include <libshmem.h>
#include <liblogging.h>

#include <stdio.h>

#include <map>
#include <vector>
#include <mutex>  // NOLINT


// LogLibFpga
#undef FPGA_LOGGER_LIBNAME
#define FPGA_LOGGER_LIBNAME LIBSHMEM


/**
 * static global variable: Memory map for virt to phys conversion
 *                         <vaddr, <paddr, size>>
 */
static std::map<void*, std::pair<uint64_t, uint64_t>> v2p_map;

/**
 * static global variable: Memory map for phys to virt conversion
 *                         <paddr, <vaddr, size>>
 */
static std::map<uint64_t, std::pair<void*, uint64_t>> p2v_map;

/**
 * static global variable: mutex for access Memory maps(v2p_map,p2v_map)
 */
static std::mutex mmap_mutex;


int fpga_shmem_register(
  void *addr,
  uint64_t paddr,
  size_t size
) {
  llf_dbg("%s(addr(%#lx), paddr(%lx), size(%#lx))\n", __func__, (uintptr_t)addr, paddr, size);

  // Check if the vaddr is already registered into v2p map
  std::lock_guard<std::mutex> lock(mmap_mutex);
  if (v2p_map.find(addr) != v2p_map.end()) {
    // llf_dbg(ALREADY_REGISTERD, "%s(already registered %#lx)\n", __func__, (uintptr_t)addr);
    return 0;
  }

  // Register into v2p map, and ensure that the registerd data does NOT overlap
  //  with other data on the map.
  v2p_map[addr] = std::pair<uint64_t, uint64_t>(paddr, size);
  auto it = v2p_map.find(addr);
  auto next = it;
  next++;
  if (next != v2p_map.end() && (uintptr_t)addr + size > (uintptr_t)(next->first)) {
    // Registered data is not the last data and the region overlaps with next data.
    llf_err(INVALID_ARGUMENT, "%s(next conflict %#lx %#lx)\n",
      __func__, (uintptr_t)addr + size, (uintptr_t)(next->first));
    v2p_map.erase(it);
    return -INVALID_ARGUMENT;
  }

  // Register into p2v map
  p2v_map[paddr] = std::pair<void*, uint64_t>(addr, size);
  // llf_dbg("  register va pa relation: %#lx > %#lx %#x\n", (uintptr_t)addr, paddr, size);
  return 0;
}


// cppcheck-suppress unusedFunction
int fpga_shmem_register_by_token(
  void* token,
  size_t token_len,
  size_t length
) {
  llf_dbg("%s(token(%#lx), token_len(%lx), length(%#lx))\n",
    __func__, (uintptr_t)token, token_len, length);

  // not implemented (using mem_manage)
  return -LIBFPGA_FATAL_ERROR;
}


int fpga_shmem_register_update(
  void *addr,
  uint64_t paddr,
  size_t size
) {
  llf_dbg("%s(addr(%#lx), paddr(%lx), size(%#lx))\n", __func__, (uintptr_t)addr, paddr, size);

  // Check if the vaddr is already registered into v2p map
  std::lock_guard<std::mutex> lock(mmap_mutex);
  auto it_v2p = v2p_map.find(addr);
  if (it_v2p == v2p_map.end()) {
    llf_err(INVALID_ARGUMENT, "%s(Not registerd %#lx)\n", __func__, (uintptr_t)addr);
    return -INVALID_ARGUMENT;
  }

  // Check if the data is registered into p2v map too
  auto it_p2v = p2v_map.find(it_v2p->second.first);
  if (it_p2v == p2v_map.end()) {
    llf_err(INVALID_ARGUMENT, "%s(Not registerd %#lx)\n", __func__, it_v2p->second.first);
    return -INVALID_ARGUMENT;
  }

  if (it_p2v->first == paddr) {
    // When paddr and vaddr is the same with the original data
    // Update only size of v2p and p2v maps when paddr and vaddr is the same with the original data
    it_p2v->second.second = size;
    it_v2p->second.second = size;
  } else {
    // When paddr is different from the original data
    // Erase the data from p2v map and register again
    p2v_map.erase(it_p2v);
    p2v_map[paddr] = std::pair<void*, uint64_t>(addr, size);
    // Update paddr and size of v2p map
    it_v2p->second.first = paddr;
    it_v2p->second.second = size;
  }
  return 0;
}


void fpga_shmem_unregister(
  void *vaddr
) {
  llf_dbg("%s(addr(%#lx))\n", __func__, (uintptr_t)vaddr);

  // Check if the vaddr is already registered into v2p map
  std::lock_guard<std::mutex> lock(mmap_mutex);
  auto it = v2p_map.find(vaddr);
  if (it == v2p_map.end()) {
    llf_dbg(" Not registred this address : %#lx\n", (uintptr_t)vaddr);
    return;
  }

  // Delete the vaddr data from both of v2p and p2v maps
  auto rev_it = p2v_map.find(it->second.first);
  if (rev_it != p2v_map.end()) {
    p2v_map.erase(rev_it);
  }
  v2p_map.erase(it);
}


void fpga_shmem_unregister_all(void) {
  llf_dbg("%s()\n", __func__);

  // Clear v2p map and p2v map
  std::lock_guard<std::mutex> lock(mmap_mutex);
  v2p_map.clear();
  p2v_map.clear();
}


int __fpga_shmem_register_check(
  void *va
) {
  std::lock_guard<std::mutex> lock(mmap_mutex);

  // Get the smallest data larger than va from v2p map
  auto it = v2p_map.upper_bound(va);

  // (va) should be between the (v2p_map.begin()) and (v2p_map.end()),
  // when `it` equals to v2p_map.begin(), va is smaller than the smallest data in the map(i.e. outside)
  if (it == v2p_map.begin()) {
    // llf_err(INVALID_ADDRESS, "The data(%#llx) is outside of the v2p map regions.)\n", (uintptr_t)va);
    return -1;
  }

  // Select the largest data smaller than va
  --it;
  // Get offset of input address from the registered vaddr data
  uint64_t offset = (uintptr_t)va - reinterpret_cast<uint64_t>(it->first);
  // offset is larger than registered region's size
  if (offset >= it->second.second) {
    // llf_err(INVALID_DATA, "The data(%#llx) is outside of the v2p map regions.)\n", (uintptr_t)va);
    return -1;
  }

  return 0;
}


uint64_t __fpga_shmem_mmap_v2p(
  void *va,
  uint64_t *len
) {
  // Check input
  if (!len) {
    llf_err(INVALID_ARGUMENT, "%s(va(%#llx), len(<null>))\n", __func__, (uintptr_t)va);
    return 0;
  }
  llf_dbg("%s(va(%#llx), len(%#llx))\n", __func__, (uintptr_t)va, *len);

  std::lock_guard<std::mutex> lock(mmap_mutex);

  // Get the smallest data larger than va from v2p map
  auto it = v2p_map.upper_bound(va);

  // (va) should be between the (v2p_map.begin()) and (v2p_map.end()),
  // when `it` equals to v2p_map.begin(), va is smaller than the smallest data in the map(i.e. outside)
  if (it == v2p_map.begin()) {
    llf_dbg("  The data(%#llx) is outside of the v2p map regions.\n", (uintptr_t)va);
    return 0;
  }

  // Select the largest data smaller than va
  --it;
  // Get offset of input address from the registered vaddr data
  uint64_t offset = (uintptr_t)va - reinterpret_cast<uint64_t>(it->first);

  // Offset is larger than registered region's size
  if (offset >= it->second.second) {
    llf_dbg("  Failed to convert address from virt to phys by local virt2phys map.\n");
    return 0;
  }

  // Get phys address
  uint64_t paddr = it->second.first + offset;
  // Check if the entity of the pointer variable `len` is too large for the registered region.
  if (*len > it->second.second - offset) {
    // Since `*len` has exceeded the region, return the length that can guarantee physical continuity.
    llf_dbg("  length is shortened to fit within registered data size(%#lx) : %#lx -> %#lx\n",
      it->second.second, *len, it->second.second - offset);
    *len = it->second.second - offset;
  }

  // llf_dbg("%s(va(%#llx), pa(%#llx), len(%#llx))\n", __func__, (uintptr_t)va, paddr, *len);
  return paddr;
}


void *__fpga_shmem_mmap_p2v(
  uint64_t pa64
) {
  llf_dbg("%s(pa64(%#llx))\n", __func__, pa64);

  std::lock_guard<std::mutex> lock(mmap_mutex);

  // Get the smallest data larger than pa64 from p2v map
  auto it = p2v_map.upper_bound(pa64);

  // (pa64) should be between the (p2v_map.begin()) and (p2v_map.end()),
  // when `it` equals to p2v_map.begin(), pa64 is smaller than the smallest data in the map(i.e. outside)
  if (it == p2v_map.begin()) {
    llf_dbg("  Failed to convert address from phys to virt by local phys2virt map.\n");
    return NULL;
  }

  // Select the largest data smaller than va
  --it;
  // Get offset of input address from the registered paddr data
  uint64_t offset = pa64 - it->first;

  // Offset is larger than registered region's size
  if (offset >= it->second.second) {
    llf_dbg("  Failed to convert address from phys to virt by local phys2virt map.\n");
    return NULL;
  }

  // Get vaddr from p2v map and Return address which is added offset
  uint64_t vaddr = reinterpret_cast<uint64_t>(it->second.first) + offset;
  return reinterpret_cast<void*>(vaddr);
}
