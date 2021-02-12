#!/bin/bash

declare -a lane=("1" "2" "4" "8" "16")
declare -a spad_part=("1" "2" "4" "8" "16")

# = SPAD size in KB =   2.01    3.96    7.98    15.98   32.01   63.99
declare -a spad_size=(  "2"     "4"     "8"     "16"    "32"    "64")
declare -a spad_width=( "20"    "22"    "38"    "50"    "72"    "90")
declare -a spad_height=("12"    "22"    "26"    "40"    "56"    "90")

mkdir -p sweep

for ((ss=0;ss<"${#spad_size[@]}";ss++)); do
    ./set_config.sh 1 ${spad_width[${ss}]} ${spad_height[${ss}]} 1
    make clean
    make build
    for l in "${lane[@]}"; do
        for sp in "${spad_part[@]}"; do
            ./set_config.sh ${l} ${spad_width[${ss}]} ${spad_height[${ss}]} ${sp}
            sh run.sh
            mv stdout.gz sweep/stdout_l_${l}_ss_${spad_size[${ss}]}_sp_${sp}.gz
        done
    done
done
