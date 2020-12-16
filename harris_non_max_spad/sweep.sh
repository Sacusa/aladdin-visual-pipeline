#!/bin/bash

declare -a lane=("1" "2" "4" "8" "16")
declare -a spad_part=("1" "2" "4" "8" "16")

mkdir -p sweep

for l in "${lane[@]}"; do
    sed -i '/unrolling,harris_non_max,img_loop/c\unrolling,harris_non_max,img_loop,'"${l}"'' harris_non_max.cfg

    for sp in "${spad_part[@]}"; do
        sed -i '/partition,cyclic,harris/c\partition,cyclic,harris_response_acc,66564,4,'"${sp}"'' harris_non_max.cfg
        sed -i '/partition,cyclic,max/c\partition,cyclic,max_values_acc,16641,1,'"${sp}"'' harris_non_max.cfg

        sh run.sh
        mv stdout.gz sweep/stdout_l_${l}_sp_${sp}.gz
    done
done
