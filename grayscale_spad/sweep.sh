#!/bin/bash

declare -a lane=("1" "2" "4" "8" "16")
declare -a spad_part=("1" "2" "4" "8" "16")

# = SPAD size in KB =   2.00    4.00    8.00    16.00   32.00   64.00
declare -a spad_size=(  "2"     "4"     "8"     "16"    "32"    "64")
declare -a spad_width=( "16"    "32"    "32"    "64"    "64"    "128")
declare -a spad_height=("16"    "16"    "32"    "32"    "64"    "64")

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
