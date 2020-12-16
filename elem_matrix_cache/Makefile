ACCEL_NAME = elem_matrix
SRCS = elem_matrix.c
BMARK_SPECIFIC_CFLAGS = -DDMA_INTERFACE_V3
export WORKLOAD=elem_matrix

include ../common/Makefile.tracer
include ../common/Makefile.gem5

build: gem5 dma-trace-binary
	./elem_matrix-instrumented

clean: clean-gem5 clean-trace
	rm -rf dddg_parse_progress.out stdout.gz out*
