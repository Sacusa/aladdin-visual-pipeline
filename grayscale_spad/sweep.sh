#!/bin/bash

declare -a lane=("1" "2" "4" "8" "16")
declare -a spad_part=("1" "2" "4" "8" "16")

mkdir -p sweep

for l in "${lane[@]}"; do
    sed -i '/unrolling,grayscale,loop/c\unrolling,grayscale,loop,'"${l}"'' grayscale.cfg

    for sp in "${spad_part[@]}"; do
        sed -i '/partition,cyclic,input/c\partition,cyclic,input_image_acc,49152,1,'"${sp}"'' grayscale.cfg
        sed -i '/partition,cyclic,output/c\partition,cyclic,output_image_acc,16384,1,'"${sp}"'' grayscale.cfg

        sh run.sh
        mv stdout.gz sweep/stdout_l_${l}_sp_${sp}.gz
    done
done
