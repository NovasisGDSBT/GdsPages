#!/bin/sh
TRUE="1"
COUNTER=0
SAVECOUNTER=0
TIMETOSAVE=300


[ ! -f /tmp/monitor_on_counter ] && echo "MONITOR_ON_COUNTER=0" > /tmp/monitor_on_counter
if [ -f /tmp/monitor_on_counter ]; then
        . /tmp/monitor_on_counter
        COUNTER=$MONITOR_ON_COUNTER
        echo "Setting MONITOR_ON_COUNTER to 0" >> /tmp/gds_log

fi


while [ "$TRUE" == "1" ]; do
	sleep 1
	let COUNTER=$COUNTER+1
	echo "MONITOR_ON_COUNTER=$COUNTER" > /tmp/monitor_on_counter
        let SAVECOUNTER=$SAVECOUNTER+1
        if [ $SAVECOUNTER -gt $TIMETOSAVE ]; then
                SAVECOUNTER=0
 
                mkdir -p /tmp/store_mountpoint           
	        mount /dev/mmcblk0p3 /tmp/store_mountpoint
	        
		if [ $? -eq 0 ]  # test mount OK
		then
		  sync
		  cp /tmp/monitor_on_counter /tmp/store_mountpoint/webparams/tmpfile
		  mv /tmp/store_mountpoint/webparams/tmpfile /tmp/store_mountpoint/webparams/monitor_on_counter
		  umount /tmp/store_mountpoint
		  echo "$0 save" >> /tmp/gds_log
		  sync
		  e2fsck /dev/mmcblk0p3
		else
		  echo "partition /dev/mmcblk0p3 busy" >> /tmp/gds_log
		fi
		
		
		
        fi
done

