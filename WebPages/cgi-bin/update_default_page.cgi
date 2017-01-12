#!/bin/sh
echo "<meta http-equiv=\"Content-Type\" content=\"text/html; CHARSET=utf-8\">"

echo "<html>"
echo "<head>"
echo "      <center><h3>Update Kernel and File System</h3></center>"
echo " </head>"
echo " <body>"

/bin/get_exec default_page.tar
cd /tmp/www
mv ../default_page.tar .
/bin/tar xf default_page.tar
rm -rf test_default_page
mv default_page test_default_page
rm -rf /tmp/tempmount
mkdir /tmp/tempmount
mount /dev/mmcblk0p3 /tmp/tempmount
rm -rf /tmp/tempmount/default_page
cp -r test_default_page /tmp/tempmount/default_page
umount /tmp/tempmount

echo "Update successfull<br>"
echo "     <center><a href=\"main_menu.cgi\"><input type=\"button\" value=\"Back to Main Menu\"></center></a>"
echo "     <br>"

echo "</body>"
echo "</html>"

