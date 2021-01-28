#!/bin/bash

if [ "$#" -ne 4 ]
then
    echo "Usage: $0 <num lanes> <width> <height> <spad_parts>"
    exit -1
fi

num_lanes=$1
width=$2
height=$3
spad_parts=$4

in_spad_size=$(((width+2) * (height+2) * 2))
out_spad_size=$((width * height * 6))

sed -i '/unrolling,isp,loop/c\unrolling,isp,loop,'"${num_lanes}"'' isp.cfg

sed -i '/partition,cyclic,input_image_acc/c\partition,cyclic,input_image_acc,'"${in_spad_size}"',1,'"${spad_parts}" isp.cfg
sed -i '/partition,cyclic,output_image_acc/c\partition,cyclic,output_image_acc,'"${out_spad_size}"',1,'"${spad_parts}" isp.cfg

sed -i '/#define OUT_SPAD_WIDTH/c\#define OUT_SPAD_WIDTH '"${width}" isp.h
sed -i '/#define OUT_SPAD_HEIGHT/c\#define OUT_SPAD_HEIGHT '"${height}" isp.h
