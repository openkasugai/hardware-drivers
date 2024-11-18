/*************************************************
* Copyright 2024 NTT Corporation, FUJITSU LIMITED
* Licensed under the GPL-2.0  License with an explicit syscall exception, see LICENSE for details.
* SPDX-License-Identifier: GPL-2.0-or-later WITH Linux-syscall-note
*************************************************/
/**
 * @file xpcie_device.h
 * @brief Header file for xpcie driver. To use ioctl, need to include this file.
 */

#ifndef __XPCIE_DEVICE_H__
#define __XPCIE_DEVICE_H__

#include <linux/ioctl.h>
#ifndef __KERNEL__
#include <stdint.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#endif

// Definition for driver version
#ifndef DRIVER_TYPE
#define DRIVER_TYPE                         0xFF
#endif

#ifndef DRIVER_MAJOR_VER
#define DRIVER_MAJOR_VER                    0xFF
#endif

#ifndef DRIVER_MINOR_VER
#define DRIVER_MINOR_VER                    0xFF
#endif

#ifndef DRIVER_REVISION
#define DRIVER_REVISION                     0xFF
#endif

#ifndef DRIVER_PATCH
#define DRIVER_PATCH                        0xFF
#endif


/**
 * @enum dma_dir_t
 * @brief Enumeration of DMA direction
 */
typedef enum DMA_DIRECTION{
  DMA_HOST_TO_DEV = 0,    /**< RX (Host -> dev) */
  DMA_DEV_TO_HOST,        /**< TX (dev -> Host) */
  DMA_NW_TO_DEV,          /**< RX (NW -> dev) */
  DMA_DEV_TO_NW,          /**< TX (dev -> NW) */
  // upper:use in kernel and user space
  DMA_DIR_MAX,            /**< Sentinel for user space */
  //lower: use in only kernel
  DMA_D2D_RX,             /**< D2D-H(Host -> dev) */
  DMA_D2D_TX,             /**< D2D-H(dev -> Host) */
  DMA_D2D_D_RX,           /**< D2D-D(Host -> dev) */
  DMA_D2D_D_TX,           /**< D2D-D(dev -> Host) */
} dma_dir_t;


/**
 * @struct fpga_power_t
 * @brief Struct for power consumption for Alveo U250
 */
typedef struct fpga_power {
  uint32_t  pcie_12V_voltage;
  uint32_t  pcie_12V_current;
  uint32_t  aux_12V_voltage;
  uint32_t  aux_12V_current;
  uint32_t  pex_3V3_voltage;
  uint32_t  pex_3V3_current;
  uint32_t  pex_3V3_power;
  uint32_t  aux_3V3_voltage;
  uint32_t  aux_3V3_current;
  uint32_t  vccint_voltage;
  uint32_t  vccint_current;
} fpga_power_t;


// Definition of Hardware
// about function chain
#define XPCIE_KERNEL_LANE_MAX               4     /**< The max num of lane */

#define XPCIE_PTU_CID_MIN_NON_MODULE        1     /**< Min cid for ethernet chain for non-module FPGA */
#define XPCIE_PTU_CID_MAX_NON_MODULE        511   /**< Max cid for ethernet chain for non-module FPGA */
#define XPCIE_LLDMA_CID_MIN_NON_MODULE      512   /**< Min cid for PCIe chain for non-module FPGA */
#define XPCIE_LLDMA_CID_MAX_NON_MODULE      543   /**< Max cid for PCIe chain for non-module FPGA */

#define XPCIE_CID_MIN                       0     /**< Min cid for PCIe for module FPGA */
#define XPCIE_CID_MAX                       511   /**< Max cid for PCIe for module FPGA */

#define XPCIE_FUNCTION_CHAIN_ID_MIN         0     /**< Min fchid for module FPGA */
#define XPCIE_FUNCTION_CHAIN_ID_MAX         511   /**< Max fchid for module FPGA */
#define XPCIE_FUNCTION_CHAIN_MAX            (XPCIE_FUNCTION_CHAIN_ID_MAX - XPCIE_FUNCTION_CHAIN_ID_MIN + 1)


/**
 * @struct fpga_desc_t
 * @brief Struct for descripter of Command Queue
 */
typedef struct fpga_desc {
  uint16_t task_id;
  uint8_t  op;
  uint8_t  status;
  uint32_t len;
  uint64_t addr;
  uint8_t __padding[48]; // for 64-bytes align
} __attribute((packed)) fpga_desc_t;

/**
 * @struct fpga_queue_t
 * @brief Struct for Command Queue
 */
typedef struct fpga_queue {
  uint16_t size;
  uint16_t readhead;
  uint16_t writehead;
  uint8_t  _padding[58]; // for 64-bytes align
  struct fpga_desc ring[0];
} __attribute((packed)) fpga_queue_t;


// Definition of Software parameter
// about general info
#define FPGA_CARD_NAME_LEN                  32    /**< Max length for card name */
#define CONNECTOR_ID_NAME_MAX               128   /**< Max length for connector_id */

#define ECCERR_TYPE_SINGLE                  0
#define ECCERR_TYPE_MULTI                   1

#define FPGA_EXTIF_NUMBER_0                 0
#define FPGA_EXTIF_NUMBER_1                 1

// Definition for driver errno
#define XPCIE_DEV_UPDATE_TIMEOUT            1     /**< errno : Failed to update function chain table */
#define XPCIE_DEV_NO_CHAIN_FOUND            2     /**< errno : Failed to find function chain */
#define XPCIE_DEV_REFCOUNT_WRITING          3     /**< errno : FPGA is just writing */
#define XPCIE_DEV_REFCOUNT_USING            4     /**< errno : FPGA is just using */
/* LINUX errno used in this driver */
// ENOMEM 12
// EFAULT 14
// EBUSY  16
// ENODEV 19
// EINVAL 22


/**
 * @enum FPGA_CONTROL_TYPE
 * @brief Enumeration of FPGA's control type
 */
enum FPGA_CONTROL_TYPE {
  FPGA_CONTROL_UNKNOWN,       /**< invalid */
  FPGA_CONTROL_MODULE,        /**< modulized FPGA */
  FPGA_CONTROL_MAX,           /**< Sentinel: Invalid parameter */
};

/**
 * @enum xpcie_refcount_cmd_t
 * @brief Enumeration of FPGA's refcount control command
 */
typedef enum XPCIE_DEV_REFCOUNT_COMMAND {
  XPCIE_DEV_REFCOUNT_INC = 0, /**< Start using */
  XPCIE_DEV_REFCOUNT_DEC,     /**< Finish using */
  XPCIE_DEV_REFCOUNT_WRITE,   /**< Start writing */
  XPCIE_DEV_REFCOUNT_CLEAR,   /**< Finish writing */
  XPCIE_DEV_REFCOUNT_GET,     /**< Get refcount */
  XPCIE_DEV_REFCOUNT_RST,     /**< Forced set 0 */
  XPCIE_DEV_REFCOUNT_MAX,     /**< Sentinel: Invalid parameter */
} xpcie_refcount_cmd_t;

/**
 * @enum xpcie_refcount_cmd_t
 * @brief Enumeration of FPGA's regions to control reference count
 */
typedef enum XPCIE_DEV_REGION{
  XPCIE_DEV_REGION_INV = 0, /**< Invalid */
  XPCIE_DEV_REGION_ALL,     /**< Device */
  XPCIE_DEV_REGION_MAX,     /**< Sentinel */
} xpcie_region_t;

/**
 * @enum XPCIE_DEV_REGCTRL_COMMAND
 * @brief Enumeration of commands to set status for register accessablity
 */
enum XPCIE_DEV_REGCTRL_COMMAND {
  XPCIE_DEV_REG_ENABLE = 0, /**< Enable read()/write() */
  XPCIE_DEV_REG_DISABLE,    /**< Disable read()/write() */
};

