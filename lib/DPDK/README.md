# DPDK README for libfpga

## Requirements
```sh
$ sudo apt install build-essential
$ sudo apt install python3-pip
$ sudo apt install pkg-config
$ sudo pip3 install meson ninja
$ sudo pip3 install pyelftools
$ sudo apt install libnuma-dev
```

## Download dpdk source archive
```sh
$ wget http://fast.dpdk.org/rel/dpdk-23.11.1.tar.xz
```

## Build & install
```sh
$ tar Jxvf dpdk-23.11.1.tar.xz
$ ./build_dpdk.sh
```
