#!/bin/sh
DATE=`date -u +"%Y-%m-%d %H:%M:%S"`
# timestamp from 01/01/1970
TIMESTAMP=`date -u +"%s"`

if [ "$1" = "DIAG" ]; then

  xml ed --inplace -s "/logging" -t elem -n "LOG"  -v "$DATE [$2] $3 $4 $5 $6" -i "/logging/LOG[last()]" -t attr -n "timestamp" -v $TIMESTAMP /tmp/www/SystemDiagnosticLog.xml

elif [ "$1" = "DREC" ]; then

  xml ed --inplace -s "/logging" -t elem -n "LOG"  -v "$DATE [$2] $3 $4 $5 $6" -i "/logging/LOG[last()]" -t attr -n "timestamp" -v $TIMESTAMP /tmp/www/DataRecording.xml

elif [ "$1" = "APPA" ]; then

  xml ed --inplace -s "/logging" -t elem -n "LOG"  -v "$DATE [$2] $3 $4 $5 $6" -i "/logging/LOG[last()]" -t attr -n "timestamp" -v $TIMESTAMP /tmp/www/AppActivityLog.xml

else

  echo "not defined"

fi


exit $?