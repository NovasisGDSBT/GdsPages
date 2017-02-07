#!/bin/sh

# HEADER    
echo "<meta http-equiv=\"Content-Type\" content=\"text/html; CHARSET=utf-8\">"
echo ""
echo "<html>"
echo "<head>"
echo "      <center><h3>MAIN MENU</h3></center>"
echo " </head>"
echo " <body>"
echo "     <br>"
echo "     <br>"
echo "     <center><a href=\"screentest_red.cgi\"><input type=\"button\" value=\"RED\"></center></a>"
echo "     <br>"
echo "     <center><a href=\"screentest_green.cgi\"><input type=\"button\" value=\"GREEN\"></center></a>"
echo "     <br>"
echo "     <center><a href=\"screentest_blue.cgi\"><input type=\"button\" value=\"BLUE\"></center></a>"
echo "     <br>"

echo "</body>"
echo "</html>"

/tmp/www/cgi-bin/screentest.sh
