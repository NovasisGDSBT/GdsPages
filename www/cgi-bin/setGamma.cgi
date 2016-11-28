#!/bin/sh

# HEADER    
echo "<meta http-equiv=\"Content-Type\" content=\"text/html; CHARSET=utf-8\">"
echo ""
echo "<html>"
echo "<head>"
echo "<center><h3>"
echo "Gamma Value"
echo "[default = 1.386, min = 0.01, max = 1.99 ]"
echo "</h3></center>"
echo " </head>"
echo " <body>"

if [ "$REQUEST_METHOD" == "POST" ];then
	read GAMMAVALUE
	VALUE=`echo $GAMMAVALUE | sed s'/GAMMAVALUE=//g'`
	if [ "$VALUE" == "0" ]; then
		VALUE="0.01"
	fi
	if [ $VALUE -gt 1 ]; then
		VALUE=1.99
	fi

	TIME=$(date | cut -f4 -d' ')
	if [ -f /tmp/log_gamma ]; then
		echo $TIME >> /tmp/log_gamma
	else
		touch /tmp/log_gamma
		echo $TIME >> /tmp/log_gamma
	fi
	
	/tmp/www/GdsGamma $(echo $VALUE) >> /tmp/log_gamma
	
	mkdir -p /tmp/store_mountpoint
	
	while true; do 
	  mount /dev/mmcblk0p3 /tmp/store_mountpoint
	  if [ $? -eq 0 ]  # test mount OK
	  then
	    sync
	    cp /tmp/GammaDefaultValue /tmp/store_mountpoint/sysconfig/.
	    cp /tmp/store_mountpoint/sysconfig/GammaDefaultValue /etc/sysconfig/.
	    break #
	  else
	    sleep 2       # wait 2 second to access resource
	  fi
	done	
	umount /tmp/store_mountpoint
	e2fsck /tmp/store_mountpoint
else
	# read default value from file
	VALUE=$(cat /etc/sysconfig/GammaDefaultValue)
fi

echo "     <br>"
echo "     <br>"
echo "      <form action=\"setGamma.cgi\" method=\"POST\">"
echo "      <button type=\"submit\" value=\"button\">Store values</button>" 
echo "     <br>"
echo "     <br>"
echo "      <a href=\"main_menu.cgi\"><input type=\"button\" value=\"Back\"></a>"
echo "<B> Gamma Value: </B><input type=\"text\" name=\"GAMMAVALUE\" value=\"$VALUE\">"

echo "      </form>"
echo "</body>"
echo "</html>"


exit 0

