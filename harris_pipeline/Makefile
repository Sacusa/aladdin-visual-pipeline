ACCEL_NAME = harris
SRCS = harris.c
BMARK_SPECIFIC_CFLAGS=-DDMA_INTERFACE_V3
export WORKLOAD=harris_non_max

include ../common/Makefile.tracer
include ../common/Makefile.gem5

build: gem5 dma-trace-binary
	./harris-instrumented

clean: clean-gem5 clean-trace
	rm -rf dddg_parse_progress.out stdout.gz out*
