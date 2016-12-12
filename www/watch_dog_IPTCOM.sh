#!/bin/sh
TRUE="1"
COUNT=0
sleep 10
while [ "${TRUE}" == "1" ]; do
	sleep 2
	pidof GDSBT_iptcom >/dev/null 2>&1
	if [ "$?" = "1" ]; then
		COUNTER=`cat /tmp/wdog_counter`
		let COUNTER=$COUNTER+1
		echo $COUNTER > /tmp/wdog_counter
		rm -rf /tmp/wdog_CNTR_DIR >/dev/null 2>&1
		mkdir -p /tmp/wdog_CNTR_DIR
		mount /dev/mmcblk0p3 /tmp/wdog_CNTR_DIR
		if [ $? -eq 0 ]; then  # test mount OK
			cp /tmp/wdog_counter /tmp/wdog_CNTR_DIR/webparams
			umount /tmp/wdog_CNTR_DIR
			echo "Watch Dog !!!"
			sleep 2
			reboot
		fi
		let COUNTER=$COUNTER-1
	fi
done
