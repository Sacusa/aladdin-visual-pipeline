#!/bin/bash

declare -a lane=("1" "2" "4" "8" "16")
declare -a spad_size=("2.25" "3.75" "8.25" "15.75" "32.25" "63.75")
declare -a spad_part=("1" "2" "4" "8" "16")

mkdir -p sweep

for l in "${lane[@]}"; do
    for ss in "${spad_size[@]}"; do
        for sp in "${spad_part[@]}"; do
            ./set_config ${l} ${}

            sh run.sh
            mv stdout.gz sweep/stdout_l_${l}_sp_${sp}.gz
        done
    done
done
