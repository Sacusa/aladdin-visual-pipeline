ACCEL_NAME = canny
SRCS = canny.c scheduler.c
BMARK_SPECIFIC_CFLAGS=-DDMA_INTERFACE_V3
export WORKLOAD=isp

include ../common/Makefile.tracer
include ../common/Makefile.gem5

all: build
	./canny-instrumented

build: gem5 dma-trace-binary

clean: clean-gem5 clean-trace
	rm -rf dddg_parse_progress.out stdout.gz out* *_trace.gz
