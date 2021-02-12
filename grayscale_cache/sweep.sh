#!/bin/bash

declare -a lane=("1" "2" "4" "8" "16")
declare -a cache_size=("2" "4" "8" "16" "32" "64")
declare -a cache_line_size=("16" "32" "64")
declare -a cache_assoc=("4" "8")
declare -a cache_ports=("1" "2" "4" "8")

mkdir -p sweep

for l in "${lane[@]}"; do
    for cs in "${cache_size[@]}"; do
        for cls in "${cache_line_size[@]}"; do
            for ca in "${cache_assoc[@]}"; do
                for cp in "${cache_ports[@]}"; do
                    ./set_config ${l} ${cs} ${cls} ${ca} ${cp}

                    sh run.sh
                    mv stdout.gz sweep/stdout_l_${l}_cs_${cs}_cls_${cls}_ca_${ca}_cp_${cp}.gz
                done
            done
        done
    done
done
