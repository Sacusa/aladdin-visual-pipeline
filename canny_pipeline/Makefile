ACCEL_NAME = canny
SRCS = canny.c
BMARK_SPECIFIC_CFLAGS=-DDMA_INTERFACE_V3
export WORKLOAD=edge_tracking

include ../common/Makefile.tracer
include ../common/Makefile.gem5

build: gem5 dma-trace-binary
	./canny-instrumented

clean: clean-gem5 clean-trace
	rm -rf dddg_parse_progress.out stdout.gz out*
