# xpcie driver README

## Build

### Commands
- Commands to build xpcie driver
	|Command|Description|
	|-|-|
	|make (default)|=make normal|
	|make all|make clean+make default|
	|make clean|Delete driver file(`xpcie.ko`) and object files|
	|make normal|- Normal log|
	|make trace|(may decrease performance)<br>- Normal log<br>- Function Trace log(may decrease performance)|
	|make debug|(may decrease performance)<br>- Normal log<br>- Function Trace log<br>- Register Access log|

### Parameters
- Switch build option by parameters below:
	- Parameters can be set with the `make` arguments or overwritten directly in the Makefile.

	|Parameter|Value|Description|
	|-|-|-|
	|ENABLE_MODULE_DIRECT    |1: enabled (default)<br>otherwise: disabled |Enable Direct Transfer Adapter module|
	|ENABLE_MODULE_PTU       |1: enabled (default)<br>otherwise: disabled |Enable PTU module|

### Commands with NOT recommend parameters
- These commands are as follows:
	- It is impossible to build xpcie driver only with these options.
	  Any commands from `Commands` are required to create xpcie driver.
	  For example: `make rw-nolock default`
	- It is possible to use some of these commands at the same time.

	|Command|Description|
	|-|-|
	|make <Build Command> rw-nolock|(not recommended)Change default status of read()/write() from LOCKED to FREE.|
	|make <Build Command> extif-inv|Build driver supporting FPGAs with inverted extenal interface ID<br>default:LLDMA(0)/PTU(1)|
	|make <Build Command> wo-direct|Build driver supporting FPGAs WithOut direct transfer adaptar<br>=`ENABLE_MODULE_DIRECT=0`|
	|make <Build Command> wo-nw|Build driver supporting FPGAs WithOut NetWork(PTU)<br>=`ENABLE_MODULE_PTU=0`|

### Caution
- Need to change DDR offset when you want to change external interface.
	- See `XPCIE_FPGA_DDR_VALUE_AXI_EXTIF*` in `chain/xpcie_regs_chain.h`.


## Examples
- Command to create `xpcie.ko` and load xpcie driver
	- You may wait for 2~3 minutes when you try to load xpcie driver.
	```sh
	$ make
	$ sudo insmod xpcie.ko
	```

- Command to create `xpcie.ko` without direct transfer adapter and network, and load xpcie driver
	- You may wait for 2~3 minutes when you try to load xpcie driver.
	```sh
	$ make normal wo-direct wo-nw
	$ sudo insmod xpcie.ko
	```

- Command to unload xpcie driver
	```sh
	$ sudo rmmod xpcie
	```

### When you failed to load xpcie driver
- You can see the reason why driver failed to be loaded at dmesg or kernel log.

- You may see below log when you try to load xpcie driver without writing a valid bitstream.
	- Please write a valid bitstream with `CMS` before you try to load xpcie driver.
		- `CMS` means `Card Management Subsystem`.
```
$ sudo insmod ../driver/xpcie.ko
insmod: ERROR: could not insert module ../driver/xpcie.ko: No such device
$ sudo dmesg --ctime
[Mon Jan 1 00:00:00 2024] xpcie: xpcie Driver Ver:(type:00)01.00.00-00
[Mon Jan 1 00:00:00 2024] xpcie:  Options=ENABLE_MODULE_GLOBAL;ENABLE_MODULE_CHAIN;ENABLE_MODULE_DIRECT;ENABLE_MODULE_LLDMA;ENABLE_MODULE_PTU;ENABLE_MODULE_CONV;ENABLE_MODULE_FUNC;ENABLE_MODULE_CMS;ENABLE_REFCOUNT_GLOBAL;ENABLE_SETTING_IN_DRIVER;
[Mon Jan 1 00:00:00 2024] xpcie 0000:1f:00.0: found FPGA
[Mon Jan 1 00:00:00 2024] xpcie: xpcie_fpga_dev_init
[Mon Jan 1 00:00:00 2024] xpcie: Dev_id=0
[Mon Jan 1 00:00:00 2024] xpcie: Address length =0x0000000000200000
[Mon Jan 1 00:00:00 2024] xpcie: Base hw address=0x00000000a0000000
[Mon Jan 1 00:00:00 2024] xpcie: Ioremap address=0xff6879ace0c00000
[Mon Jan 1 00:00:00 2024] xpcie: ParentBitstream=00000000
[Mon Jan 1 00:00:00 2024] xpcie:  * Failed to get base address as Module FPGA.
[Mon Jan 1 00:00:00 2024] xpcie:  FPGA[00]'s MAP is being considered as unknown
[Mon Jan 1 00:00:00 2024] xpcie: Failed to get FPGA's Address Map in (-14)...
[Mon Jan 1 00:00:00 2024] xpcie: xpcie_fpga_dev_close : Dev_id = 0
[Mon Jan 1 00:00:00 2024] xpcie: XPCIE DMA Device not found.
```


