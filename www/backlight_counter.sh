#!/bin/sh
TRUE="1"
COUNTER=0
SAVECOUNTER=0
TIMETOSAVE=300

if [ -f /tmp/backlight_on_counter ]; then
	. /tmp/backlight_on_counter
	COUNTER=$BACKLIGHT_ON_COUNTER
else
	echo "BACKLIGHT_ON_COUNTER=0" > /tmp/backlight_on_counter
	
fi

while [ "$TRUE" == "1" ]; do
	sleep 1

        DEV=`cat /tmp/www/cgi-bin/lvds_device`                   
        LEV=`cat ${DEV}/actual_brightness`                       
        ENABLE=`cat ${DEV}/bl_power`                             

	if [ $LEV -gt 0 ]; then
		if [ "$ENABLE" == "0" ]; then
			let COUNTER=$COUNTER+1
			echo "BACKLIGHT_ON_COUNTER=$COUNTER" > /tmp/backlight_on_counter
		fi
	fi
	
	DATE=`date -u`
	if [ -f /tmp/backlight_on_reset ]; then
		rm /tmp/backlight_on_reset
		echo "BACKLIGHT_ON_COUNTER=0" > /tmp/backlight_on_counter
		COUNTER=0
	fi
	let SAVECOUNTER=$SAVECOUNTER+1
	if [ $SAVECOUNTER -gt $TIMETOSAVE ]; then
		SAVECOUNTER=0
		mkdir -p /tmp/store_mountpoint
		mount /dev/mmcblk0p3 /tmp/store_mountpoint

		if [ $? -eq 0 ]  # test mount OK
		then
		  sync
		  cp /tmp/backlight_on_counter /tmp/store_mountpoint/webparams/tempfile
		  mv /tmp/store_mountpoint/webparams/tempfile /tmp/store_mountpoint/webparams/backlight_on_counter
		  umount /tmp/store_mountpoint
		  sync
		  e2fsck /dev/mmcblk0p3
		  #echo "$0" "Save"
		  #/tmp/www/logwrite.sh "APPA" "INFO" "$0" "Save counters"

		#else

		 # echo "$0" "Partition /dev/mmcblk0p3 busy"

		fi
	fi
done

