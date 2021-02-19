#!/bin/bash

# backup gem5.cfg
mv gem5.cfg gem5_my

# copy scheduler
cp -r ../scheduler/* .
rm gem5.cfg
mv gem5_my gem5.cfg

# build
make gem5
