#!/bin/bash
#=================================================
# Copyright 2024 NTT Corporation, FUJITSU LIMITED
# Licensed under the 3-Clause BSD License, see LICENSE for details.
# SPDX-License-Identifier: BSD-3-Clause
#=================================================

VERSION='3.4.3'
INSTALL_PATH='/usr/local/opencv-3.4.3'


############################################
### APT update
############################################
sudo apt-get -y update


############################################
### Install the tools
############################################
sudo apt-get install -y cmake build-essential wget unzip doxygen


############################################
### Install the dependencies for OpenCV
############################################
# gstreamer
PACKAGE=(
  libgstreamer1.0-dev libgstreamer-plugins-base1.0-dev
  gstreamer1.0-plugins-base gstreamer1.0-plugins-bad
  gstreamer1.0-plugins-good gstreamer1.0-libav
)

# video
PACKAGE+=(
  libavcodec-dev libavformat-dev libswscale-dev
  libtheora-dev libvorbis-dev libxvidcore-dev libx264-dev
  libopencore-amrnb-dev libopencore-amrwb-dev libv4l-dev
  libxine2-dev yasm
)

# media
PACKAGE+=(
  libjpeg-dev libwebp-dev libpng-dev libtiff5-dev libopenexr-dev
  libgdal-dev zlib1g-dev
)

# gui
PACKAGE+=(
  qtbase5-dev
)

# parallelism
PACKAGE+=(
  libtbb2-dev
)

# linear algebra
PACKAGE+=(
  libeigen3-dev
)

# python
PACKAGE+=(
    python3-dev python3-tk python3-numpy python-tk pylint flake8
)

# java
PACKAGE+=(
    default-jdk ant
)

# Install 
sudo apt-get install -y ${PACKAGE[@]}


############################################
### Build & Install OpenCV
############################################
wget https://github.com/opencv/opencv/archive/${VERSION}.zip && unzip ${VERSION}.zip

mkdir -p opencv-${VERSION}/build
cd opencv-${VERSION}/build/

/usr/bin/cmake \
      -DCMAKE_INSTALL_PREFIX=${INSTALL_PATH} \
      -DENABLE_PRECOMPILED_HEADERS=OFF \
      -DFORCE_VTK=ON \
      -DWITH_FFMPEG=OFF \
      -DWITH_GDAL=ON \
      -DWITH_GSTREAMER=ON \
      -DWITH_OPENGL=ON \
      -DWITH_QT=ON \
      -DWITH_TBB=ON \
      -DWITH_XINE=ON \
      ..

make -j8
sudo make install
sudo cp ../../opencv.conf /etc/ld.so.conf.d/ && sudo ldconfig

