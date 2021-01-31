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

out_spad_size=$((width * height * 4 * 2))
in_spad_size=$(((width+4) * (height+4) * 4 * 2))

sed -i '/unrolling,convolution,loop/c\unrolling,convolution,loop,'"${num_lanes}" convolution.cfg

sed -i '/partition,cyclic,output_image/c\partition,cyclic,output_image_acc,'"${out_spad_size}"',4,'"${spad_parts}" convolution.cfg
sed -i '/partition,cyclic,input_image/c\partition,cyclic,input_image_acc,'"${in_spad_size}"',4,'"${spad_parts}" convolution.cfg

sed -i '/#define OUT_SPAD_WIDTH/c\#define OUT_SPAD_WIDTH '"${width}" convolution.h
sed -i '/#define OUT_SPAD_HEIGHT/c\#define OUT_SPAD_HEIGHT '"${height}" convolution.h
