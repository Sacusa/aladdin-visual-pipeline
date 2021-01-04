#!/bin/bash

if [ "$#" -ne 5 ]
then
    echo "Usage: $0 <num lanes> <cache_size> <cache_line_size> <cache_assoc> <cache_ports>"
    exit -1
fi

num_lanes=$1
cache_size=$2
cache_line_size=$3
cache_assoc=$4
cache_ports=$5

sed -i '/unrolling,edge_tracking,tr_loop/c\unrolling,edge_tracking,tr_loop,'"${num_lanes}" edge_tracking.cfg
sed -i '/unrolling,edge_tracking,h_loop/c\unrolling,edge_tracking,h_loop,'"${num_lanes}" edge_tracking.cfg

sed -i '/cache_size/c\cache_size = '"${cache_size}"'kB'       gem5.cfg
sed -i '/-size/c\-size (bytes) '"$((${cache_size} * 1024))"'' cacti/cache.cfg

sed -i '/cache_line_sz/c\cache_line_sz = '"${cache_line_size}"''   gem5.cfg
sed -i '/-block size/c\-block size (bytes) '"${cache_line_size}"'' cacti/cache.cfg

sed -i '/cache_assoc/c\cache_assoc = '"${cache_assoc}"''     gem5.cfg
sed -i '/-associativity/c\-associativity '"${cache_assoc}"'' cacti/cache.cfg

if [ ${cache_ports} -eq "1" ]
then
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
elif [ ${cache_ports} -eq "2" ]
then
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
else
    read_ports=$((3 * cache_ports / 4))
    write_ports=$((cache_ports / 4))

    sed -i '/cache_bandwidth/c\cache_bandwidth = '"${cache_ports}" gem5.cfg
    sed -i '/load_bandwidth/c\load_bandwidth = '"${read_ports}"    gem5.cfg
    sed -i '/store_bandwidth/c\store_bandwidth = '"${write_ports}" gem5.cfg
    sed -i '/-read-write port/c\-read-write port 0'                     cacti/cache.cfg
    sed -i '/-exclusive write/c\-exclusive write port '"${write_ports}" cacti/cache.cfg
    sed -i '/-exclusive read/c\-exclusive read port '"${read_ports}"    cacti/cache.cfg
    sed -i '/-read-write port/c\-read-write port '"${read_ports}"  cacti/lq.cfg
    sed -i '/-search port/c\-search port '"${read_ports}"          cacti/lq.cfg
    sed -i '/-read-write port/c\-read-write port '"${write_ports}" cacti/sq.cfg
    sed -i '/-search port/c\-search port '"${write_ports}"         cacti/sq.cfg
fi
