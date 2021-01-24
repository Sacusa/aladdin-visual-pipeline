#!/bin/bash

if [ "$#" -ne 4 ]
then
    echo "Usage: $0 <num lanes> <width> <height <spad_parts>"
    exit -1
fi

num_lanes=$1
width=$2
height=$3
spad_parts=$4

out_spad_size=$((width * height * 2))
in_spad_size=$((width * height * 3 * 2))

sed -i '/unrolling,grayscale,loop/c\unrolling,grayscale,loop,'"${num_lanes}" grayscale.cfg

sed -i '/partition,cyclic,input/c\partition,cyclic,input_image_acc,'"${in_spad_size}"',1,'"${spad_parts}" grayscale.cfg
sed -i '/partition,cyclic,output/c\partition,cyclic,output_image_acc,'"${out_spad_size}"',1,'"${spad_parts}" grayscale.cfg

sed -i '/#define SPAD_WIDTH/c\#define SPAD_WIDTH '"${width}" grayscale.h
sed -i '/#define SPAD_HEIGHT/c\#define SPAD_HEIGHT '"${height}" grayscale.h