## Build log example
```
$ make
###
### About LOG into syslog                           // cmd:normal,trace,debug
###  - Default
###
### Device file name : /dev/xpcie_<serial_id>       // cmd:dev-id
###
### read()/write() : LOCKED                         // cmd:rw-nolock
###
### SUPPORT FPGA TYPE                               // supporting FPGA type
###  Enabled feature
###   * module : GLOBAL
###   * module : CHAIN
###   * module : DIRECT
###   * module : LLDMA
###   * module : PTU
###   * module : CONV
###   * module : FUNC
###   * module : CMS
###
### Setting parameters when insmod : True
###  - LLDMA DDR4 buffer address
###  - LLDMA DDR4 buffer size
###  - CMS reset
###
### Modules which have refcount
###   * GLOBAL
###
### Driver Type           : 0x00                    // reserved
### Driver Version(major) : 0x01
### Driver Version(minor) : 0x00
### Driver Revision       : 0x00
### Driver Patch          : 0x00
###
### Building xpcie.ko...
###
```

## Files
```
 +---- xpcie_device.c                     # Source file for insmod/rmmod
 +---- xpcie_device_fops.c                # Source file for interface of system calls
 +---- xpcie_device.h                     # Header file for user space(command/struct/enum for ioctl,...)
 |
 +---- libxpcie.c                         # Source file for functions without accessing register
 +---- libxpcie.h                         # Header file for functions without accessing register
 |
 +---- global
 |    +---- xpcie_device_fops_global.c    # Source fils for ioctl of Global module
 |    +---- libxpcie_global.c             # Source file for functions for Global module
 |    +---- libxpcie_global.h             # Header file for this directory
 |    \---- xpcie_regs_global.h           # Header file for register map of Global module
 |
 +---- chain
 |    +---- xpcie_device_fops_chain.c     # Source fils for ioctl of Chain module
 |    +---- libxpcie_chain.c              # Source file for functions for Chain module
 |    +---- libxpcie_chain.h              # Header file for this directory
 |    \---- xpcie_regs_chain.h            # Header file for register map of Chain module
 |
 +---- direct
 |    +---- xpcie_device_fops_direct.c    # Source fils for ioctl of Direct transfer module
 |    +---- libxpcie_direct.c             # Source file for functions for Direct transfer module
 |    +---- libxpcie_direct.h             # Header file for this directory
 |    \---- xpcie_regs_direct.h           # Header file for register map of Direct transfer module
 |
 +---- lldma
 |    +---- xpcie_device_fops_lldma.c     # Source fils for ioctl of LLDMA module
 |    +---- libxpcie_lldma.c              # Source file for functions for LLDMA module
 |    +---- libxpcie_lldma.h              # Header file for this directory
 |    \---- xpcie_regs_lldma.h            # Header file for register map of LLDMA module
 |
 +---- ptu
 |    +---- libxpcie_ptu.c                # Source file for functions for PTU module
 |    +---- libxpcie_ptu.h                # Header file for this directory
 |    \---- xpcie_regs_ptu.h              # Header file for register map of PTU module
 |
 +---- conv
 |    +---- libxpcie_conv.c               # Source file for functions for Conversion module
 |    +---- libxpcie_conv.h               # Header file for this directory
 |    \---- xpcie_regs_conv.h             # Header file for register map of Conversion module
 |
 +---- func
 |    +---- libxpcie_func.c               # Source file for functions for Function module
 |    +---- libxpcie_func.h               # Header file for this directory
 |    \---- xpcie_regs_func.h             # Header file for register map of Function module
 |
 +---- cms
 |    +---- xpcie_device_fops_cms.c       # Source fils for ioctl of CMS module
 |    +---- libxpcie_cms.c                # Source file for functions for CMS module
 |    +---- libxpcie_cms.h                # Header file for this directory
 |    \---- xpcie_regs_cms.h              # Header file for register map of CMS module
 |
 +---- Makefile                           # Makefile for xpcie driver
 \---- README.md                          # THIS FILE
```
