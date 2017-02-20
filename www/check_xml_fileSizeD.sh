#!/bin/sh
TRUE="1"

SAVECOUNTER=0
TIMETOSAVE=50

SPACETIME=0
FILEZISE_KBLIMIT=10000
	        
#  24 hh buffer log message timestamp from 1/01/1970
SPACETIME_LIMIT=170000




while [ "$TRUE" == "1" ]; do
    sleep 6
    # select first and last element
    FIRST_TIMESTAMP=`xml sel -t -m "logging/LOG[position()=1]" -v @timestamp -n /tmp/www/DataRecording.xml`
    LAST_TIMESTAMP=`xml sel -t -m "logging/LOG[last()]" -v @timestamp -n /tmp/www/DataRecording.xml`

    #read log file size in KB
    FILESIZE=`du -k /tmp/www/DataRecording.xml | cut -f1`

    if [ -n "$FIRST_TIMESTAMP" -a -n "$LAST_TIMESTAMP" ]
    then
      let SPACETIME=$LAST_TIMESTAMP-$FIRST_TIMESTAMP
    fi
#    echo "*********************SPACETIME="$SPACETIME
    if [ $SPACETIME -ge $SPACETIME_LIMIT -o $FILESIZE -ge $FILEZISE_KBLIMIT ]
    then
      xml ed --inplace -d "/logging/LOG[position()=1]" /tmp/www/DataRecording.xml
    fi
    
    let SAVECOUNTER=$SAVECOUNTER+1
    
#    echo "*********************SAVECOUNTER="$SAVECOUNTER
    if [ $SAVECOUNTER -gt $TIMETOSAVE ]
    then
	let SAVECOUNTER=0
        	        
	mkdir -p /tmp/log_mountpoint           
	mount /dev/mmcblk0p2 /tmp/log_mountpoint
	if [ $? -eq 0 ]  # test mount OK
	then
	  sync
	  cp /tmp/www/DataRecording.xml /tmp/log_mountpoint/tmpfileDataRecording
	  mv /tmp/log_mountpoint/tmpfileDataRecording /tmp/log_mountpoint/DataRecording.xml
	  umount /tmp/log_mountpoint
	  sync
	  e2fsck /dev/mmcblk0p2
	else
	  echo "ERROR" "$0" "DataRecording /dev/mmcblk0p2 busy"
	fi
    fi
   
done