/**
 * @enum FPGA_TEMP_FLAG
 * @brief Enumeration of flags for CMS temp register
 */
enum FPGA_TEMP_FLAG {
  U250_CAGE_TEMP0 = 0,
  U250_CAGE_TEMP1,
  U250_DIMM_TEMP0,
  U250_DIMM_TEMP1,
  U250_DIMM_TEMP2,
  U250_DIMM_TEMP3,
  U250_FAN_TEMP,
  U250_FPGA_TEMP,
  U250_SE98_TEMP0,
  U250_SE98_TEMP1,
  U250_SE98_TEMP2,
  U250_VCCINT_TEMP,
  /* New Card info will be added later when it is supported. */
};

/**
 * @enum FPGA_POWER_FLAG
 * @brief Enumeration of flags for CMS power register
 */
enum FPGA_POWER_FLAG {
  U250_PCIE_12V_VOLTAGE = 0,
  U250_PCIE_12V_CURRENT,
  U250_AUX_12V_VOLTAGE,
  U250_AUX_12V_CURRENT,
  U250_PEX_3V3_VOLTAGE,
  U250_PEX_3V3_CURRENT,
  U250_PEX_3V3_POWER,
  U250_AUX_3V3_VOLTAGE,
  U250_AUX_3V3_CURRENT,
  U250_VCCINT_VOLTAGE,
  U250_VCCINT_CURRENT,
  /* New Card info will be added later when it is supported. */
};

/**
 * @enum CHAIN_REG_COUNTER
 * @brief Enumeration for counter of chain module
 */
enum CHAIN_REG_COUNTER{
  CHAIN_STAT_INGR_RCV0 = 0,
  CHAIN_STAT_INGR_RCV1,
  CHAIN_STAT_INGR_SND0,
  CHAIN_STAT_INGR_SND1,
  CHAIN_STAT_EGR_RCV0,
  CHAIN_STAT_EGR_RCV1,
  CHAIN_STAT_EGR_SND0,
  CHAIN_STAT_EGR_SND1,
  CHAIN_STAT_INGR_DISCARD0,
  CHAIN_STAT_INGR_DISCARD1,
  CHAIN_STAT_EGR_DISCARD0,
  CHAIN_STAT_EGR_DISCARD1,
};

/**
 * @enum DIRECT_REG_COUNTER
 * @brief Enumeration for counter of direct module
 */
enum DIRECT_REG_COUNTER{
  DIRECT_STAT_INGR_RCV = 0,
  DIRECT_STAT_INGR_SND,
  DIRECT_STAT_EGR_RCV,
  DIRECT_STAT_EGR_SND,
};

/**
 * @enum DIRECT_DIR_KIND
 * @brief Enumeration for direction and kind of direct module
 */
enum DIRECT_DIR_KIND{
  DIRECT_DIR_INGR_RCV = 0,
  DIRECT_DIR_INGR_SND,
  DIRECT_DIR_EGR_RCV,
  DIRECT_DIR_EGR_SND,
};

/**
 * @enum FPGA_INGRESS_EGRESS_KIND
 * @brief Enumeration for direction of cid
 */
enum FPGA_INGRESS_EGRESS_KIND{
  FPGA_CID_KIND_INGRESS = 0,
  FPGA_CID_KIND_EGRESS,
  FPGA_CID_KIND_MAX,
};


/**
 * @struct xpcie_fpga_bitstream_id_t
 * @brief Struct for FPGA's BitstreamID
 */
typedef struct xpcie_fpga_bitstream_id {
  uint32_t parent;  /**< Parent bitstream_id(PCI configuration) */
  uint32_t child;   /**< Global Major version */
} xpcie_fpga_bitstream_id_t;

/**
 * @struct fpga_card_info_t
 * @brief Struct for FPGA's card information
 */
typedef struct fpga_card_info {
  xpcie_fpga_bitstream_id_t bitstream_id;   /**< Bitstream id */
  uint16_t  pci_device_id;                  /**< pci device id */
  uint16_t  pci_vendor_id;                  /**< pci vendor id */
  uint16_t  pci_domain;                     /**< pci domain */
  uint16_t  pci_bus;                        /**< pci bus id */
  uint8_t   pci_dev;                        /**< pci dev id */
  uint8_t   pci_func;                       /**< pci func id */
  enum FPGA_CONTROL_TYPE ctrl_type;         /**< FPGA's type(module/non-module) */
  char      card_name[FPGA_CARD_NAME_LEN];  /**< Card name got by CMS */
} fpga_card_info_t;

/**
 * @struct fpga_address_info_t
 * @brief Struct for Module's register map information for user space
 */
typedef struct fpga_address_info {
  uint64_t base;  /**< base address */
  uint32_t len;   /**< length per a lane */
  uint32_t num;   /**< lane num */
} fpga_address_info_t;

/**
 * @struct fpga_address_map_t
 * @brief Struct for ALL Module's register map information for user space
 */
typedef struct fpga_address_map {
  fpga_address_info_t global; /**< Global module's address map */
  fpga_address_info_t chain;  /**< Chain module's address map */
  fpga_address_info_t direct; /**< Direct module's address map */
  fpga_address_info_t lldma;  /**< LLDMA module's address map */
  fpga_address_info_t ptu;    /**< PTU module's address map */
  fpga_address_info_t conv;   /**< Conversion module's address map */
  fpga_address_info_t func;   /**< Function module's address map */
  fpga_address_info_t cms;    /**< CMS module's address map */
} fpga_address_map_t;


// Struct for ioctl information structure
// About global
/**
 * @struct fpga_ioctl_clkdown_t
 */
typedef struct fpga_ioctl_clkdown {
  uint8_t user_clk;
  uint8_t ddr4_clk0;
  uint8_t ddr4_clk1;
  uint8_t ddr4_clk2;
  uint8_t ddr4_clk3;
  uint8_t qsfp_clk0;
  uint8_t qsfp_clk1;
} fpga_ioctl_clkdown_t;

/**
 * @struct fpga_ioctl_eccerr_t
 */
typedef struct fpga_ioctl_eccerr {
  uint32_t type;
  uint32_t eccerr;
} fpga_ioctl_eccerr_t;

/**
 * @struct fpga_ioctl_extif_t
 */
typedef struct fpga_ioctl_extif {
  int lane;
  uint8_t extif_id;
} fpga_ioctl_extif_t;

// Chain
/**
 * @struct fpga_id_t
 * @brief Struct for function chain update
 */
typedef struct fpga_id{
  uint32_t lane;          /**< lane */
  uint32_t extif_id;      /**< external interface id */
  uint16_t cid;           /**< connection id */
  uint16_t fchid;         /**< function chain id */
  uint8_t  enable_flag;   /**< flag to enable table */
  uint8_t  active_flag;   /**< flag to allow transfer */
  uint8_t  direct_flag;   /**< flag for direct transfer */
  uint8_t  virtual_flag;  /**< flag for virtual connection */
  uint8_t  blocking_flag; /**< flag for blocking transfer */
}fpga_id_t;

/**
 * @struct fpga_ioctl_chain_id_t
 * @brief Struct for function chain read table in driver
 */
typedef struct fpga_ioctl_chain_ids{
  uint32_t lane;              /**< lane */
  uint32_t fchid;             /**< function chain id */
  uint32_t ingress_extif_id;  /**< external interface id */
  uint32_t ingress_cid;       /**< connection id */
  uint32_t egress_extif_id;   /**< external interface id */
  uint32_t egress_cid;        /**< connection id */
}fpga_ioctl_chain_ids_t;

/**
 * @struct fpga_ioctl_chain_ddr_t
 */
