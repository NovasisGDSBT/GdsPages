#!/bin/sh
# selezionare un range di valori tramite il timestamp_end=start_timestamp+spacetime
TIMESTAMP_END=0
FILENAME=$(echo "uploadCbmLog_"`date -u +"%Y%m%d_%H%M%S"`".cbm")

let TIMESTAMP_END=$1+$2

xml sel -t -m "logging/LOG[@timestamp>=$1 and @timestamp<=$TIMESTAMP_END]" -v . -n /tmp/www/gds_log.xml >> /tmp/$FILENAME

echo "log file created:"/tmp/$FILENAME