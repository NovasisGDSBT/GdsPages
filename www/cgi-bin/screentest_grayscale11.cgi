#!/bin/sh
echo "<meta http-equiv=\"Content-Type\" content=\"text/html; CHARSET=utf-8\">"

echo "<html>"
echo "<head>"
echo "      <center><h3>Fill Gray Scale 11 Levels</h3></center>"
echo " </head>"
echo " <body>"
echo "     <br>"
echo "     <br>"
echo "     <center><a href=\"do_screentest.cgi\"><input type=\"button\" value=\"Back\"></center></a>"
echo "     <br>"

echo "</body>"
echo "</html>"

/tmp/www/GdsScreenTestWrite FILL_GRAYSCALE11 >/dev/null 2>&1 &
