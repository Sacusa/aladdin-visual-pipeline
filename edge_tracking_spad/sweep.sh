#!/bin/bash

declare -a lane=("1" "2" "4" "8" "16")
declare -a spad_part=("1" "2" "4" "8" "16")

# = SPAD size in KB =   2.00    4.00    7.97    16.00   32.00   64.00
declare -a spad_size=(  "2"     "4"     "8"     "16"    "32"    "64")
declare -a spad_width=( "29"    "31"    "50"    "63"    "90"    "127")
declare -a spad_height=("16"    "31"    "39"    "63"    "89"    "127")

mkdir -p sweep

for l in "${lane[@]}"; do
    for ((ss=0;ss<"${#spad_size[@]}";ss++)); do
        for sp in "${spad_part[@]}"; do
            ./set_config.sh ${l} ${spad_width[${ss}]} ${spad_height[${ss}]} ${sp}
            make clean
            make build

            sh run.sh
            mv stdout.gz sweep/stdout_l_${l}_ss_${spad_size[${ss}]}_sp_${sp}.gz
        done
    done
done
