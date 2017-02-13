#!/bin/sh
# selezionare un range di valori tramite il timestamp_end=start_timestamp+spacetime
TIMESTAMP_END=0
FILENAME=$(echo "uploadCbmLog_"`date -u +"%Y%m%d_%H%M%S"`".cbm")


if [ "$#" -eq 3 ]; then

      let TIMESTAMP_END=$2+$3

      if [ "$1" = "S" ]; then

      xml sel -t -m "logging/LOG[@timestamp>=$2 and @timestamp<=$TIMESTAMP_END]" -v . -n /tmp/www/SystemDiagnosticLog.xml >> /tmp/$FILENAME
      echo "log file created:"/tmp/$FILENAME
      elif [ "$1" = "D" ]; then

      xml sel -t -m "logging/LOG[@timestamp>=$2 and @timestamp<=$TIMESTAMP_END]" -v . -n /tmp/www/DataRecording.xml >> /tmp/$FILENAME
      echo "log file created:"/tmp/$FILENAME
      elif [ "$1" = "A" ]; then

      xml sel -t -m "logging/LOG[@timestamp>=$2 and @timestamp<=$TIMESTAMP_END]" -v . -n /tmp/www/AppActivityLog.xml >> /tmp/$FILENAME
      echo "log file created:"/tmp/$FILENAME
      else

	echo "$0" "[S or D or A]" "TIMESTAMP_START TIMESPAN"
	echo "S: SystemDiagnosticLog"
	echo "D: DataRecording" 
	echo "A: AppActivityLog" 

      fi


else

      echo "$0" "[S or D or A]" "TIMESTAMP_START TIMESPAN"
      echo "S: SystemDiagnosticLog"
      echo "D: DataRecording" 
      echo "A: AppActivityLog" 
fi