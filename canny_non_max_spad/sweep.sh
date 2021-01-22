#!/bin/bash

declare -a lane=("1" "2" "4" "8" "16")
declare -a spad_part=("1" "2" "4" "8" "16")

# = SPAD size in KB =   1.93    3.95    8.01    16.05   31.96   64.06
declare -a spad_size=(  "2"     "4"     "8"     "16"    "32"    "64")
declare -a spad_width=( "13"    "18"    "35"    "67"    "69"    "95")
declare -a spad_height=("12"    "18"    "19"    "20"    "39"    "57")

mkdir -p sweep

for l in "${lane[@]}"; do
    for ((ss=0;ss<="${#spad_size[@]}";ss++)); do
        for sp in "${spad_part[@]}"; do
            ./set_config ${l} ${spad_width[${ss}]} ${spad_height[${ss}]} ${sp}

            sh run.sh
            mv stdout.gz sweep/stdout_l_${l}_ss_${spad_size[${ss}]}_sp_${sp}.gz
        done
    done
done
