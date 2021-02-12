#!/bin/bash

declare -a lane=("1" "2" "4" "8" "16")
declare -a spad_part=("1" "2" "4" "8" "16")

# = SPAD size in KB =   1.93    3.96    8.00    16.00   31.99   63.98
declare -a spad_size=(  "2"     "4"     "8"     "16"    "32"    "84")
declare -a spad_width=( "33"    "27"    "39"    "42"    "78"    "78")
declare -a spad_height=("6"     "15"    "21"    "39"    "42"    "57")

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
