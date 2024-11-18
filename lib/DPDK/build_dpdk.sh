#!/bin/bash
#=================================================
# Copyright 2024 NTT Corporation, FUJITSU LIMITED
# Licensed under the 3-Clause BSD License, see LICENSE for details.
# SPDX-License-Identifier: BSD-3-Clause
#=================================================

DPDK_DIR=./dpdk-stable-23.11.1
INSTALL_PREFIX=$(pwd)/dpdk
MESON_BUILD_OPTIONS="\
	--prefix=$INSTALL_PREFIX
	-Dplatform=generic
	-Ddisable_drivers=*/*
	-Ddisable_libs=*
	-Ddisable_apps=*
	"

cd $DPDK_DIR
if [ -e build ]; then
	rm -rf build
fi
meson $MESON_BUILD_OPTIONS build
cd build
ninja
ninja install
