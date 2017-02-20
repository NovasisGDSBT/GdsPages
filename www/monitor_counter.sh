#!/bin/sh
TRUE="1"
COUNTER=0
SAVECOUNTER=0
TIMETOSAVE=300


[ ! -f /tmp/monitor_on_counter ] && echo "MONITOR_ON_COUNTER=0" > /tmp/monitor_on_counter
if [ -f /tmp/monitor_on_counter ]; then
        . /tmp/monitor_on_counter
        COUNTER=$MONITOR_ON_COUNTER
      #  /tmp/www/logwrite.sh "APPA"  "INFO" "$0" "Setting MONITOR_ON_COUNTER=s0"
fi


while [ "$TRUE" == "1" ]; do
	sleep 1

        if [ -f /tmp/monitor_on_reset ]; then
                rm /tmp/monitor_on_reset		
		echo "MONITOR_ON_COUNTER=0" > /tmp/monitor_on_counter
        	
                COUNTER=0
	else
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
        		  
			  sync
			  e2fsck /dev/mmcblk0p3
			#else
        		  #echo "APPA"  "INFO" "$0" "Partition /dev/mmcblk0p3 busy"
			fi
        	fi
        fi
done