typedef struct fpga_ioctl_chain_ddr {
  int      lane;
  uint64_t base;
  uint64_t rx_offset;
  uint32_t rx_stride;
  uint64_t tx_offset;
  uint32_t tx_stride;
  uint8_t  extif_id;
  uint8_t  rx_size;
  uint8_t  tx_size;
} fpga_ioctl_chain_ddr_t;

/**
 * @struct fpga_ioctl_chain_latency_t
 */
typedef struct fpga_ioctl_chain_latency {
  int      lane;
  uint8_t  extif_id;
  uint16_t cid;
  uint8_t  dir;
  uint32_t latency ;
} fpga_ioctl_chain_latency_t;

/**
 * @struct fpga_ioctl_chain_func_latency_t
 */
typedef struct fpga_ioctl_chain_func_latency {
  int      lane;
  uint32_t latency;
  uint16_t fchid;
} fpga_ioctl_chain_func_latency_t;

/**
 * @struct fpga_ioctl_chain_framenum_t
 */
typedef struct fpga_ioctl_chain_framenum {
  int      lane;
  uint16_t reg_id;
  uint16_t fchid;
  uint32_t frame_num;
} fpga_ioctl_chain_framenum_t;

/**
 * @struct fpga_ioctl_chain_bytenum_t
 */
typedef struct fpga_ioctl_chain_bytenum {
  int      lane;
  uint16_t reg_id;
  uint16_t cid_fchid;
  uint64_t byte_num;
} fpga_ioctl_chain_bytenum_t;

/**
 * @struct fpga_ioctl_err_all_t
 */
typedef struct fpga_ioctl_err_all {
  int        lane;
  uint32_t   err_all;
} fpga_ioctl_err_all_t;

/**
 * @struct fpga_ioctl_chain_err_t
 */
typedef struct fpga_ioctl_chain_err {
  int      lane;
  uint8_t  extif_id;
  uint8_t  dir;
  uint16_t cid_fchid;
  uint8_t  header_marker;
  uint8_t  payload_len;
  uint8_t  header_len;
  uint8_t  header_chksum;
  uint8_t  header_stat;
  uint8_t  pointer_table_miss;
  uint8_t  payload_table_miss;
  uint8_t  con_table_miss;
  uint8_t  pointer_table_invalid;
  uint8_t  payload_table_invalid;
  uint8_t  con_table_invalid;
  uint8_t  __padding;
} fpga_ioctl_chain_err_t;

/**
 * @struct fpga_ioctl_chain_err_table_t
 */
typedef struct fpga_ioctl_chain_err_table {
  int      lane;
  uint8_t  extif_id;
  uint8_t  dir;
  uint16_t cid_fchid;
  uint8_t  con_table_miss;
  uint8_t  con_table_invalid;
} fpga_ioctl_chain_err_table_t;

/**
 * @struct fpga_ioctl_chain_err_prot_t
 */
typedef struct fpga_ioctl_chain_err_prot {
  int     lane;
  uint8_t dir;
  uint8_t prot_ch;
  uint8_t prot_len;
  uint8_t prot_sof;
  uint8_t prot_eof;
  uint8_t prot_reqresp;
  uint8_t prot_datanum;
  uint8_t prot_req_outstanding;
  uint8_t prot_resp_outstanding;
  uint8_t prot_max_datanum;
  uint8_t prot_reqlen;
  uint8_t prot_reqresplen;
} fpga_ioctl_chain_err_prot_t;

/**
 * @struct fpga_ioctl_chain_err_evt_t
 */
typedef struct fpga_ioctl_chain_err_evt {
  int     lane;
  uint8_t extif_id;
  uint8_t established;
  uint8_t close_wait;
  uint8_t erased;
  uint8_t syn_timeout;
  uint8_t syn_ack_timeout;
  uint8_t timeout;
  uint8_t recv_data;
  uint8_t send_data;
  uint8_t recv_urgent_data;
  uint8_t recv_rst;
} fpga_ioctl_chain_err_evt_t;

/**
 * @struct fpga_ioctl_chain_err_stif_t
 */
typedef struct fpga_ioctl_chain_err_stif {
  int     lane;
  uint8_t ingress_req;
  uint8_t ingress_resp;
  uint8_t ingress_data;
  uint8_t egress_req;
  uint8_t egress_resp;
  uint8_t egress_data;
  uint8_t extif_event;
  uint8_t extif_command;
} fpga_ioctl_chain_err_stif_t;

/**
 * @struct fpga_ioctl_chain_ctrl_t
 */
typedef struct fpga_ioctl_chain_ctrl {
  int      lane;
  uint32_t value;
} fpga_ioctl_chain_ctrl_t;

/**
 * @struct fpga_ioctl_chain_err_cmdfault_t
 */
typedef struct fpga_ioctl_chain_err_cmdfault {
  int lane;
  uint16_t enable;
  uint16_t cid;
  uint8_t extif_id;
} fpga_ioctl_chain_err_cmdfault_t;

/**
 * @struct fpga_ioctl_chain_con_status_t
 */
typedef struct fpga_ioctl_chain_con_status {
  int lane;
  uint32_t extif_id;
  uint32_t cid;
  uint32_t value;
} fpga_ioctl_chain_con_status_t;

// Direct
/**
 * @struct fpga_ioctl_direct_framenum_t
 */
typedef struct fpga_ioctl_direct_framenum {
  int      lane;
  uint16_t reg_id;
  uint16_t fchid;
  uint32_t frame_num;
} fpga_ioctl_direct_framenum_t;

/**
 * @struct fpga_ioctl_direct_bytenum_t
 */
typedef struct fpga_ioctl_direct_bytenum {
  int      lane;
  uint16_t reg_id;
  uint16_t fchid;
  uint64_t byte_num;
} fpga_ioctl_direct_bytenum_t;

/**
 * @struct fpga_ioctl_direct_err_prot_t
 */
typedef struct fpga_ioctl_direct_err_prot {
  int     lane;
  uint8_t dir_type;
  uint8_t prot_ch;
  uint8_t prot_len;
  uint8_t prot_sof;
  uint8_t prot_eof;
  uint8_t prot_reqresp;
  uint8_t prot_datanum;
  uint8_t prot_req_outstanding;
  uint8_t prot_resp_outstanding;
  uint8_t prot_max_datanum;
  uint8_t prot_reqlen;
  uint8_t prot_reqresplen;
} fpga_ioctl_direct_err_prot_t;

/**
 * @struct fpga_ioctl_direct_err_stif_t
 */
typedef struct fpga_ioctl_direct_err_stif {
  int     lane;
  uint8_t ingress_rcv_req;
  uint8_t ingress_rcv_resp;
  uint8_t ingress_rcv_data;
  uint8_t ingress_snd_req;
  uint8_t ingress_snd_resp;
  uint8_t ingress_snd_data;
  uint8_t egress_rcv_req;
  uint8_t egress_rcv_resp;
  uint8_t egress_rcv_data;
  uint8_t egress_snd_req;
  uint8_t egress_snd_resp;
  uint8_t egress_snd_data;
} fpga_ioctl_direct_err_stif_t;

/**
 * @struct fpga_ioctl_direct_ctrl_t
 */
typedef struct fpga_ioctl_direct_ctrl {
  int      lane;
  uint32_t value;
} fpga_ioctl_direct_ctrl_t;

// LLDMA
/**
 * @struct fpga_ioctl_queue_t
 * @brief Struct for information of LLDMA's Command queue
 */
typedef struct fpga_ioctl_queue {
  uint16_t dir;                                 /**< Direction of LLDMA channel */
  uint16_t chid;                                /**< Channel id of LLDMA channel */
  ssize_t  map_size;                            /**< The size of command queue */
  char     connector_id[CONNECTOR_ID_NAME_MAX]; /**< ConnectorID */
} fpga_ioctl_queue_t;

