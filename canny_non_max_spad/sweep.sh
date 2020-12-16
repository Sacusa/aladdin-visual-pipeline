#!/bin/bash

declare -a lane=("1" "2" "4" "8" "16")
declare -a spad_part=("1" "2" "4" "8" "16")

mkdir -p sweep

for l in "${lane[@]}"; do
    sed -i '/unrolling,canny_non_max,loop/c\unrolling,canny_non_max,loop,'"${l}"'' canny_non_max.cfg

    for sp in "${spad_part[@]}"; do
        sed -i '/partition,cyclic,hypotenuse/c\partition,cyclic,hypotenuse_acc,16900,1,'"${sp}"'' canny_non_max.cfg
        sed -i '/partition,cyclic,theta/c\partition,cyclic,theta_acc,65536,1,'"${sp}"'' canny_non_max.cfg
        sed -i '/partition,cyclic,result/c\partition,cyclic,result_acc,16384,1,'"${sp}"'' canny_non_max.cfg

        sh run.sh
        mv stdout.gz sweep/stdout_l_${l}_sp_${sp}.gz
    done
done
