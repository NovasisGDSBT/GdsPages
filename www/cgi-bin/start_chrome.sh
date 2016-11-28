#!/bin/sh
CHROME=`pidof chrome`
if [ "$CHROME" == "" ]; then
        echo "Starting Chrome" > /tmp/chrome_start_web.log
else
        echo "Restarting Chrome" > /tmp/chrome_start_web.log
        kill -9 `pidof fluxbox`
        sleep 1
fi
su -c startx &
echo 0 > /sys/class/graphics/fb0/blank