/**
 * @struct fpga_ioctl_chsts_t
 * @brief Struct for status of LLDMA's channel
 */
typedef struct fpga_ioctl_chsts {
  uint16_t dir;             /**< Direction of LLDMA channel */
  uint32_t avail_status;    /**< Available Status(is implemented or not) */
  uint32_t active_status;   /**< Active Status(is used or not) */
} fpga_ioctl_chsts_t;

/**
 * @struct fpga_ioctl_cidchain_t
 * @brief Struct for status of LLDMA's chain
 */
typedef struct fpga_ioctl_cidchain {
  uint16_t dir;       /**< Direction of LLDMA channel */
  uint16_t chid;      /**< Channel id of LLDMA channel */
  uint16_t cid;       /**< ConnectorID */
  uint16_t chain_no;  /**< Chain control Number */
} fpga_ioctl_cidchain_t;

/**
 * @struct fpga_ioctl_connect_t
 * @brief Struct for status of LLDMA's chain
 */
typedef struct fpga_ioctl_connect {
  uint16_t self_dir;                            /**< Direction of self device */
  uint32_t self_chid;                           /**< Channel id of self device */
  uint32_t peer_chid;                           /**< Direction of peer device */
  uint8_t  peer_minor;                          /**< Channel id of peer device */
  uint32_t buf_size;                            /**< Buffer size used when D2D-H */
  uint64_t buf_addr;                            /**< Buffer address used when D2D-H */
  char     connector_id[CONNECTOR_ID_NAME_MAX]; /**< ConnectorID */
} fpga_ioctl_connect_t;

/**
 * @struct fpga_ioctl_up_info_t
 */
typedef struct fpga_ioctl_up_info {
  uint16_t chid;     /**< Channel id of LLDMA channel */
  uint32_t size;     /**< Frame size */
} fpga_ioctl_up_info_t;

/**
 * @enum fpga_ioctl_lldma_buffer_cmd_t
 * @brief Enumeration of commands to control LLDMA's buffer
 */
typedef enum FPGA_IOCTL_LLDMA_BUFFER_CMD {
  CMD_FPGA_IOCTL_LLDMA_BUF_INV = 0,  /**< Invalid */
  CMD_FPGA_IOCTL_LLDMA_BUF_SET,      /**< Set data */
  CMD_FPGA_IOCTL_LLDMA_BUF_CLR,      /**< Clear data(= Set 0) */
  CMD_FPGA_IOCTL_LLDMA_BUF_GET,      /**< Read registers' values */
} fpga_ioctl_lldma_buffer_cmd_t;

/**
 * @struct fpga_ioctl_lldma_buffer_regs_t
 * @brief Struct for getting register value of LLDMA's buffer
 */
typedef struct fpga_ioctl_lldma_buffer_regs {
  uint32_t dn_rx_val_l[XPCIE_KERNEL_LANE_MAX];  /**< XPCIE_FPGA_LLDMA_CIF_DN_RX_BASE_L(lane) */
  uint32_t dn_rx_val_h[XPCIE_KERNEL_LANE_MAX];  /**< XPCIE_FPGA_LLDMA_CIF_DN_RX_BASE_H(lane) */
  uint32_t up_tx_val_l[XPCIE_KERNEL_LANE_MAX];  /**< XPCIE_FPGA_LLDMA_CIF_UP_TX_BASE_L(lane) */
  uint32_t up_tx_val_h[XPCIE_KERNEL_LANE_MAX];  /**< XPCIE_FPGA_LLDMA_CIF_UP_TX_BASE_H(lane) */
  uint32_t dn_rx_ddr_size;                      /**< XPCIE_FPGA_LLDMA_CIF_DN_RX_DDR_SIZE_VAL */
  uint32_t up_tx_ddr_size;                      /**< XPCIE_FPGA_LLDMA_CIF_UP_TX_DDR_SIZE_VAL */
} fpga_ioctl_lldma_buffer_regs_t;

/**
 * @struct fpga_ioctl_lldma_buffer_t
 * @brief Struct for pair of command and data of LLDMA's buffer
 */
typedef struct fpga_ioctl_lldma_buffer {
  fpga_ioctl_lldma_buffer_cmd_t cmd;   /**< Command for LLDMA's buffer */
  fpga_ioctl_lldma_buffer_regs_t regs; /**< Data for LLDMA's buffer */
} fpga_ioctl_lldma_buffer_t;

// CMS
/**
 * @struct fpga_ioctl_temp_t
 * @brief Struct for pair of register and data of FPGA's temperature
 */
typedef struct fpga_ioctl_temp {
  uint32_t temp;              /**< Temperature */
  enum FPGA_TEMP_FLAG flag;   /**< Register */
} fpga_ioctl_temp_t;

/**
 * @struct fpga_ioctl_power_t
 * @brief Struct for pair of register and data of FPGA's power
 */
typedef struct fpga_ioctl_power {
  uint32_t power;              /**< Power */
  enum FPGA_POWER_FLAG flag;   /**< Register */
} fpga_ioctl_power_t;

// General
/**
 * @struct fpga_ioctl_refcount_t;
 * @brief Struct for controlling reference count
 */
typedef struct fpga_ioctl_refcount {
  xpcie_refcount_cmd_t cmd; /**< Command for controlling refcount */
  xpcie_region_t region;    /**< Target Region of FPGA */
  int refcount;             /**< The data to set for user */
} fpga_ioctl_refcount_t;


// Definition of ioctl Commands
#define MAGIC 'h'

// device info
#define XPCIE_DEV_MPOLL                       _IO(MAGIC,   0x00)
#define XPCIE_DEV_DRIVER_GET_DEVICE_ID        _IOR(MAGIC,  0x01, uint32_t)

#define XPCIE_DEV_DRIVER_GET_VERSION          _IOR(MAGIC,  0x02, uint32_t)

#define XPCIE_DEV_DRIVER_GET_DEVICE_INFO      _IOR(MAGIC,  0x03, fpga_card_info_t)
#define XPCIE_DEV_DRIVER_SET_REFCOUNT         _IOWR(MAGIC, 0x04, fpga_ioctl_refcount_t)

#define XPCIE_DEV_DRIVER_SET_FPGA_UPDATE      _IO(MAGIC,   0x05)
#define XPCIE_DEV_DRIVER_SET_REG_LOCK         _IOW(MAGIC,  0x06, uint32_t)
#define XPCIE_DEV_DRIVER_GET_FPGA_TYPE        _IOR(MAGIC,  0x07, enum FPGA_CONTROL_TYPE)
#define XPCIE_DEV_DRIVER_GET_FPGA_ADDR_MAP    _IOR(MAGIC,  0x08, fpga_address_map_t)

#define XPCIE_DEV_DRIVER_GET_REFCOUNT         _IOWR(MAGIC, 0x09, fpga_ioctl_refcount_t)

/* 0x0a-0x0f : Missing number */

// LLDMA
#define XPCIE_DEV_LLDMA_GET_VERSION            _IOR(MAGIC,  0x10, uint32_t)
#define XPCIE_DEV_LLDMA_ALLOC_QUEUE            _IOWR(MAGIC, 0x11, fpga_ioctl_queue_t)
#define XPCIE_DEV_LLDMA_FREE_QUEUE             _IOW(MAGIC,  0x12, fpga_ioctl_queue_t)
#define XPCIE_DEV_LLDMA_BIND_QUEUE             _IOWR(MAGIC, 0x13, fpga_ioctl_queue_t)

#define XPCIE_DEV_LLDMA_GET_CH_STAT            _IOWR(MAGIC, 0x14, fpga_ioctl_chsts_t)
#define XPCIE_DEV_LLDMA_GET_CID_CHAIN          _IOWR(MAGIC, 0x15, fpga_ioctl_cidchain_t)

