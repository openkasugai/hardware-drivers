# hardware-drivers (for Experimental Branch)

> [!WARNING]  
> This software must be used in combination with the one in the `hardware-design` repository on the branch of the same name. (Cannot be used with main branch)

## Introduction

`hardware-drivers` are libraries, drivers, and sample program for controlling OpenKasugai Hardware in the experimental branch.

**This software is for the experimental branch only.**

## Directories

```
hardware-drivers :
       -+- include     : C header files
        +- src         : C++ source files
        +- test        : test codes
        +- lib         : generate for libraries after build.
```

## Build procedure

### Preparations

- configure FPGA bitstream. (Please refer to the hardware-design repository)

- install cmake
  - need to build googletest.
  - Don't source Vitis environment, because required libraries will changed.

### Build Drivers and Libraries

- build at the top of the repository
  ```
  $ make src 
  ```

### Install Drivers

- install before build tests
  ```
  $ make install-drivers
  ```
  - created some device files started 'xse?\_'
  - if FPGA design has any function with axilite port, check the device file for the deivce file.

### Build Tests

- build at the top of the repository
  ```
  $ make test
  ```

## License

|Item|Path|License|
|:--|:--|:--|
|TestCode|`/test/`|BSD 3-Clause License|
|Tools|`/src/tools/`|BSD 3-Clause License|
|Library|`/src/lib/`|BSD 3-Clause License|
|Driver|`/src/drivers/`|GNU General Public License v2.0|