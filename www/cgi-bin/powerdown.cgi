#!/bin/sh
# This kills all of X apps
echo "<meta http-equiv=\"Content-Type\" content=\"text/html; CHARSET=utf-8\">"

echo "<html>"
echo "<head>"
echo "      <center><h3>System is in low power</h3></center>"
echo " </head>"
echo " <body>"
echo "     <br>"
echo "     <br>"
echo "     <center><a href=\"main_menu.cgi\"><input type=\"button\" value=\"Back to Main Menu\"></center></a>"
echo "     <br>"

echo "</body>"
echo "</html>"
kill -9 `pidof fluxbox`
echo 1 > /sys/class/graphics/fb0/blank
