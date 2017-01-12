#!/bin/sh
echo "<meta http-equiv=\"Content-Type\" content=\"text/html; CHARSET=utf-8\">"

echo "<html>"
echo "<head>"
echo "      <center><h3>Start Chrome</h3></center>"
echo " </head>"
echo " <body>"
/tmp/www/GdsScreenTestWrite STOP
sleep 1
touch /tmp/start_chrome
echo " Chrome started"
echo "     <center><a href=\"main_menu.cgi\"><input type=\"button\" value=\"Back to Main Menu\"></center></a>"
echo "</body>"
echo "</html>"

exit 0
