#!/bin/sh
TRUE="1"

SAVECOUNTER=0
TIMETOSAVE=50

SPACETIME=0
FILEZISE_KBLIMIT=5
	        
#  24 hh buffer log message timestamp from 1/01/1970
SPACETIME_LIMIT=170000




while [ "$TRUE" == "1" ]; do
    sleep 6
    # select first and last element
    FIRST_TIMESTAMP=`xml sel -t -m "logging/LOG[position()=1]" -v @timestamp -n /tmp/www/gds_log.xml`
    LAST_TIMESTAMP=`xml sel -t -m "logging/LOG[last()]" -v @timestamp -n /tmp/www/gds_log.xml`

    #read log file size in KB
    FILESIZE=`du -k /tmp/www/gds_log.xml | cut -f1`

    let SPACETIME=$LAST_TIMESTAMP-$FIRST_TIMESTAMP
    
#    echo "*********************SPACETIME="$SPACETIME
    if [ $SPACETIME -ge $SPACETIME_LIMIT -o $FILESIZE -ge $FILEZISE_KBLIMIT ]
    then
      #remove first line
      echo "*********************REMOVE="$?
      xml ed --inplace -d "/logging/LOG[position()=1]" /tmp/www/gds_log.xml
    fi
    
    let SAVECOUNTER=$SAVECOUNTER+1
    
#    echo "*********************SAVECOUNTER="$SAVECOUNTER
    if [ $SAVECOUNTER -gt $TIMETOSAVE ]
    then
	SAVECOUNTER=0
        	        
	mkdir -p /tmp/log_mountpoint           
	mount /dev/mmcblk0p2 /tmp/log_mountpoint
	if [ $? -eq 0 ]  # test mount OK
	then
	  sync
	  cp /tmp/www/gds_log.xml /tmp/log_mountpoint/tmpfile
	  mv /tmp/log_mountpoint/tmpfile /tmp/log_mountpoint/gds_log.xml
	  umount /tmp/log_mountpoint
	  #/tmp/www/logwrite.sh "INFO" "$0" "Save"
	  sync
	  e2fsck /dev/mmcblk0p2
	else
	  /tmp/www/logwrite.sh "INFO" "$0" "Partition /dev/mmcblk0p2 busy"
	fi
    fi
   
done

