#!/bin/bash

if [ "$#" -ne 3 ]
then
    echo "Usage: $0 <num lanes> <total_spad_size> <spad_parts>"
    exit -1
fi

num_lanes=$1
total_spad_size=$2
spad_parts=$3

spad_size=$((total_spad_size / 3))

sed -i '/unrolling,elem_matrix,loop/c\unrolling,elem_matrix,loop,'"${num_lanes}" elem_matrix.cfg

sed -i '/partition,cyclic,arg1/c\partition,cyclic,arg1_acc,'"${spad_size}"',4,'"${spad_parts}" elem_matrix.cfg
sed -i '/partition,cyclic,arg2/c\partition,cyclic,arg2_acc,'"${spad_size}"',4,'"${spad_parts}" elem_matrix.cfg
sed -i '/partition,cyclic,result/c\partition,cyclic,result_acc,'"${spad_size}"',4,'"${spad_parts}" elem_matrix.cfg

sed -i '/#define SPAD_SIZE_IN_B/c\#define SPAD_SIZE_IN_B '"${total_spad_size}"'' elem_matrix.h
