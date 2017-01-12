#!/bin/sh
echo "<meta http-equiv=\"Content-Type\" content=\"text/html; CHARSET=utf-8\">"

echo "<html>"
echo "<head>"
echo "      <center><h3>Fill Square Center Reverse</h3></center>"
echo " </head>"
echo " <body>"
echo "     <br>"
echo "     <br>"
echo "     <center><a href=\"do_screentest.cgi\"><input type=\"button\" value=\"Back\"></center></a>"
echo "     <br>"

echo "</body>"
echo "</html>"

/tmp/www/GdsScreenTestWrite FILL_SQUARECENTER_REVERSE >/dev/null 2>&1 &

