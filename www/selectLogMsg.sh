#!/bin/sh
# selezionare un range di valori tramite il timestamp_end=start_timestamp+spacetime
TIMESTAMP_END=0
FILENAME=$(echo "uploadCbmLog_"`date -u +"%Y%m%d_%H%M%S"`".cbm")

let TIMESTAMP_END=$1+$2

if [ "$1" = "S" ]; then

xml sel -t -m "logging/LOG[@timestamp>=$1 and @timestamp<=$TIMESTAMP_END]" -v . -n /tmp/www/SystemDiagnosticLog.xml >> /tmp/$FILENAME

elif [ "$1" = "D" ]; then

xml sel -t -m "logging/LOG[@timestamp>=$1 and @timestamp<=$TIMESTAMP_END]" -v . -n /tmp/www/DataRecording.xml >> /tmp/$FILENAME

elif [ "$1" = "A" ]; then

xml sel -t -m "logging/LOG[@timestamp>=$1 and @timestamp<=$TIMESTAMP_END]" -v . -n /tmp/www/AppActivityLog.xml >> /tmp/$FILENAME

else

  echo "not defined"

fi

echo "log file created:"/tmp/$FILENAME