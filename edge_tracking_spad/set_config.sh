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
out_spad_size=$(((size_in_bytes - 256) / 2))
in_spad_size=$((size_in_bytes - out_spad_size))

sed -i '/unrolling,edge_tracking,tr_loop/c\unrolling,edge_tracking,tr_loop,'"${num_lanes}" edge_tracking.cfg
sed -i '/unrolling,edge_tracking,h_loop/c\unrolling,edge_tracking,h_loop,'"${num_lanes}" edge_tracking.cfg

sed -i '/partition,cyclic,input/c\partition,cyclic,input_image_acc,'"${in_spad_size}"',1,'"${spad_parts}" edge_tracking.cfg
sed -i '/partition,cyclic,output/c\partition,cyclic,output_image_acc,'"${out_spad_size}"',1,'"${spad_parts}" edge_tracking.cfg

sed -i '/#define SPAD_SIZE_IN_KB/c\#define SPAD_SIZE_IN_KB '"${total_spad_size}" edge_tracking.h
