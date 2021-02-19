#!/bin/bash
mv gem5.cfg my_gem5
rm -rf *.h scheduler.c cacti/ *.cfg
mv my_gem5 gem5.cfg