#define XPCIE_DEV_LLDMA_ALLOC_CONNECTION       _IOW(MAGIC,  0x16, fpga_ioctl_connect_t)
#define XPCIE_DEV_LLDMA_FREE_CONNECTION        _IOW(MAGIC,  0x17, fpga_ioctl_connect_t)

#define XPCIE_DEV_LLDMA_GET_UP_SIZE            _IOWR(MAGIC, 0x18, fpga_ioctl_up_info_t)

#define XPCIE_DEV_LLDMA_GET_RXCH_CTRL0         _IOR(MAGIC,  0x19, uint32_t)

#define XPCIE_DEV_LLDMA_CTRL_DDR_BUFFER        _IOWR(MAGIC, 0x1a, fpga_ioctl_lldma_buffer_t)

/* 0x1b-0x3f : Missing number */

// Global
#define XPCIE_DEV_GLOBAL_CTRL_SOFT_RST        _IO(MAGIC,   0x40)
#define XPCIE_DEV_GLOBAL_GET_CHK_ERR          _IOR(MAGIC,  0x41, uint32_t)

#define XPCIE_DEV_GLOBAL_GET_CLKDOWN          _IOR(MAGIC,  0x42, fpga_ioctl_clkdown_t)
#define XPCIE_DEV_GLOBAL_SET_CLKDOWN_CLR      _IOW(MAGIC,  0x43, fpga_ioctl_clkdown_t)
#define XPCIE_DEV_GLOBAL_GET_CLKDOWN_RAW      _IOR(MAGIC,  0x44, fpga_ioctl_clkdown_t)
#define XPCIE_DEV_GLOBAL_SET_CLKDOWN_MASK     _IOW(MAGIC,  0x45, fpga_ioctl_clkdown_t)
#define XPCIE_DEV_GLOBAL_GET_CLKDOWN_MASK     _IOR(MAGIC,  0x46, fpga_ioctl_clkdown_t)
#define XPCIE_DEV_GLOBAL_SET_CLKDOWN_FORCE    _IOW(MAGIC,  0x47, fpga_ioctl_clkdown_t)
#define XPCIE_DEV_GLOBAL_GET_CLKDOWN_FORCE    _IOR(MAGIC,  0x48, fpga_ioctl_clkdown_t)

#define XPCIE_DEV_GLOBAL_GET_ECCERR           _IOWR(MAGIC, 0x49, fpga_ioctl_eccerr_t)
#define XPCIE_DEV_GLOBAL_SET_ECCERR_CLR       _IOW(MAGIC,  0x4a, fpga_ioctl_eccerr_t)
#define XPCIE_DEV_GLOBAL_GET_ECCERR_RAW       _IOWR(MAGIC, 0x4b, fpga_ioctl_eccerr_t)
#define XPCIE_DEV_GLOBAL_SET_ECCERR_MASK      _IOW(MAGIC,  0x4c, fpga_ioctl_eccerr_t)
#define XPCIE_DEV_GLOBAL_GET_ECCERR_MASK      _IOWR(MAGIC, 0x4d, fpga_ioctl_eccerr_t)
#define XPCIE_DEV_GLOBAL_SET_ECCERR_FORCE     _IOW(MAGIC,  0x4e, fpga_ioctl_eccerr_t)
#define XPCIE_DEV_GLOBAL_GET_ECCERR_FORCE     _IOWR(MAGIC, 0x4f, fpga_ioctl_eccerr_t)

#define XPCIE_DEV_GLOBAL_UPDATE_MAJOR_VERSION _IO(MAGIC,   0x50)
#define XPCIE_DEV_GLOBAL_GET_MINOR_VERSION    _IOR(MAGIC,  0x51, uint32_t)

/* 0x52-0x5f : Missing number */

// Chain
#define XPCIE_DEV_CHAIN_UPDATE_TABLE_INGR     _IOW(MAGIC,  0x60, fpga_id_t)
#define XPCIE_DEV_CHAIN_UPDATE_TABLE_EGR      _IOW(MAGIC,  0x61, fpga_id_t)

#define XPCIE_DEV_CHAIN_DELETE_TABLE_INGR     _IOWR(MAGIC, 0x62, fpga_id_t)
#define XPCIE_DEV_CHAIN_DELETE_TABLE_EGR      _IOWR(MAGIC, 0x63, fpga_id_t)

#define XPCIE_DEV_CHAIN_READ_TABLE_INGR       _IOWR(MAGIC, 0x64, fpga_id_t)
#define XPCIE_DEV_CHAIN_READ_TABLE_EGR        _IOWR(MAGIC, 0x65, fpga_id_t)

#define XPCIE_DEV_CHAIN_START_MODULE          _IOW(MAGIC,  0x66, uint32_t)
#define XPCIE_DEV_CHAIN_STOP_MODULE           _IOW(MAGIC,  0x67, uint32_t)

#define XPCIE_DEV_CHAIN_SET_DDR_OFFSET_FRAME  _IOWR(MAGIC, 0x68, fpga_ioctl_extif_t)
#define XPCIE_DEV_CHAIN_GET_DDR_OFFSET_FRAME  _IOWR(MAGIC, 0x69, fpga_ioctl_chain_ddr_t)

#define XPCIE_DEV_CHAIN_GET_LATENCY_CHAIN     _IOWR(MAGIC, 0x70, fpga_ioctl_chain_latency_t)
#define XPCIE_DEV_CHAIN_GET_LATENCY_FUNC      _IOWR(MAGIC, 0x71, fpga_ioctl_chain_func_latency_t)

#define XPCIE_DEV_CHAIN_GET_CHAIN_BYTES       _IOWR(MAGIC, 0x72, fpga_ioctl_chain_bytenum_t)
#define XPCIE_DEV_CHAIN_GET_CHAIN_FRAMES      _IOWR(MAGIC, 0x73, fpga_ioctl_chain_framenum_t)
#define XPCIE_DEV_CHAIN_GET_CHAIN_BUFF        _IOWR(MAGIC, 0x74, fpga_ioctl_chain_framenum_t)
#define XPCIE_DEV_CHAIN_GET_CHAIN_BP          _IOWR(MAGIC, 0x75, fpga_ioctl_chain_framenum_t)
#define XPCIE_DEV_CHAIN_SET_CHAIN_BP_CLR      _IOWR(MAGIC, 0x76, fpga_ioctl_chain_framenum_t)

#define XPCIE_DEV_CHAIN_GET_CHK_ERR           _IOWR(MAGIC, 0x77, fpga_ioctl_err_all_t)

#define XPCIE_DEV_CHAIN_GET_ERR               _IOWR(MAGIC, 0x78, fpga_ioctl_chain_err_t)
#define XPCIE_DEV_CHAIN_SET_ERR_MASK          _IOW(MAGIC,  0x79, fpga_ioctl_chain_err_t)
#define XPCIE_DEV_CHAIN_GET_ERR_MASK          _IOWR(MAGIC, 0x7a, fpga_ioctl_chain_err_t)
#define XPCIE_DEV_CHAIN_SET_ERR_FORCE         _IOW(MAGIC,  0x7b, fpga_ioctl_chain_err_t)
#define XPCIE_DEV_CHAIN_GET_ERR_FORCE         _IOWR(MAGIC, 0x7c, fpga_ioctl_chain_err_t)
#define XPCIE_DEV_CHAIN_ERR_INS               _IOW(MAGIC,  0x7d, fpga_ioctl_chain_err_t)
#define XPCIE_DEV_CHAIN_ERR_GET_INS           _IOWR(MAGIC, 0x7e, fpga_ioctl_chain_err_t)

