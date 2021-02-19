ACCEL_NAME = deblur
SRCS = deblur.c scheduler.c
BMARK_SPECIFIC_CFLAGS=-DDMA_INTERFACE_V3
export WORKLOAD=isp

include ../common/Makefile.tracer
include ../common/Makefile.gem5

build: gem5 dma-trace-binary
	./deblur-instrumented

clean: clean-gem5 clean-trace
	rm -rf dddg_parse_progress.out stdout.gz out* *_trace.gz
