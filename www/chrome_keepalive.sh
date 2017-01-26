#!/bin/sh
TRUE="1"
COUNT=0
sleep 30
while [ "${TRUE}" == "1" ]; do
	sleep 2
	PIDS=`pidof chrome`
	COUNT=0
	echo $PIDS > /tmp/chrome_pids
	for i in $PIDS; do
		let COUNT=$COUNT+1
	done
	if [ $COUNT -le 3 ]; then
		# First crash api_mod contains 0.
		# Second crash contains 1, so update wdog_api_mod
		echo 0 > /sys/class/gpio/gpio159/value
		cp /tmp/api_mod /tmp/wdog_api_mod
		kill -9 `pidof chrome`
		sleep 1
		[ -f /etc/sysconfig/chromium_var ] && . /etc/sysconfig/chromium_var
		[ ! -f /etc/sysconfig/chromium_var ] &&  CHROMIUM_SERVER="http://127.0.0.1/default_page.html"
		DISPLAY=":0.0"  google-chrome -load-extension=/tmp/www/ChromeExtension --disk-cache-dir=/dev/null --disable-low-res-tiling --kiosk ${CHROMIUM_SERVER} & 
		echo 1 > /tmp/api_mod
	        sleep 15
	else
		echo 1 > /sys/class/gpio/gpio159/value
        fi
done

