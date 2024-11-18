#! /bin/sh
#=================================================
# Copyright 2024 NTT Corporation, FUJITSU LIMITED
# Licensed under the 3-Clause BSD License, see LICENSE for details.
# SPDX-License-Identifier: BSD-3-Clause
#=================================================
set -e

export MAGICK_THREAD_LIMIT=1

while getopts i:t: OPT
do
  case $OPT in

     t) VAR=$OPTARG
        ;;
     i) VAR2=$OPTARG
        ;;

  esac
done 

if [ ${#VAR} -eq 0 ] || [ ${#VAR2} -eq 0 ]; then
    echo 'Parameter Error.'
    echo 'usage: ./run_flash.sh -t filename(mcs) -i targetfpgano(0 or 1)'
    exit 1
fi

#echo $VAR
#echo $VAR2

cp -rfp run_flash_write.tcl original.tcl

sed -i -e "s/hogehoge/$VAR/g" ./original.tcl 
sed -i -e "s/hoge2/$VAR2/g" ./original.tcl 

vivado -mode batch -source original.tcl

rm -f original.tcl
rm -rf project_flash


