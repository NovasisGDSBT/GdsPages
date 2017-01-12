#!/bin/sh

/tmp/www/GdsScreenTestWrite STOP >/dev/null 2>&1 &
sleep 1
#su -c /usr/bin/startx &
TEMP_IP=`ifconfig | grep Bcast | awk '{ print $2 }' | sed 's/addr://g'`
CURRENT_IP=`echo $TEMP_IP | awk '{print $1}'`

echo "<html>"
echo"  <head>"
echo "    <title>Redirect Page</title>"
echo "    <meta http-equiv=\"refresh\" content=\"1;URL=http://${CURRENT_IP}:8080/cgi-bin/main_menu.cgi\">"
echo "  </head>"
echo "  <body>"
echo "    <p>"
echo "    Waiting for screen restart<br>"
echo "    </p>"
echo "  </body>"
echo "</html>"
exit 0

