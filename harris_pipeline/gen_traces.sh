#!/bin/bash

gen_trace () {
    cp -r ../scheduler/* .
    sed -i '/WORKLOAD/c\export WORKLOAD='"${1}"'' Makefile
    make clean-trace
    make dma-trace-binary
    ./harris-instrumented
}

gen_trace convolution
gen_trace elem_matrix
gen_trace grayscale
gen_trace harris_non_max
gen_trace isp
