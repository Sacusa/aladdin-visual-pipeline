#!/bin/bash

cp -r ../scheduler/* .
make gem5
source gen_traces.sh
