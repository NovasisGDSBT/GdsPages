#!/bin/sh

IPCLIENT=`netstat -nat | awk '/8080/ && /ESTABLISHED/ {print $5}' | cut -d ":" -f1`
#MESS=`tail -1 /var/log/lighttpd-access.log | grep 200 |cut -f1 -d" "`

sleep 1
/tmp/www/logwrite.sh "DREC" "INFO" "WEB - Successful Login from:$IPCLIENT" &


echo "<meta http-equiv=\"refresh\" content=\"1; url=/cgi-bin/main_menu.cgi\">" 
