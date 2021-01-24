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

in_spad_size=$((width * height * 4 * 2))
out_spad_size=$((width * height * 2))

sed -i '/unrolling,harris_non_max,img_loop/c\unrolling,harris_non_max,img_loop,'"${num_lanes}"'' harris_non_max.cfg

sed -i '/partition,cyclic,harris/c\partition,cyclic,harris_response_acc,'"${in_spad_size}"',4,'"${spad_parts}" harris_non_max.cfg
sed -i '/partition,cyclic,max/c\partition,cyclic,max_values_acc,'"${out_spad_size}"',1,'"${spad_parts}" harris_non_max.cfg

sed -i '/#define SPAD_WIDTH/c\#define SPAD_WIDTH '"${width}" harris_non_max.h
sed -i '/#define SPAD_HEIGHT/c\#define SPAD_HEIGHT '"${height}" harris_non_max.h
