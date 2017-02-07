#!/bin/sh

if [ -f /tmp/www/HwSw.xml ]; then
        BOARD_REV=`xml sel -t -v "/data-set/unit/BOARD_REV"  /tmp/www/HwSw.xml`
        if [ $? -eq 0 ]  
        then
	  echo BOARD_REV=$BOARD_REV  >  /tmp/hw_version
	else
	 /tmp/www/logwrite.sh "APPA" "ERROR" "$0"  "BOARD_REV"
        fi
        
        MONITOR_SN=`xml sel -t -v "/data-set/unit/MONITOR_SN"  /tmp/www/HwSw.xml`
        if [ $? -eq 0 ]  
        then
	  echo MONITOR_SN=$MONITOR_SN  >>  /tmp/hw_version
	else
	  /tmp/www/logwrite.sh "APPA" "ERROR" "$0"  "MONITOR_SN"
        fi
        
        PRODUCTION_DATE=`xml sel -t -v "/data-set/unit/PRODUCTION_DATE"  /tmp/www/HwSw.xml`
        if [ $? -eq 0 ]  
        then
	  echo PRODUCTION_DATE=$PRODUCTION_DATE  >>  /tmp/hw_version
	else
	 /tmp/www/logwrite.sh "APPA" "ERROR" "$0"  "PRODUCTION_DATE"
  
        fi
        
        IMAGE_REV=`xml sel -t -v "/data-set/system/image_rev"  /tmp/www/HwSw.xml`
        if [ $? -eq 0 ]  
        then
	  echo IMAGE_REV=$IMAGE_REV  >  /tmp/sw_version
	else
	  /tmp/www/logwrite.sh "APPA" "ERROR" "$0"  "IMAGE_REV"

        fi
        
fi

exit 0
