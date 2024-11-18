# dbgreg
### Version:1.0.0

## Build
- Need to build libfpga at first.
- When libfpga is not build, build libfpga by `make` in this repository as follows:
	- make dpdk(Inastall DPDK)
	- make mcap(Build MCAP)
	- make json(Get parson)
	- make(Build libfpga)
```
make
```

## Tested version
- driver：v1.0.0-0
- libfpga：v1.0.0-0


## Execute
```
./reg32w <device_file_name|serial_id> <address(hex:0-0x13ffff)> <data(hex)> [data(hex)]...
./reg32r <device_file_name|serial_id> <address(hex:0-0x13ffff)> [size(dec)]
```

### Argument
|parameter|requirement|description|
|-|-|-|
|device_file_name<br>serial_id|mandatory|Path of the device file or its serial_id|
|address|mandatory|Address to read/write|
|data|mandatory|Value to write(4byte)<br>At least one value must be set|
|size|arbitary|Size to read<br>Rounded to the nearest 4-byte unit|


## Example
- Command and Output
```
$ ./reg32r /dev/xpcie_21320621V01M 200 32
offset:        0        4        8       12
 0200 : 0000ffff 00000000 00000002 00000000
 0210 : 00000000 02000000 00000000 00000000
```
```
$ ./reg32w /dev/xpcie_21320621V01M 20c 1
```
```
$ ./reg32r 21320621V01M 200 32
offset:        0        4        8       12
 0200 : 0000ffff 00000000 00000002 00000001
 0210 : 00000000 02010000 00000000 00000000
```
