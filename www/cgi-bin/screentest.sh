#!/bin/sh

AS=`pidof fluxbox`
if ! [ "$AS" == "" ]; then
	kill -9 `pidof fluxbox`
	sleep 1
fi
AS=`pidof X`
if ! [ "$AS" == "" ]; then
	kill -9 `pidof X`
	sleep 1
fi
AS=`pidof GdsScreenTest`
if [ "$AS" == "" ]; then
	cd /tmp
	/tmp/www/GdsScreenTest &
	sleep 1
	/tmp/www/GdsScreenTestWrite START
	sleep 1
fi
echo 0 > /sys/class/graphics/fb0/blank
/tmp/www/NovaCSC f /tmp/rgb_matrix
/tmp/www/GdsScreenTestWrite FILL_GRAY

