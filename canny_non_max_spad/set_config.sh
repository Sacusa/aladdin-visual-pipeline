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

hypo_spad_size=$(((width + 2) * (height + 2) * 2))
thta_spad_size=$((width * height * 4 * 2))
out_spad_size=$((width * height * 2))

sed -i '/unrolling,canny_non_max,loop/c\unrolling,canny_non_max,loop,'"${num_lanes}" canny_non_max.cfg

sed -i '/partition,cyclic,hypotenuse/c\partition,cyclic,hypotenuse_acc,'"${hypo_spad_size}"',1,'"${spad_parts}" canny_non_max.cfg
sed -i '/partition,cyclic,theta/c\partition,cyclic,theta_acc,'"${thta_spad_size}"',4,'"${spad_parts}" canny_non_max.cfg
sed -i '/partition,cyclic,result/c\partition,cyclic,result_acc,'"${out_spad_size}"',1,'"${spad_parts}" canny_non_max.cfg

sed -i '/#define TR_SPAD_WIDTH/c\#define TR_SPAD_WIDTH '"${width}" canny_non_max.h
sed -i '/#define TR_SPAD_HEIGHT/c\#define TR_SPAD_HEIGHT '"${height}" canny_non_max.h
