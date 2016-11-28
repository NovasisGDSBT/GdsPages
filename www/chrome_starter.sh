#!/bin/sh
TRUE="1"
while [ "$TRUE" == "1" ]; do
        if [ -f /tmp/start_chrome ]; then
                rm /tmp/start_chrome
                kill -9 `pidof fluxbox`
                sleep 1
                DISPLAY=":0.0" startx &
        fi
        sleep 1
done

