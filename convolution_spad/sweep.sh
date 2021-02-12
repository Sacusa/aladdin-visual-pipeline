#!/bin/bash

declare -a lane=("1" "2" "4" "8" "16")
declare -a spad_part=("1" "2" "4" "8" "16")

# = SPAD size in KB =   2.03    4.00    7.97    16.00   31.94   64.00
declare -a spad_size=(  "2"     "4"     "8"     "16"    "32"    "64")
declare -a spad_width=( "12"    "16"    "21"    "32"    "49"    "64")
declare -a spad_height=("7"     "12"    "20"    "28"    "38"    "60")

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
