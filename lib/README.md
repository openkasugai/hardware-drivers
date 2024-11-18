# FPGA library(libfpga)

## Introduction
- `libfpga` is a library to control FPGAs implemented LLDMA.

## Software configuration
- The table below shows Items needed to be installed and the Versions tested for `libfpga`.

|Item|Version|
|:--|:--|
|OS|Ubuntu 22.04.4 LTS|
|Kernel|5.15.0-117-generic|
|DPDK|23.11.1|
|parson|1.5.3|
|MCAP|2023.1|
|build-essential|12.9|
|python3-pip|3.10.12|
|pkg-config|0.29.2|
|meson|0.61.2.1|
|ninja|1.11.1.1|
|pyelftools|0.31|
|libnuma-dev|2.0.14.3|
|libpciaccess-dev|0.16.3|

## Setup
1. Inastall Packages if not installed.

```sh
$ sudo apt-get update
$ sudo apt-get install build-essential python3-pip pkg-config libnuma-dev libpciaccess-dev
$ sudo pip3 install meson ninja pyelftools
```

- If you want to install package with the specific version, install as follows:
```sh
$ sudo apt-get install <package>=<version>
$ sudo pip3 install <package>==<version>
```


## Build
1. Build DPDK.
	- `make dpdk` at at hardware-drivers/lib
	- See hardware-drivers/lib/DPDK/README.md when you install by manual.
2. Build MCAP tool(if you do not need to include `libfpgabs.h` you can skip this step).
	- `make mcap` at hardware-drivers/lib
	- Copy hardware-drivers/lib/MCAP/mcap into PATH for sudo(e.g. /usr/local/bin).
3. Get Parson library.
	- `make json` at hardware-drivers/lib
4. Build libfpga.
	- `make` at hardware-drivers/lib

### Build Examples
```sh
$ make dpdk mcap json
$ make
```

### Build Commands
|Command|Description|
|-|-|
|make (default)|= make all|
|make all|make clean + make static|
|make static|create libfpga.a|
|make shared|create libfpga.so(not tested)|
|make clean|clean all object files and libraries and build directory|
|make dpdk|wget DPDK and build DPDK by meson,ninja|
|make dpdk-uninstall|`rm -rf DPDK/dpdk*`|
|make mcap|build MCAP tool|
|make mcap-uninstall|`make clean -C MCAP`|
|make json|clone parson library from git|
|make json-uninstall|`cd JSON && rm -rf parson`|

## How to link
- Use pkg-config with hardware-drivers/lib/build/pkgconfig/lifpga.pc

- Example of Makefile for an application which uses libfpga
	```
	# when `pwd`=hardware-drivers/lib/test
	src := test.c
	app := test

	LIBFPGA_BUILD_DIR := ../build # Please change the valid path
	PKGCONF_PATH      := $(LIBFPGA_BUILD_DIR)/pkgconfig
	PKGCONF           := env PKG_CONFIG_PATH="$(PKGCONF_PATH)" pkg-config

	CFLAGS += $(shell $(PKGCONF) --cflags libfpga)

	LDFLAGS += $(shell $(PKGCONF) --libs libfpga)
	LDFLAGS += $(shell $(PKGCONF) --libs libdpdk) # if you need libshmem.h/liblldma.h/libdma.h

	.PHONY: build clean

	build: $(app)
	$(app): $(src)
		gcc $< -o $@ $(CFLAGS) $(LDFLAGS)

	clean:
		rm -f $(app)
	```


## Files in libfpga
- Note: partially omitted
```
hardware-drivers/lib/
 | 
 +---- build/                                      # Directory build by Makefile in default
 |
 +---- README.md                                   # THIS FILE
 +---- Makefile                                    # Makefile for libfpga
 +---- libfpga.pc.in                               # `.in` file for libfpga.pc
 |
 +---- libfpga/                                    # Directory for libfpga
 |    +---- include/                               # Header files of libfpga
 |    |    +---- libfpga_internal/                 # Internal header files of libfpga
 |    \---- src/                                   # Source files of libfpga
 |
 +---- libptu/                                     # Directory for libptu
 |    +---- README.md                              # README for libptu
 |    \---- include/                               # Header files of libptu
 |    \---- src/                                   # Source files of libptu
 |
 +---- DPDK/                                       # Directory for DPDK
 |    +---- README.md                              # README for DPDK
 |    \---- build_dpdk.sh                          # Shell script for manual DPDK install
 |
 +---- JSON/                                       # Directory for parson
 |    \---- README.md                              # README for parson
 |
 +---- MCAP/                                       # Directory for MCAP
      +---- README.md                              # README for MCAP
      \---- patch/                                 # Patch files of mcap
```