#define XPCIE_DEV_CHAIN_GET_ERR_PROT          _IOWR(MAGIC, 0x7f, fpga_ioctl_chain_err_prot_t)
#define XPCIE_DEV_CHAIN_SET_ERR_PROT_CLR      _IOW(MAGIC,  0x80, fpga_ioctl_chain_err_prot_t)
#define XPCIE_DEV_CHAIN_SET_ERR_PROT_MASK     _IOW(MAGIC,  0x81, fpga_ioctl_chain_err_prot_t)
#define XPCIE_DEV_CHAIN_GET_ERR_PROT_MASK     _IOWR(MAGIC, 0x82, fpga_ioctl_chain_err_prot_t)
#define XPCIE_DEV_CHAIN_SET_ERR_PROT_FORCE    _IOW(MAGIC,  0x83, fpga_ioctl_chain_err_prot_t)
#define XPCIE_DEV_CHAIN_GET_ERR_PROT_FORCE    _IOWR(MAGIC, 0x84, fpga_ioctl_chain_err_prot_t)
#define XPCIE_DEV_CHAIN_ERR_PROT_INS          _IOW(MAGIC,  0x85, fpga_ioctl_chain_err_prot_t)
#define XPCIE_DEV_CHAIN_ERR_PROT_GET_INS      _IOWR(MAGIC, 0x86, fpga_ioctl_chain_err_prot_t)

#define XPCIE_DEV_CHAIN_GET_ERR_EVT           _IOWR(MAGIC, 0x87, fpga_ioctl_chain_err_evt_t)
#define XPCIE_DEV_CHAIN_SET_ERR_EVT_CLR       _IOW(MAGIC,  0x88, fpga_ioctl_chain_err_evt_t)
#define XPCIE_DEV_CHAIN_SET_ERR_EVT_MASK      _IOW(MAGIC,  0x89, fpga_ioctl_chain_err_evt_t)
#define XPCIE_DEV_CHAIN_GET_ERR_EVT_MASK      _IOWR(MAGIC, 0x8a, fpga_ioctl_chain_err_evt_t)
#define XPCIE_DEV_CHAIN_SET_ERR_EVT_FORCE     _IOW(MAGIC,  0x8b, fpga_ioctl_chain_err_evt_t)
#define XPCIE_DEV_CHAIN_GET_ERR_EVT_FORCE     _IOWR(MAGIC, 0x8c, fpga_ioctl_chain_err_evt_t)

#define XPCIE_DEV_CHAIN_GET_ERR_STIF          _IOWR(MAGIC, 0x8d, fpga_ioctl_chain_err_stif_t)
#define XPCIE_DEV_CHAIN_SET_ERR_STIF_MASK     _IOW(MAGIC,  0x8e, fpga_ioctl_chain_err_stif_t)
#define XPCIE_DEV_CHAIN_GET_ERR_STIF_MASK     _IOWR(MAGIC, 0x8f, fpga_ioctl_chain_err_stif_t)
#define XPCIE_DEV_CHAIN_SET_ERR_STIF_FORCE    _IOW(MAGIC,  0x90, fpga_ioctl_chain_err_stif_t)
#define XPCIE_DEV_CHAIN_GET_ERR_STIF_FORCE    _IOWR(MAGIC, 0x91, fpga_ioctl_chain_err_stif_t)

#define XPCIE_DEV_CHAIN_ERR_CMDFAULT_INS      _IOW(MAGIC,  0x92, fpga_ioctl_chain_err_cmdfault_t)
#define XPCIE_DEV_CHAIN_ERR_CMDFAULT_GET_INS  _IOWR(MAGIC, 0x93, fpga_ioctl_chain_err_cmdfault_t)

#define XPCIE_DEV_CHAIN_GET_MODULE            _IOWR(MAGIC, 0x94, fpga_ioctl_chain_ctrl_t)
#define XPCIE_DEV_CHAIN_GET_MODULE_ID         _IOWR(MAGIC, 0x95, fpga_ioctl_chain_ctrl_t)
#define XPCIE_DEV_CHAIN_GET_CONNECTION        _IOWR(MAGIC, 0x96, fpga_ioctl_chain_con_status_t)

#define XPCIE_DEV_CHAIN_GET_EGR_BUSY          _IOWR(MAGIC, 0x97, fpga_ioctl_chain_framenum_t)

#define XPCIE_DEV_CHAIN_GET_ERR_TBL           _IOWR(MAGIC, 0x98, fpga_ioctl_chain_err_table_t)
#define XPCIE_DEV_CHAIN_SET_ERR_TBL_MASK      _IOW(MAGIC,  0x99, fpga_ioctl_chain_err_table_t)
#define XPCIE_DEV_CHAIN_GET_ERR_TBL_MASK      _IOWR(MAGIC, 0x9a, fpga_ioctl_chain_err_table_t)
#define XPCIE_DEV_CHAIN_SET_ERR_TBL_FORCE     _IOW(MAGIC,  0x9b, fpga_ioctl_chain_err_table_t)
#define XPCIE_DEV_CHAIN_GET_ERR_TBL_FORCE     _IOWR(MAGIC, 0x9c, fpga_ioctl_chain_err_table_t)

#define XPCIE_DEV_CHAIN_READ_SOFT_TABLE       _IOWR(MAGIC, 0x9d, fpga_ioctl_chain_ids_t)
#define XPCIE_DEV_CHAIN_RESET_SOFT_TABLE      _IO(MAGIC,   0x9e)

/* 0x9f : Missing number */

// Direct
#define XPCIE_DEV_DIRECT_START_MODULE         _IOW(MAGIC,  0xa0, uint32_t)
#define XPCIE_DEV_DIRECT_STOP_MODULE          _IOW(MAGIC,  0xa1, uint32_t)

#define XPCIE_DEV_DIRECT_GET_BYTES            _IOWR(MAGIC, 0xa2, fpga_ioctl_direct_bytenum_t)
#define XPCIE_DEV_DIRECT_GET_FRAMES           _IOWR(MAGIC, 0xa3, fpga_ioctl_direct_framenum_t)

#define XPCIE_DEV_DIRECT_GET_ERR_ALL          _IOWR(MAGIC, 0xa4, fpga_ioctl_err_all_t)

#define XPCIE_DEV_DIRECT_GET_ERR_PROT         _IOWR(MAGIC, 0xa5, fpga_ioctl_direct_err_prot_t)
#define XPCIE_DEV_DIRECT_SET_ERR_PROT_CLR     _IOW(MAGIC,  0xa6, fpga_ioctl_direct_err_prot_t)
#define XPCIE_DEV_DIRECT_SET_ERR_PROT_MASK    _IOW(MAGIC,  0xa7, fpga_ioctl_direct_err_prot_t)
#define XPCIE_DEV_DIRECT_GET_ERR_PROT_MASK    _IOWR(MAGIC, 0xa8, fpga_ioctl_direct_err_prot_t)
#define XPCIE_DEV_DIRECT_SET_ERR_PROT_FORCE   _IOW(MAGIC,  0xa9, fpga_ioctl_direct_err_prot_t)
#define XPCIE_DEV_DIRECT_GET_ERR_PROT_FORCE   _IOWR(MAGIC, 0xaa, fpga_ioctl_direct_err_prot_t)
#define XPCIE_DEV_DIRECT_ERR_PROT_INS         _IOW(MAGIC,  0xab, fpga_ioctl_direct_err_prot_t)
#define XPCIE_DEV_DIRECT_ERR_PROT_GET_INS     _IOWR(MAGIC, 0xac, fpga_ioctl_direct_err_prot_t)

#define XPCIE_DEV_DIRECT_GET_ERR_STIF         _IOWR(MAGIC, 0xad, fpga_ioctl_direct_err_stif_t)
#define XPCIE_DEV_DIRECT_SET_ERR_STIF_MASK    _IOW(MAGIC,  0xae, fpga_ioctl_direct_err_stif_t)
#define XPCIE_DEV_DIRECT_GET_ERR_STIF_MASK    _IOWR(MAGIC, 0xaf, fpga_ioctl_direct_err_stif_t)
#define XPCIE_DEV_DIRECT_SET_ERR_STIF_FORCE   _IOW(MAGIC,  0xb0, fpga_ioctl_direct_err_stif_t)
#define XPCIE_DEV_DIRECT_GET_ERR_STIF_FORCE   _IOWR(MAGIC, 0xb1, fpga_ioctl_direct_err_stif_t)

