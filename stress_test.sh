#!/bin/bash
export ARG_NUMNER=$1
export INPUT_FILE=$2
export OUTPUT_FILE=$3
echo "$0: $@" >> $OUTPUT_FILE
cat $INPUT_FILE >> $OUTPUT_FILE

# MacOS stress test
yes > /dev/null&
sleep 30
killall yes
