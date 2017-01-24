#!/bin/sh
selected()
{
	echo "$1 $2"
	if [ "$1" == "$2" ]; then
		echo "selected"
	fi
}

TIMEZONE=`cat /etc/timezone`
# HEADER    
echo "<meta http-equiv=\"Content-Type\" content=\"text/html; CHARSET=utf-8\">"

echo "<html>"
echo "<head>"
echo "      <center><h3>Set Time Zone</h3></center>"
echo " </head>"
echo " <body>"

if [ "$REQUEST_METHOD" == "POST" ];then
        read QUERY_STRING
        saveIFS=$IFS
        IFS='&'
        set -- $QUERY_STRING
	#echo "Number of param is $#<br>"
	TIMEZONE=`echo "$1" | sed 's/%2F/\//g' | sed 's/timezone=//g'`
	echo "Time Zone set to $TIMEZONE<br>"
	echo $TIMEZONE > /etc/timezone
	HERE=`pwd`
	cd /etc
	rm localtime
	ln -s ../usr/share/zoneinfo/$TIMEZONE localtime
	cd $HERE
	
	mkdir  -p /tmp/store_mountpoint
	mount /dev/mmcblk0p3 /tmp/store_mountpoint
	if [ $? -eq 0 ]  # test mount OK
	then
	  sync
	  cp /etc/timezone /tmp/store_mountpoint/webparams/.
	  umount /tmp/store_mountpoint
	  e2fsck /dev/mmcblk0p3
	else
	  echo "store_mountpoint error: timezone no stored"
	fi
	
fi

echo "<select name=\"timezone\" form=\"timeform\">"
echo "	<option value=\"Europe/Casablanca\"";selected $TIMEZONE "Europe/Casablanca"; echo ">(GMT+00:00) Casablanca, Monrovia, Reykjavik</option>"
echo "	<option value=\"Europe/Dublin\"";    selected $TIMEZONE "Europe/Dublin";     echo ">(GMT+00:00) Greenwich Mean Time : Dublin, Edinburgh, Lisbon, London</option>"
echo "	<option value=\"Europe/Rome\"";      selected $TIMEZONE "Europe/Rome";       echo ">(GMT+01:00) Amsterdam, Berlin, Bern, Rome, Stockholm, Vienna </option>"
echo "	<option value=\"Europe/Belgrade\"";  selected $TIMEZONE "Europe/Belgrade";   echo ">(GMT+01:00) Belgrade, Bratislava, Budapest, Ljubljana, Prague</option>"
echo "	<option value=\"Europe/Brussels\"";  selected $TIMEZONE "Europe/Brussels";   echo ">(GMT+01:00) Brussels, Copenhagen, Madrid, Paris</option>"
echo "	<option value=\"Europe/Zagreb\"";    selected $TIMEZONE "Europe/Zagreb";     echo ">(GMT+01:00) Sarajevo, Skopje, Warsaw, Zagreb</option>"
echo "	<option value=\"Europe/Athens\"";    selected $TIMEZONE "Europe/Athens";     echo ">(GMT+02:00) Athens, Bucharest, Istanbul</option>"
echo "	<option value=\"Europe/Helsinki\"";  selected $TIMEZONE "Europe/Helsinki";   echo ">(GMT+02:00) Helsinki, Kyiv, Riga, Sofia, Tallinn, Vilnius</option>"
echo "	<option value=\"Europe/Minsk\"";     selected $TIMEZONE "Europe/Minsk";      echo ">(GMT+02:00) Minsk</option>"
echo "	<option value=\"Europe/Moscow\"";    selected $TIMEZONE "Europe/Moscow";     echo ">(GMT+03:00) Moscow, St. Petersburg, Volgograd</option>"
echo "</select>"


echo "<form action=\"timezone.cgi\" method=\"POST\" id=\"timeform\">"
echo "<button type=\"submit\" value=\"OK\">Set Time Zone</button>" 
echo "</form>"
echo "<center><a href=\"main_menu.cgi\"><input type=\"button\" value=\"Back\"></center></a>"
echo "</body>"
echo "</html>"
