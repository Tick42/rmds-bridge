#!/bin/bash
# $1 source
# $2 symbol
export WOMBAT_PATH=./
export LD_LIBRARY_PATH=./:$LD_LIBRARY_PATH
./mamalistenc -S $1 -s $2 -tport rmds_sub -m tick42rmds -threads 1 
