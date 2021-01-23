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

out_spad_size=$((width * height * 2))
in_spad_size=$(((width+2) * (height+2) * 2))

sed -i '/unrolling,edge_tracking,tr_loop/c\unrolling,edge_tracking,tr_loop,'"${num_lanes}" edge_tracking.cfg
sed -i '/unrolling,edge_tracking,h_loop/c\unrolling,edge_tracking,h_loop,'"${num_lanes}" edge_tracking.cfg

sed -i '/partition,cyclic,input/c\partition,cyclic,input_image_acc,'"${in_spad_size}"',1,'"${spad_parts}" edge_tracking.cfg
sed -i '/partition,cyclic,output/c\partition,cyclic,output_image_acc,'"${out_spad_size}"',1,'"${spad_parts}" edge_tracking.cfg

sed -i '/#define OUT_SPAD_WIDTH/c\#define OUT_SPAD_WIDTH '"${width}" edge_tracking.h
sed -i '/#define OUT_SPAD_HEIGHT/c\#define OUT_SPAD_HEIGHT '"${height}" edge_tracking.h
