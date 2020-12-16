#!/bin/bash

declare -a lane=("1" "2" "4" "8" "16")
declare -a spad_part=("1" "2" "4" "8" "16")

mkdir -p sweep

for l in "${lane[@]}"; do
    sed -i '/unrolling,edge_tracking,tr_loop/c\unrolling,edge_tracking,tr_loop,'"${l}"'' edge_tracking.cfg
    sed -i '/unrolling,edge_tracking,h_loop/c\unrolling,edge_tracking,h_loop,'"${l}"'' edge_tracking.cfg

    for sp in "${spad_part[@]}"; do
        sed -i '/partition,cyclic,input/c\partition,cyclic,input_image_acc,16900,1,'"${sp}"'' edge_tracking.cfg
        sed -i '/partition,cyclic,output/c\partition,cyclic,output_image_acc,16384,1,'"${sp}"'' edge_tracking.cfg

        sh run.sh
        mv stdout.gz sweep/stdout_l_${l}_sp_${sp}.gz
    done
done
