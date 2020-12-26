#!/bin/bash

gen_trace () {
    cp -r ../scheduler/* .
    sed -i '/WORKLOAD/c\export WORKLOAD='"${1}"'' Makefile
    make clean-trace
    make dma-trace-binary
    ./canny-instrumented
}

gen_trace canny_non_max
gen_trace edge_tracking
gen_trace elem_matrix
gen_trace grayscale
gen_trace isp
