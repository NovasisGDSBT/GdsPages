#!/bin/sh
# This kills all of X apps
echo "<meta http-equiv=\"Content-Type\" content=\"text/html; CHARSET=utf-8\">"

echo "<html>"
echo "<head>"
echo "      <center><h3>Update Kernel and File System</h3></center>"
echo " </head>"
echo " <body>"
kill -9 `pidof fluxbox`
echo 1 > /sys/class/graphics/fb0/blank
/bin/update_kernel > /tmp/update.log
/bin/update_fs >> /tmp/update.log
/bin/update_GDS_code >> /tmp/update.log
echo "Update successfull<br>"
echo " Updated items : Kernel, File System, GDS Application<br>"
echo "     <center><a href=\"main_menu.cgi\"><input type=\"button\" value=\"Back to Main Menu\"></center></a>"
echo "     <br>"

echo "</body>"
echo "</html>"
