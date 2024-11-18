# MCAP tool README

## Introduction
- This file describes changes and notes.
- Default mcap cannot specify the target device,
	so we changed source code to allow this to select the device.
- We use patch files to change source code.

## Patch source codes and build mcap
### Build pciutils
```sh
$ git clone https://github.com/pciutils/pciutils.git -b v3.13.0 --depth 1
$ make -C pciutils HWDB=no
```

### Build mcap
```sh
$ git clone https://github.com/Xilinx/embeddedsw.git -b xilinx_v2023.1 --depth 1
$ patch embeddedsw/mcap/linux/mcap.c patch/mcap.c.patch -o mcap.c
$ patch embeddedsw/mcap/linux/mcap_lib.c patch/mcap_lib.c.patch -o mcap_lib.c
$ patch embeddedsw/mcap/linux/mcap_lib.h patch/mcap_lib.h.patch -o mcap_lib.h
$ cp embeddedsw/mcap/linux/Makefile ./Makefile
$ make PCIUTILS_PATH=pciutils
```

## How to use
### Command
```sh
$ sudo ./mcap -s <slot> -x <device_id> -p <file_name> [-E]
```

### Options
```
Options:
        -x              Specify MCAP Device Id in hex (MANDATORY)
        -s   bb:dd.f    Specify Device Id in hex (MANDATORY)
                          bb:busID, dd:devID, f:funcID
        -p    <file>    Program Bitstream (.bin/.bit/.rbt)
        -E              Get Exit status when Program Bitstream(-p)
```


## Examples
- bash is expected.
### PCI device id
- When PCI device id of FPGA is 903f, you can get slot number as follows.
	```sh
	$ lspci -d :903f
	1f:00.0 Memory controller: Xilinx Corporation Device 903f
	8b:00.0 Memory controller: Xilinx Corporation Device 903f
	```

### Success case
- A few tens of seconds after "Xilinx MCAP device found" is output,
	"FPGA Configuration Done!!" is output.
	```sh
	$ sudo ./mcap -s 1f:00.0 -x 903f -p ~/bitstreams/tandem_filter_resize.bit
	Xilinx MCAP device found (1f:00.0)
	FPGA Configuration Done!!
	```
- When -E options is used, the retval is 0
	```sh
	$ sudo ./mcap -s 1f:00.0 -x 903f -p ~/bitstreams/tandem_filter_resize.bit -E
	Xilinx MCAP device found (1f:00.0)
	FPGA Configuration Done!!
	$ echo $?
	0
	```

### Failure case(execute without superuser do)
- Immediately after "Xilinx MCAP device found" is output,
	"Unable to get the Register Base" is output.
	```sh
	$ ./mcap -s 1f:00.0 -x 903f -p ~/bitstreams/tandem_filter_resize.bit
	Xilinx MCAP device found (1f:00.0)
	Unable to get the Register Base
	```
- When -E options is used, the retval is 1
	```sh
	$ ./mcap -s 1f:00.0 -x 903f -p ~/bitstreams/tandem_filter_resize.bit -E
	Xilinx MCAP device found (1f:00.0)
	Unable to get the Register Base
	$ echo $?
	1
	```

### Failure case(bitstream file not exist)
- Immediately after "Xilinx MCAP device found" is output,
	this app finish.
	```sh
	$ sudo ./mcap -s 1f:00.0 -x 903f -p dummy.bit
	Xilinx MCAP device found (1f:00.0)
	```
- When -E options is used, the retval is 130((unsinged char)-126)
	```sh
	$ sudo ./mcap -s 1f:00.0 -x 903f -p dummy.bit -E
	Xilinx MCAP device found (1f:00.0)
	$ echo $?
	130
	```

## Patch
- Mcap source files in this library is changed for libfpga
	- mcap.c
		- Add `-s` option to specify the device.
		- Add `-E` option to get an error.
		- Change num of arguments for checking argc.
	- mcap_lib.h
		- Add including header files to delete warning when build mcap
			- #include <unistd.h>
		- Apply for specifying the device.
	- mcap_lib.c
		- Apply for specifying the device.
		- Change num of arguments for checking argc.

## Original source for this directory
- [mcap](https://github.com/Xilinx/embeddedsw/tree/master/mcap/linux)
	- Tag : xilinx_v2023.1
- [pciutils](https://github.com/pciutils/pciutils)
	- Tag : v3.13.0
