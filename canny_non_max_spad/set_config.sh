#!/bin/bash

if [ "$#" -ne 3 ]
then
    echo "Usage: $0 <num lanes> <total_spad_size> <spad_parts>"
    exit -1
fi

num_lanes=$1
total_spad_size=$2
spad_parts=$3

size_in_bytes=$((total_spad_size * 1024))
hypo_spad_size=$((size_in_bytes / 6))
thta_spad_size=$((4 * size_in_bytes / 6))
out_spad_size=$((size_in_bytes / 6))

sed -i '/unrolling,canny_non_max,loop/c\unrolling,canny_non_max,loop,'"${num_lanes}"'' canny_non_max.cfg

sed -i '/partition,cyclic,hypotenuse/c\partition,cyclic,hypotenuse_acc,'"${hypo_spad_size}"',1,'"${spad_parts}"'' canny_non_max.cfg
sed -i '/partition,cyclic,theta/c\partition,cyclic,theta_acc,'"${thta_spad_size}"',4,'"${spad_parts}"'' canny_non_max.cfg
sed -i '/partition,cyclic,result/c\partition,cyclic,result_acc,'"${out_spad_size}"',1,'"${spad_parts}"'' canny_non_max.cfg

sed -i '/#define SPAD_SIZE_IN_KB/c\#define SPAD_SIZE_IN_KB '"${total_spad_size}"'' canny_non_max.h
