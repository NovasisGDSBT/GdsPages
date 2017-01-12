#!/bin/sh
# This kills all of X apps
echo "<meta http-equiv=\"Content-Type\" content=\"text/html; CHARSET=utf-8\">"

echo "<html>"
echo "<head>"
echo "      <center><h3>System Info</h3></center>"
echo " </head>"
echo " <body>"
echo " <h3>Memory Usage</h3>"
free -m | tr "\n" "W" | sed 's/W/<br>/g'
echo " <h3>Processor Load</h3>"
top -n 1 | grep CPU | tr "\n" "W" | sed 's/W/<br>/g'
echo " <h3>FLASH Memory Usage</h3>"
mount /dev/mmcblk0p1 /mnt
df -h | grep mmcblk0p1 | sed 's/\/mnt//g' | sed 's/\/dev\///g'
umount /mnt
echo "<br>"
mount /dev/mmcblk0p2 /mnt
df -h | grep mmcblk0p2 | sed 's/\/mnt//g' | sed 's/\/dev\///g'
umount /mnt
echo "<br>"

	mkdir -p /tmp/store_mountpoint
	while true; do 
	  mount /dev/mmcblk0p3 /tmp/store_mountpoint
	  if [ $? -eq 0 ]  # test mount OK
	  then
	    sync
	    df -h | grep mmcblk0p3 | sed 's/\/tmp.*//g' | sed 's/\/dev\///g'
	    break #
	  else
	    sleep 2       # wait 2 second to access resource
	  fi
	done	
	umount /tmp/store_mountpoint
	e2fsck /dev/mmcblk0p3

echo "<br>"
echo "     <center><a href=\"main_menu.cgi\"><input type=\"button\" value=\"Back to Main Menu\"></center></a>"
echo "     <br>"
e2fsck /dev/mmcblk0p3

echo "</body>"
echo "</html>"
