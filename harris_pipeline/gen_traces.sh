#!/bin/bash

gen_trace () {
    sed -i '/WORKLOAD/c\export WORKLOAD='"${1}"'' Makefile
    make clean
    make build
    mv dynamic_trace.gz ${1}_trace.gz
}

gen_trace elem_matrix
gen_trace grayscale
gen_trace harris_non_max
gen_trace isp
