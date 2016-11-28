#!/bin/sh

# HEADER    
echo "<meta http-equiv=\"Content-Type\" content=\"text/html; CHARSET=utf-8\">"
echo ""
echo "<html>"
echo "<head>"
echo "      <center><h3>Screen Tests</h3></center>"
echo " </head>"
echo " <body>"
echo "     <br>"
echo "     <br>"
echo "     <center><a href=\"screentest_red.cgi\"><input type=\"button\" value=\"RED\"></center></a>"
echo "     <center><a href=\"screentest_green.cgi\"><input type=\"button\" value=\"GREEN\"></center></a>"
echo "     <center><a href=\"screentest_blue.cgi\"><input type=\"button\" value=\"BLUE\"></center></a>"
echo "     <center><a href=\"screentest_black.cgi\"><input type=\"button\" value=\"BLACK\"></center></a>"
echo "     <center><a href=\"screentest_white.cgi\"><input type=\"button\" value=\"WHITE\"></center></a>"
echo "     <center><a href=\"screentest_grayscale12.cgi\"><input type=\"button\" value=\"GRAYSCALE 12 LEVELS\"></center></a>"
echo "     <center><a href=\"screentest_grayscale11.cgi\"><input type=\"button\" value=\"GRAYSCALE 11 LEVELS\"></center></a>"
echo "     <center><a href=\"screentest_borders.cgi\"><input type=\"button\" value=\"WHITE BORDER\"></center></a>"
echo "     <center><a href=\"screentest_squareleft.cgi\"><input type=\"button\" value=\"SQUARE LEFT\"></center></a>"
echo "     <center><a href=\"screentest_squarecenter.cgi\"><input type=\"button\" value=\"SQUARE CENTER\"></center></a>"
echo "     <center><a href=\"screentest_squareright.cgi\"><input type=\"button\" value=\"SQUARE RIGHT\"></center></a>"
echo "     <center><a href=\"screentest_squarecenter_reverse.cgi\"><input type=\"button\" value=\"SQUARE CENTER REVERSE\"></center></a>"
echo "     <center><a href=\"screentest_diag.cgi\"><input type=\"button\" value=\"DIAG\"></center></a>"
echo "     <center><a href=\"screentest_loop.cgi\"><input type=\"button\" value=\"LOOP\"></center></a>"
echo "     <br>"
echo "     <center><a href=\"do_start_chrome.cgi\"><input type=\"button\" value=\"Back\"></center></a>"
echo "</body>"
echo "</html>"

echo 0 > /sys/class/graphics/fb0/blank
/tmp/www/cgi-bin/screentest.sh
exit 0

