#!/bin/bash

# backup gem5.cfg
mv gem5.cfg gem5_my

# copy scheduler
cp -r ../scheduler/* .

# build
make gem5
source gen_traces.sh

# cleanup and restore gem5.cfg
rm -rf *.h *.cfg scheduler.c cacti/
mv gem5_my gem5.cfg
