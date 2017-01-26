#!/bin/sh

TEMP_IP=`ifconfig | grep Bcast | awk '{ print $2 }' | sed 's/addr://g'`
CURRENT_IP=`echo $TEMP_IP | awk '{print $1}'`
NTP_SERVER=`cat /etc/sysconfig/ntp.conf | grep server`
NTP_SERVER1=`echo $NTP_SERVER | awk '{print $2}'`
NTP_SERVER2=`echo $NTP_SERVER | awk '{print $5}'`

echo "<meta http-equiv=\"Content-Type\" content=\"text/html; CHARSET=utf-8\">"

echo "<html>"
echo "<head>"
echo "      <center><h3>SETTINGS</h3></center>"
echo "</head>"
echo "<body>"
echo "      <br>"

# PARSE QUERY STRING
if [ "$REQUEST_METHOD" == "POST" ];then
	read QUERY_STRING
	saveIFS=$IFS
	IFS='&'
	set -- $QUERY_STRING
	cd /tmp

# NETWORK SETTINGS
	echo $2 | grep "DHCP" > /dev/null
	if [ "$?" == "0" ];then
		echo "# Wired ethernet" > /tmp/network
	        echo "ETH0_ENABLED=Y" >> /tmp/network
        	echo "NET_DEVICE=eth0" >> /tmp/network
        	echo "NET_SUFFIX=novasis.it" >> /tmp/network
	        echo "# Maybe N or Y" >> /tmp/network
	        echo "# NET_USE_DHCP=N" >> /tmp/network
	        echo "NET_USE_DHCP=Y" >> /tmp/network
		echo "#The following will be ignored if NET_USE_DHCP=N" >> /tmp/network
	        echo "${EXPORT} NET_IP_ADDRESS=192.168.10.51" >> /tmp/network

	else
	        echo "# Wired ethernet" > /tmp/network
	        echo "ETH0_ENABLED=Y" >> /tmp/network
	        echo "NET_DEVICE=eth0" >> /tmp/network
	        echo "NET_SUFFIX=novasis.it" >> /tmp/network
	        echo "# Maybe N or Y" >> /tmp/network
	        echo "# NET_USE_DHCP=N" >> /tmp/network
	        echo "NET_USE_DHCP=N" >> /tmp/network
		echo $1 | sed s/new_ip=//g > /tmp/new_ip
       		NEWIP=`cat /tmp/new_ip`
		echo "#The following will be ignored if NET_USE_DHCP=N" >> /tmp/network
	        echo "NET_IP_ADDRESS=$NEWIP" >> /tmp/network
	fi

	echo $3 | sed s/new_mask=//g > /tmp/new_mask
	NEWMASK=`cat /tmp/new_mask`

	echo $4 | sed s/new_hostname=//g > /tmp/new_hostname
	NEWHOSTNAME=`cat /tmp/new_hostname`

	echo $5 | sed s/new_gateway=//g > /tmp/new_gateway
	NEWGATEWAY=`cat /tmp/new_gateway`

	echo $6 | sed s/new_dns=//g > /tmp/new_dns
	NEWDNS=`cat /tmp/new_dns`

	echo $7 | sed s/new_reference_server=//g > /tmp/new_reference_server
	NEWREFERENCE_SERVER=`cat /tmp/new_reference_server`

	echo $8 | sed 's/new_chromium_var=//g' | sed 's/%3A/:/g' | sed 's/%2F/\//g' > /tmp/new_chromium_var
	NEWCHROMIUM_SERVER=`cat /tmp/new_chromium_var`

	echo "NET_BROADCAST=10.0.0.0" >> /tmp/network
	echo "NET_MASK=$NEWMASK" >> /tmp/network
	echo "NET_HOSTNAME=$NEWHOSTNAME" >> /tmp/network
	echo "NET_NETWORK=10.0.0.0" >> /tmp/network
	echo "NET_GATEWAY=$NEWGATEWAY" >> /tmp/network
	echo "NET_DNS_LIST=$NEWDNS" >> /tmp/network
	echo "NET_DNS_DOMAIN=-" >> /tmp/network


        NTP_SERVER1=`echo $9 | sed 's/NTP_SERVER1=//g'`
        NTP_SERVER2=`echo $10 | sed 's/NTP_SERVER2=//g'`

        echo "server $NTP_SERVER1 iburst" > /tmp/ntp.conf
        echo "server $NTP_SERVER2 iburst" >> /tmp/ntp.conf
        echo "restrict default kod nomodify notrap nopeer noquery" >> /tmp/ntp.conf
        echo "restrict -6 default kod nomodify notrap nopeer noquery" >> /tmp/ntp.conf
        echo "restrict 127.0.0.1" >> /tmp/ntp.conf
        echo "restrict -6 ::1" >> /tmp/ntp.conf
        echo "minpoll 3" >> /tmp/ntp.conf
        echo "maxpoll 3" >> /tmp/ntp.conf

	echo "REFERENCE_SERVER=$NEWREFERENCE_SERVER" > /tmp/system_vars

	echo "CHROMIUM_SERVER=$NEWCHROMIUM_SERVER" > /tmp/chromium_var

		
	mkdir -p /tmp/store_mountpoint
	while true; do 
	  mount /dev/mmcblk0p3 /tmp/store_mountpoint
	  if [ $? -eq 0 ]  # test mount OK
	  then 
	    sync
	    cp /tmp/network /tmp/store_mountpoint/sysconfig/etc/sysconfig/.
	    cp /tmp/ntp.conf /tmp/store_mountpoint/sysconfig/etc/sysconfig/.
	    cp /tmp/ntp.conf /tmp/store_mountpoint/etc/ntp.conf
	    cp /tmp/ntp.conf /etc/.
	    cp /tmp/system_vars /tmp/store_mountpoint/sysconfig/etc/sysconfig/.
	    cp /tmp/chromium_var /tmp/store_mountpoint/sysconfig/etc/sysconfig/.
	    cp /tmp/store_mountpoint/sysconfig/etc/sysconfig/network /etc/sysconfig/.
	    cp /tmp/store_mountpoint/sysconfig/etc/sysconfig/system_vars /etc/sysconfig/.
	    cp /tmp/store_mountpoint/sysconfig/etc/sysconfig/chromium_var /etc/sysconfig/.
	    cp /tmp/store_mountpoint/sysconfig/etc/sysconfig/ntp.conf /etc/sysconfig/.
	    cd /
	    break #
	  else
	    sleep 2       # wait 2 second to access resource
	  fi
	done	
	umount /tmp/store_mountpoint
	sync
	e2fsck /dev/mmcblk0p3

	echo "<html>"
	echo"  <head>"
	echo "    <title>Applying Network Settings</title>"
	echo "    <meta http-equiv=\"refresh\" content=\"1;URL=http://${CURRENT_IP}:8080/cgi-bin/main_menu.cgi\">"
	echo "  </head>"
	echo "  <body>"
	echo "    <p>"
	echo "    <center>Waiting for network settings apply</center><br>"
	echo "    </p>"
	echo "  </body>"
	echo "</html>"
	exit 0

