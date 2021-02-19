#!/bin/bash

gen_trace () {
    mv gem5.cfg my_gem5
    cp -r ../scheduler/* .
    mv my_gem5 gem5.cfg
    sed -i '/WORKLOAD/c\export WORKLOAD='"${1}"'' Makefile
    make clean-trace
    make dma-trace-binary
    ./canny-instrumented
}

gen_trace canny_non_max
gen_trace convolution
gen_trace edge_tracking
gen_trace elem_matrix
gen_trace grayscale
gen_trace isp
