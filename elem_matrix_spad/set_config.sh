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

spad_size=$((width * height * 2 * 4))

sed -i '/unrolling,elem_matrix,loop/c\unrolling,elem_matrix,loop,'"${num_lanes}" elem_matrix.cfg

sed -i '/partition,cyclic,arg1/c\partition,cyclic,arg1_acc,'"${spad_size}"',4,'"${spad_parts}" elem_matrix.cfg
sed -i '/partition,cyclic,arg2/c\partition,cyclic,arg2_acc,'"${spad_size}"',4,'"${spad_parts}" elem_matrix.cfg
sed -i '/partition,cyclic,result/c\partition,cyclic,result_acc,'"${spad_size}"',4,'"${spad_parts}" elem_matrix.cfg

sed -i '/#define SPAD_WIDTH/c\#define SPAD_WIDTH '"${width}"'' elem_matrix.h
sed -i '/#define SPAD_HEIGHT/c\#define SPAD_HEIGHT '"${height}"'' elem_matrix.h
