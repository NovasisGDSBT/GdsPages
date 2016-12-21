#!/bin/sh
DATE=`date -u +"%Y-%m-%d %H:%M:%S"`
# timestamp from 01/01/1970
TIMESTAMP=`date -u +"%s"`
#echo "$DATE : $1 $2 $3" >> /tmp/gds_log
xml ed --inplace -s "/logging" -t elem -n "LOG"  -v "$DATE [$1] $2 $3 $4" -i "/logging/LOG[last()]" -t attr -n "timestamp" -v $TIMESTAMP /tmp/www/gds_log.xml

exit $?