fi

. /etc/sysconfig/network
. /etc/sysconfig/system_vars
. /etc/sysconfig/chromium_var

# HEADER    
#CURRENT_IP=`ifconfig | grep Bcast | awk '{ print $2 }' | sed 's/addr://g'`
echo "      <form action=\"settings.cgi\" method=\"POST\">"
echo "  <table style=\"width:100%\">"
echo "		     <tr>"
 
if [ "${NET_USE_DHCP}" == "Y" ];then
	echo "              <td>IP</td><td><input type=\"text\" name=\"new_ip\" value="${CURRENT_IP}">"
	echo "		    DHCP <input type=\"radio\" name=\"dhcp\" value=\"DHCP\" checked/>"
	echo "              STATIC IP <input type=\"radio\" name=\"dhcp\" value=\"STATIC\"/></td>"
else
	echo "              <td>IP</td><td><input type=\"text\" name=\"new_ip\" value="${NET_IP_ADDRESS}">"
	echo "		    DHCP <input type=\"radio\" name=\"dhcp\" value=\"DHCP\"/>"
	echo "              STATIC IP <input type=\"radio\" name=\"dhcp\" value=\"STATIC\" checked/></td>"
fi
echo "              </tr>"
echo "              <tr>"
echo "              <td>MASK</td><td><input type=\"text\" name=\"new_mask\" value="${NET_MASK}"></td>"
echo "              </tr>"
echo "              <tr>"
echo "              <td>HOSTNAME</td><td><input type=\"text\" name=\"new_hostname\" value="${NET_HOSTNAME}"></td>"
echo "              </tr>"
echo "              <tr>"
echo "              <td>GATEWAY</td><td><input type=\"text\" name=\"new_gateway\" value="${NET_GATEWAY}"></td>"
echo "              </tr>"
echo "              <tr>"
echo "              <td>DNS</td><td><input type=\"text\" name=\"new_dns\" value="${NET_DNS_LIST}"></td>"
echo "              </tr>"
echo "              <tr>"
echo "              <td>REFERENCE_SERVER</td><td><input type=\"text\" name=\"new_reference_server\" value="${REFERENCE_SERVER}"></td>"
echo "              </tr>"
echo "              <tr>"
echo "              <td>CHROMIUM_ADDRESS</td><td><input type=\"text\" name=\"new_chromium_var\" value="${CHROMIUM_SERVER}"></td>"
echo "              </tr>"
echo "              <tr>"
echo "		    <td>NTP Server 1</td><td><input type=\"text\" name=\"NTP_SERVER1\" value=\"${NTP_SERVER1}\"></td>"
echo "              </tr>"
echo "              <tr>"
echo "		    <td>NTP Server 2</td><td><input type=\"text\" name=\"NTP_SERVER2\" value=\"${NTP_SERVER2}\"></td>"
echo "              </tr>"

ifconfig eth0 | grep "RX packets" > /tmp/net_stats
ifconfig eth0 | grep "TX packets" >> /tmp/net_stats
ifconfig eth0 | grep "collisions" >> /tmp/net_stats
ifconfig eth0 | grep "RX bytes" >> /tmp/net_stats

echo "              <tr>"
echo "      	    <td>NETWORK STATS</td><td><pre><left>"
cat /tmp/net_stats
echo "              </left></pre></td></tr>"

echo "      <tr><td>&nbsp</td><td><button type=\"submit\" value=\"OK\">OK</button>&nbsp<a href=\"main_menu.cgi\"><input type=\"button\" value=\"Back\"></a></td></tr>" 
echo "      </table>"
echo "      </form>"
echo "<br>"
echo "</body>"                                                                                          
echo "</html>"
exit 0
