#!/bin/bash

if [ "$#" -ne 3 ]
then
    echo "Usage: $0 <num lanes> <dim> <spad_parts>"
    exit -1
fi

num_lanes=$1
dim=$2
spad_parts=$3

hypo_spad_size=$(((dim + 2) * (dim + 2)))
thta_spad_size=$((dim * dim * 4))
out_spad_size=$((dim * dim))

sed -i '/unrolling,canny_non_max,loop/c\unrolling,canny_non_max,loop,'"${num_lanes}" canny_non_max.cfg

sed -i '/partition,cyclic,hypotenuse/c\partition,cyclic,hypotenuse_acc,'"${hypo_spad_size}"',1,'"${spad_parts}" canny_non_max.cfg
sed -i '/partition,cyclic,theta/c\partition,cyclic,theta_acc,'"${thta_spad_size}"',4,'"${spad_parts}" canny_non_max.cfg
sed -i '/partition,cyclic,result/c\partition,cyclic,result_acc,'"${out_spad_size}"',1,'"${spad_parts}" canny_non_max.cfg

sed -i '/#define TR_SPAD_DIM/c\#define TR_SPAD_DIM '"${dim}" canny_non_max.h
