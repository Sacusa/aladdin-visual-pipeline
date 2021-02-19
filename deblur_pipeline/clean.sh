#!/bin/bash
mv gem5.cfg my_gem5
rm -rf *.h *.cfg scheduler.c cacti/
mv my_gem5 gem5.cfg
