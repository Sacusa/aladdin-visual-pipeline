#!/bin/bash

declare -a lane=("1" "2" "4" "8" "16")
declare -a spad_part=("1" "2" "4" "8" "16")

mkdir -p sweep

for l in "${lane[@]}"; do
    sed -i '/unrolling,elem_matrix,loop/c\unrolling,elem_matrix,loop,'"${l}"'' elem_matrix.cfg

    for sp in "${spad_part[@]}"; do
        sed -i '/partition,cyclic,arg1/c\partition,cyclic,arg1_acc,65536,4,'"${sp}"'' elem_matrix.cfg
        sed -i '/partition,cyclic,arg2/c\partition,cyclic,arg2_acc,65536,4,'"${sp}"'' elem_matrix.cfg
        sed -i '/partition,cyclic,result/c\partition,cyclic,result_acc,65536,4,'"${sp}"'' elem_matrix.cfg

        sh run.sh
        mv stdout.gz sweep/stdout_l_${l}_sp_${sp}.gz
    done
done
