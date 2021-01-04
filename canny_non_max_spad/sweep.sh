#!/bin/bash

declare -a lane=("1" "2" "4" "8" "16")
declare -a spad_size=("1.75" "2.5" "4" "7.75" "8.5" "16" "31.75" "32.5" "64")
declare -a spad_part=("1" "2" "4" "8" "16")

mkdir -p sweep

for l in "${lane[@]}"; do
    for ss in "${spad_size[@]}"; do
        for sp in "${spad_part[@]}"; do
            ./set_config ${l} ${ss} ${sp}

            sh run.sh
            mv stdout.gz sweep/stdout_l_${l}_ss_${ss}_sp_${sp}.gz
        done
    done
done
