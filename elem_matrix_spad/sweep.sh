#!/bin/bash

declare -a lane=("1" "2" "4" "8" "16")
declare -a spad_part=("1" "2" "4" "8" "16")

# = SPAD size in KB =   1.97    3.98    7.99    15.98   31.99   63.98
declare -a spad_size=(  "2"     "4"     "8"     "16"    "32"    "64")
declare -a spad_width=( "12"    "17"    "31"    "31"    "65"    "65")
declare -a spad_height=("7"     "10"    "11"    "22"    "42"    "42")

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
