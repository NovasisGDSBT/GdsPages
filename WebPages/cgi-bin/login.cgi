#!/bin/sh

sleep 1
IPCLIENT=`cat  /var/log/lighttpd-access.log  | grep login.cgi | tail -1 | awk '{print $1}'`
#MESS=`tail -1 /var/log/lighttpd-access.log | grep 200 |cut -f1 -d" "`
if [ $? -eq 0 ]  # test mount OK
then
/tmp/www/logwrite.sh "DREC" "INFO" "WEB - Successful Login from:$IPCLIENT" &
fi
echo "<meta http-equiv=\"refresh\" content=\"1; url=/cgi-bin/main_menu.cgi\">"

