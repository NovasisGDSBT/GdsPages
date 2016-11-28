#!/bin/sh
TEMP_IP=`ifconfig | grep Bcast | awk '{ print $2 }' | sed 's/addr://g'`
CURRENT_IP=`echo $TEMP_IP | awk '{print $1}'`

# PARSE QUERY STRING
if [ "$REQUEST_METHOD" == "POST" ];then
	read QUERY_STRING
	saveIFS=$IFS
	IFS='&'
	set -- $QUERY_STRING
	cd /tmp

	NEWMAC=`echo $1 | sed 's/%3A/:/g'`
	echo $NEWMAC > /tmp/mac_addr

	mkdir  -p /tmp/store_mountpoint
	while true; do 
	  mount /dev/mmcblk0p3 /tmp/store_mountpoint
	  if [ $? -eq 0 ]  # test mount OK
	  then
	    sync
	    break
	  else
	    sleep 2
	  fi
	done
	cp /tmp/mac_addr /tmp/store_mountpoint/sysconfig/.
	cp /tmp/mac_addr /etc/sysconfig/.
	umount /tmp/store_mountpoint
	 e2fsck /dev/mmcblk0p3

	echo "<html>"
	echo"  <head>"
	echo "    <title>Back to main</title>"
	echo "    <meta http-equiv=\"refresh\" content=\"1;URL=http://${CURRENT_IP}:8080/cgi-bin/main_menu.cgi\">"
	echo "  </head>"
	echo "  <body>"
	echo "    <p>"
	echo "    <center>Applying new MAC</center>"
	echo "    </p>"
	echo "  </body>"
	echo "</html>"
	exit 0
else
	echo "<meta http-equiv=\"Content-Type\" content=\"text/html; CHARSET=utf-8\">"
	echo "<html>"
	echo "<head>"
	echo " <center><h3>PRODUCTION TEST PAGE</h3></center>"
	echo "</head>"
	echo "<body>"
fi

. /etc/sysconfig/mac_addr

# HEADER    
echo " <form action=\"test_only.cgi\" method=\"POST\">"
echo "  <table style=\"width:100%\">"
echo "   <tr>"
echo "    <td>MAC address</td><td><input type=\"text\" name=\"MAC_FROM_CMDLINE\" value="${MAC_FROM_CMDLINE}"></td>"
echo "   </tr>"
echo "      <tr><td>&nbsp</td><td><button type=\"submit\" value=\"OK\">OK</button></td></tr>" 
echo "  </table>"
echo " </form>"
echo "</body>"                                                                                          
echo "</html>"
exit 0