#define XPCIE_DEV_DIRECT_GET_MODULE           _IOWR(MAGIC, 0xb2, fpga_ioctl_direct_ctrl_t)
#define XPCIE_DEV_DIRECT_GET_MODULE_ID        _IOWR(MAGIC, 0xb3, fpga_ioctl_direct_ctrl_t)

/* 0xb4-0xcf : Missing number */

// CMS
#define XPCIE_DEV_CMS_GET_TEMP                _IOWR(MAGIC, 0xd0, fpga_ioctl_temp_t)

#define XPCIE_DEV_CMS_GET_POWER               _IOR(MAGIC,  0xd1, fpga_ioctl_power_t)
#define XPCIE_DEV_CMS_GET_POWER_U250          _IOR(MAGIC,  0xd2, fpga_power_t)
#define XPCIE_DEV_CMS_SET_RESET               _IOW(MAGIC,  0xd3, uint32_t)

/* 0xd4-0xdf : Missing number */

// test
/* 0xe0-0xff : Missing number */


#ifdef XPCIE_TRACE_LOG
typedef struct XPCIE_DEV_COMMAND_NAME_STRUCT{
  unsigned long cmd;
  char *name;
}CMD_NAME_T;
#define COMMAND_ELEMENT(cmd) {cmd, #cmd}
static const CMD_NAME_T CMD_NAME_TABLE[] = {
  COMMAND_ELEMENT(XPCIE_DEV_MPOLL),
  COMMAND_ELEMENT(XPCIE_DEV_DRIVER_GET_DEVICE_ID),
  COMMAND_ELEMENT(XPCIE_DEV_DRIVER_GET_VERSION),
  COMMAND_ELEMENT(XPCIE_DEV_DRIVER_GET_DEVICE_INFO),
  COMMAND_ELEMENT(XPCIE_DEV_DRIVER_SET_REFCOUNT),
  COMMAND_ELEMENT(XPCIE_DEV_DRIVER_GET_REFCOUNT),
  COMMAND_ELEMENT(XPCIE_DEV_DRIVER_SET_FPGA_UPDATE),
  COMMAND_ELEMENT(XPCIE_DEV_DRIVER_SET_REG_LOCK),
  COMMAND_ELEMENT(XPCIE_DEV_DRIVER_GET_FPGA_TYPE),
  COMMAND_ELEMENT(XPCIE_DEV_DRIVER_GET_FPGA_ADDR_MAP),
  // LLDMA
  COMMAND_ELEMENT(XPCIE_DEV_LLDMA_ALLOC_QUEUE),
  COMMAND_ELEMENT(XPCIE_DEV_LLDMA_FREE_QUEUE),
  COMMAND_ELEMENT(XPCIE_DEV_LLDMA_BIND_QUEUE),
  COMMAND_ELEMENT(XPCIE_DEV_LLDMA_GET_CH_STAT),
  COMMAND_ELEMENT(XPCIE_DEV_LLDMA_GET_CID_CHAIN),
  COMMAND_ELEMENT(XPCIE_DEV_LLDMA_ALLOC_CONNECTION),
  COMMAND_ELEMENT(XPCIE_DEV_LLDMA_FREE_CONNECTION),
  COMMAND_ELEMENT(XPCIE_DEV_LLDMA_GET_VERSION),
  COMMAND_ELEMENT(XPCIE_DEV_LLDMA_GET_UP_SIZE),
  COMMAND_ELEMENT(XPCIE_DEV_LLDMA_CTRL_DDR_BUFFER),
  COMMAND_ELEMENT(XPCIE_DEV_LLDMA_GET_RXCH_CTRL0),
  // Cms
  COMMAND_ELEMENT(XPCIE_DEV_CMS_GET_TEMP),
  COMMAND_ELEMENT(XPCIE_DEV_CMS_GET_POWER),
  COMMAND_ELEMENT(XPCIE_DEV_CMS_GET_POWER_U250),
  COMMAND_ELEMENT(XPCIE_DEV_CMS_SET_RESET),
  //Global
  COMMAND_ELEMENT(XPCIE_DEV_GLOBAL_CTRL_SOFT_RST),
  COMMAND_ELEMENT(XPCIE_DEV_GLOBAL_GET_CHK_ERR),
  COMMAND_ELEMENT(XPCIE_DEV_GLOBAL_GET_CLKDOWN),
  COMMAND_ELEMENT(XPCIE_DEV_GLOBAL_SET_CLKDOWN_CLR),
  COMMAND_ELEMENT(XPCIE_DEV_GLOBAL_GET_CLKDOWN_RAW),
  COMMAND_ELEMENT(XPCIE_DEV_GLOBAL_SET_CLKDOWN_MASK),
  COMMAND_ELEMENT(XPCIE_DEV_GLOBAL_GET_CLKDOWN_MASK),
  COMMAND_ELEMENT(XPCIE_DEV_GLOBAL_SET_CLKDOWN_FORCE),
  COMMAND_ELEMENT(XPCIE_DEV_GLOBAL_GET_CLKDOWN_FORCE),
  COMMAND_ELEMENT(XPCIE_DEV_GLOBAL_GET_ECCERR),
  COMMAND_ELEMENT(XPCIE_DEV_GLOBAL_SET_ECCERR_CLR),
  COMMAND_ELEMENT(XPCIE_DEV_GLOBAL_GET_ECCERR_RAW),
  COMMAND_ELEMENT(XPCIE_DEV_GLOBAL_SET_ECCERR_MASK),
  COMMAND_ELEMENT(XPCIE_DEV_GLOBAL_GET_ECCERR_MASK),
  COMMAND_ELEMENT(XPCIE_DEV_GLOBAL_SET_ECCERR_FORCE),
  COMMAND_ELEMENT(XPCIE_DEV_GLOBAL_GET_ECCERR_FORCE),
  COMMAND_ELEMENT(XPCIE_DEV_GLOBAL_UPDATE_MAJOR_VERSION),
  COMMAND_ELEMENT(XPCIE_DEV_GLOBAL_GET_MINOR_VERSION),
  // Chain
  COMMAND_ELEMENT(XPCIE_DEV_CHAIN_UPDATE_TABLE_INGR),
  COMMAND_ELEMENT(XPCIE_DEV_CHAIN_UPDATE_TABLE_EGR),
  COMMAND_ELEMENT(XPCIE_DEV_CHAIN_DELETE_TABLE_INGR),
  COMMAND_ELEMENT(XPCIE_DEV_CHAIN_DELETE_TABLE_EGR),
  COMMAND_ELEMENT(XPCIE_DEV_CHAIN_READ_TABLE_INGR),
  COMMAND_ELEMENT(XPCIE_DEV_CHAIN_READ_TABLE_EGR),
  COMMAND_ELEMENT(XPCIE_DEV_CHAIN_READ_SOFT_TABLE),
  COMMAND_ELEMENT(XPCIE_DEV_CHAIN_RESET_SOFT_TABLE),
  COMMAND_ELEMENT(XPCIE_DEV_CHAIN_START_MODULE),
  COMMAND_ELEMENT(XPCIE_DEV_CHAIN_STOP_MODULE),
  COMMAND_ELEMENT(XPCIE_DEV_CHAIN_SET_DDR_OFFSET_FRAME),
  COMMAND_ELEMENT(XPCIE_DEV_CHAIN_GET_DDR_OFFSET_FRAME),
  COMMAND_ELEMENT(XPCIE_DEV_CHAIN_GET_LATENCY_CHAIN),
  COMMAND_ELEMENT(XPCIE_DEV_CHAIN_GET_LATENCY_FUNC),
  COMMAND_ELEMENT(XPCIE_DEV_CHAIN_GET_CHAIN_BYTES),
  COMMAND_ELEMENT(XPCIE_DEV_CHAIN_GET_CHAIN_FRAMES),
  COMMAND_ELEMENT(XPCIE_DEV_CHAIN_GET_CHAIN_BUFF),
  COMMAND_ELEMENT(XPCIE_DEV_CHAIN_GET_CHAIN_BP),
  COMMAND_ELEMENT(XPCIE_DEV_CHAIN_SET_CHAIN_BP_CLR),
  COMMAND_ELEMENT(XPCIE_DEV_CHAIN_GET_CHK_ERR),
  COMMAND_ELEMENT(XPCIE_DEV_CHAIN_GET_ERR),
  COMMAND_ELEMENT(XPCIE_DEV_CHAIN_SET_ERR_MASK),
  COMMAND_ELEMENT(XPCIE_DEV_CHAIN_GET_ERR_MASK),
  COMMAND_ELEMENT(XPCIE_DEV_CHAIN_SET_ERR_FORCE),
  COMMAND_ELEMENT(XPCIE_DEV_CHAIN_GET_ERR_FORCE),
  COMMAND_ELEMENT(XPCIE_DEV_CHAIN_ERR_INS),
  COMMAND_ELEMENT(XPCIE_DEV_CHAIN_ERR_GET_INS),
  COMMAND_ELEMENT(XPCIE_DEV_CHAIN_GET_ERR_PROT),
  COMMAND_ELEMENT(XPCIE_DEV_CHAIN_SET_ERR_PROT_CLR),
  COMMAND_ELEMENT(XPCIE_DEV_CHAIN_SET_ERR_PROT_MASK),
  COMMAND_ELEMENT(XPCIE_DEV_CHAIN_GET_ERR_PROT_MASK),
  COMMAND_ELEMENT(XPCIE_DEV_CHAIN_SET_ERR_PROT_FORCE),
  COMMAND_ELEMENT(XPCIE_DEV_CHAIN_GET_ERR_PROT_FORCE),
  COMMAND_ELEMENT(XPCIE_DEV_CHAIN_ERR_PROT_INS),
  COMMAND_ELEMENT(XPCIE_DEV_CHAIN_ERR_PROT_GET_INS),
  COMMAND_ELEMENT(XPCIE_DEV_CHAIN_GET_ERR_EVT),
  COMMAND_ELEMENT(XPCIE_DEV_CHAIN_SET_ERR_EVT_CLR),
  COMMAND_ELEMENT(XPCIE_DEV_CHAIN_SET_ERR_EVT_MASK),
  COMMAND_ELEMENT(XPCIE_DEV_CHAIN_GET_ERR_EVT_MASK),
  COMMAND_ELEMENT(XPCIE_DEV_CHAIN_SET_ERR_EVT_FORCE),
  COMMAND_ELEMENT(XPCIE_DEV_CHAIN_GET_ERR_EVT_FORCE),
  COMMAND_ELEMENT(XPCIE_DEV_CHAIN_GET_ERR_STIF),
  COMMAND_ELEMENT(XPCIE_DEV_CHAIN_SET_ERR_STIF_MASK),
  COMMAND_ELEMENT(XPCIE_DEV_CHAIN_GET_ERR_STIF_MASK),
  COMMAND_ELEMENT(XPCIE_DEV_CHAIN_SET_ERR_STIF_FORCE),
  COMMAND_ELEMENT(XPCIE_DEV_CHAIN_GET_ERR_STIF_FORCE),
  COMMAND_ELEMENT(XPCIE_DEV_CHAIN_ERR_CMDFAULT_INS),
  COMMAND_ELEMENT(XPCIE_DEV_CHAIN_ERR_CMDFAULT_GET_INS),
  COMMAND_ELEMENT(XPCIE_DEV_CHAIN_GET_MODULE),
  COMMAND_ELEMENT(XPCIE_DEV_CHAIN_GET_MODULE_ID),
  COMMAND_ELEMENT(XPCIE_DEV_CHAIN_GET_CONNECTION),
  COMMAND_ELEMENT(XPCIE_DEV_CHAIN_GET_EGR_BUSY),
  COMMAND_ELEMENT(XPCIE_DEV_CHAIN_GET_ERR_TBL),
  COMMAND_ELEMENT(XPCIE_DEV_CHAIN_SET_ERR_TBL_MASK),
  COMMAND_ELEMENT(XPCIE_DEV_CHAIN_GET_ERR_TBL_MASK),
  COMMAND_ELEMENT(XPCIE_DEV_CHAIN_SET_ERR_TBL_FORCE),
  COMMAND_ELEMENT(XPCIE_DEV_CHAIN_GET_ERR_TBL_FORCE),
  // Direct
  COMMAND_ELEMENT(XPCIE_DEV_DIRECT_START_MODULE),
  COMMAND_ELEMENT(XPCIE_DEV_DIRECT_STOP_MODULE),
  COMMAND_ELEMENT(XPCIE_DEV_DIRECT_GET_BYTES),
  COMMAND_ELEMENT(XPCIE_DEV_DIRECT_GET_FRAMES),
  COMMAND_ELEMENT(XPCIE_DEV_DIRECT_GET_ERR_ALL),
  COMMAND_ELEMENT(XPCIE_DEV_DIRECT_GET_ERR_PROT),
  COMMAND_ELEMENT(XPCIE_DEV_DIRECT_SET_ERR_PROT_CLR),
  COMMAND_ELEMENT(XPCIE_DEV_DIRECT_SET_ERR_PROT_MASK),
  COMMAND_ELEMENT(XPCIE_DEV_DIRECT_GET_ERR_PROT_MASK),
  COMMAND_ELEMENT(XPCIE_DEV_DIRECT_SET_ERR_PROT_FORCE),
  COMMAND_ELEMENT(XPCIE_DEV_DIRECT_GET_ERR_PROT_FORCE),
  COMMAND_ELEMENT(XPCIE_DEV_DIRECT_ERR_PROT_INS),
  COMMAND_ELEMENT(XPCIE_DEV_DIRECT_ERR_PROT_GET_INS),
  COMMAND_ELEMENT(XPCIE_DEV_DIRECT_GET_ERR_STIF),
  COMMAND_ELEMENT(XPCIE_DEV_DIRECT_SET_ERR_STIF_MASK),
  COMMAND_ELEMENT(XPCIE_DEV_DIRECT_GET_ERR_STIF_MASK),
  COMMAND_ELEMENT(XPCIE_DEV_DIRECT_SET_ERR_STIF_FORCE),
  COMMAND_ELEMENT(XPCIE_DEV_DIRECT_GET_ERR_STIF_FORCE),
  COMMAND_ELEMENT(XPCIE_DEV_DIRECT_GET_MODULE),
  COMMAND_ELEMENT(XPCIE_DEV_DIRECT_GET_MODULE_ID),
  {(unsigned long)-1, ""} // Sentinel
};
#undef COMMAND_ELEMENT
/**
 * @brief Function which convert ioctl command from number to name
 * @param[in] cmd
 *   Target ioctl command
 * @return
 *   String of ioctl command
 */
static inline char *XPCIE_DEV_COMMAND_NAME(unsigned long cmd){
  int i = 0;
  while(CMD_NAME_TABLE[i].cmd != (unsigned long)-1){
    if(CMD_NAME_TABLE[i].cmd == cmd)
      break;
    i++;
  }
  return CMD_NAME_TABLE[i].name;
}
#endif

#endif  /* __XPCIE_DEVICE_H__ */
