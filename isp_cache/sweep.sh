#!/bin/bash

declare -a lane=("1" "2" "4" "8" "16")
declare -a cache_size=("2" "4" "8" "16" "32" "64")
declare -a cache_line_size=("16" "32" "64")
declare -a cache_assoc=("4" "8")

mkdir -p sweep

function do_sweep {
    for l in "${lane[@]}"; do
        sed -i '/unrolling,isp,loop/c\unrolling,isp,loop,'"${l}"'' isp.cfg

        for cs in "${cache_size[@]}"; do
            sed -i '/cache_size/c\cache_size = '"${cs}"'kB' gem5.cfg
            sed -i '/-size/c\-size (bytes) '"$((${cs} * 1024))"'' cacti/cache.cfg

            for cls in "${cache_line_size[@]}"; do
                sed -i '/cache_line_sz/c\cache_line_sz = '"${cls}"'' gem5.cfg
                sed -i '/-block size/c\-block size (bytes) '"${cls}"'' cacti/cache.cfg

                for ca in "${cache_assoc[@]}"; do
                    sed -i '/cache_assoc/c\cache_assoc = '"${ca}"'' gem5.cfg
                    sed -i '/-associativity/c\-associativity '"${ca}"'' cacti/cache.cfg

                    sh run.sh
                    mv stdout.gz sweep/stdout_l_${l}_cs_${cs}_cls_${cls}_ca_${ca}_cp_${1}.gz
                done
            done
        done
    done
}

# 1 cache port
sed -i '/cache_bandwidth/c\cache_bandwidth = 1' gem5.cfg
sed -i '/load_bandwidth/c\load_bandwidth = 1'   gem5.cfg
sed -i '/store_bandwidth/c\store_bandwidth = 1' gem5.cfg
sed -i '/-read-write port/c\-read-write port 1'      cacti/cache.cfg
sed -i '/-exclusive write/c\-exclusive write port 0' cacti/cache.cfg
sed -i '/-exclusive read/c\-exclusive read port 0'   cacti/cache.cfg
sed -i '/-read-write port/c\-read-write port 1' cacti/lq.cfg
sed -i '/-search port/c\-search port 1'         cacti/lq.cfg
sed -i '/-read-write port/c\-read-write port 1' cacti/sq.cfg
sed -i '/-search port/c\-search port 1'         cacti/sq.cfg
do_sweep 1

# 2 cache ports
sed -i '/cache_bandwidth/c\cache_bandwidth = 2' gem5.cfg
sed -i '/load_bandwidth/c\load_bandwidth = 1'   gem5.cfg
sed -i '/store_bandwidth/c\store_bandwidth = 1' gem5.cfg
sed -i '/-read-write port/c\-read-write port 0'      cacti/cache.cfg
sed -i '/-exclusive write/c\-exclusive write port 1' cacti/cache.cfg
sed -i '/-exclusive read/c\-exclusive read port 1'   cacti/cache.cfg
sed -i '/-read-write port/c\-read-write port 1' cacti/lq.cfg
sed -i '/-search port/c\-search port 1'         cacti/lq.cfg
sed -i '/-read-write port/c\-read-write port 1' cacti/sq.cfg
sed -i '/-search port/c\-search port 1'         cacti/sq.cfg
do_sweep 2

# 4 cache ports
sed -i '/cache_bandwidth/c\cache_bandwidth = 4' gem5.cfg
sed -i '/load_bandwidth/c\load_bandwidth = 3'   gem5.cfg
sed -i '/store_bandwidth/c\store_bandwidth = 1' gem5.cfg
sed -i '/-read-write port/c\-read-write port 0'      cacti/cache.cfg
sed -i '/-exclusive write/c\-exclusive write port 1' cacti/cache.cfg
sed -i '/-exclusive read/c\-exclusive read port 3'   cacti/cache.cfg
sed -i '/-read-write port/c\-read-write port 3' cacti/lq.cfg
sed -i '/-search port/c\-search port 3'         cacti/lq.cfg
sed -i '/-read-write port/c\-read-write port 1' cacti/sq.cfg
sed -i '/-search port/c\-search port 1'         cacti/sq.cfg
do_sweep 4

# 8 cache ports
sed -i '/cache_bandwidth/c\cache_bandwidth = 8' gem5.cfg
sed -i '/load_bandwidth/c\load_bandwidth = 6'   gem5.cfg
sed -i '/store_bandwidth/c\store_bandwidth = 2' gem5.cfg
sed -i '/-read-write port/c\-read-write port 0'      cacti/cache.cfg
sed -i '/-exclusive write/c\-exclusive write port 2' cacti/cache.cfg
sed -i '/-exclusive read/c\-exclusive read port 6'   cacti/cache.cfg
sed -i '/-read-write port/c\-read-write port 6' cacti/lq.cfg
sed -i '/-search port/c\-search port 6'         cacti/lq.cfg
sed -i '/-read-write port/c\-read-write port 2' cacti/sq.cfg
sed -i '/-search port/c\-search port 2'         cacti/sq.cfg
do_sweep 8
