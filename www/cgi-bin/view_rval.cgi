#!/bin/sh

. /tmp/backlight_on
. /tmp/reboot_counter

LVDS_CLASS_POINTER=`cat /tmp/www/cgi-bin/lvds_device`

# Note : ioports must be initialize from c code befare they can be used
# There is no initialization code here
GPIO_PORT1_BASE=0*32
GPIO_PORT2_BASE=1*32
GPIO_PORT3_BASE=2*32
GPIO_PORT4_BASE=3*32
GPIO_PORT5_BASE=4*32
GPIO_PORT6_BASE=5*32
# Outputs
# gpio159=SW_FAULT
let SW_FAULT=$GPIO_PORT5_BASE+31
# gpio162=URL_COM
let URL_COM=$GPIO_PORT6_BASE+2
# gpio163=OVERTEMP
let OVERTEMP=$GPIO_PORT6_BASE+3
# gpio165=PANEL_LIGHT_FAIL
let PANEL_LIGHT_FAIL=$GPIO_PORT6_BASE+5
# gpio24=ETH_SWITCH
let ETH_SWITCH=$GPIO_PORT1_BASE+24
# Inputs
let CFG_BIT0=$GPIO_PORT5_BASE+20
let CFG_BIT1=$GPIO_PORT1_BASE+2
let CFG_BIT2=$GPIO_PORT3_BASE+28
let CFG_BIT3=$GPIO_PORT3_BASE+30
let BACKLIGHT_FAULT=$GPIO_PORT5_BASE+30

get_bit_level ()
{
# param $1 is the bit gpio number
	BIT_LEVEL=`cat /sys/class/gpio/gpio$1/value`
}

set_bit_level ()
{
# param $1 is the bit gpio number
# param $2 is the bit level at wich the gpio will be set
	echo $2 > /sys/class/gpio/gpio$1/value
}

get_address ()
{ 
        SYS_ADDRESS="0"
        get_bit_level $CFG_BIT0
        [ "${BIT_LEVEL}" == "0" ] &&  SYS_ADDRESS="1"
        get_bit_level $CFG_BIT1
        [ "${BIT_LEVEL}" == "0" ] &&  SYS_ADDRESS="2"
        get_bit_level $CFG_BIT2
        [ "${BIT_LEVEL}" == "0" ] &&  SYS_ADDRESS="3"
        get_bit_level $CFG_BIT3
        [ "${BIT_LEVEL}" == "0" ] &&  SYS_ADDRESS="4"
}

# VARIABLES
MAC=`cat /sys/class/net/eth0/address`
HOSTNAME=$NET_HOSTNAME
HTTPDPORT="8080"
if [ -f /tmp/autobacklight_enable ]; then
	AUTO_BACKLIGHT=ON
else
	AUTO_BACKLIGHT=OFF
fi
let CPU_OVERTEMP_FREQ=`cat /sys/class/thermal/thermal_zone0/trip_point_0_temp`/1000
let CPU_OVERTEMP_CRIT=`cat /sys/class/thermal/thermal_zone0/trip_point_1_temp`/1000
BACKLIGHT_LEVEL=`cat $LVDS_CLASS_POINTER/brightness`
LEDALARM1=OFF
LEDALARM2=OFF
LEDALARM3=OFF
LEDALARM4=OFF
LEDALARM5=OFF

# CODE

echo "<meta http-equiv=\"Content-Type\" content=\"text/html; CHARSET=utf-8\">"

echo "<html>"
echo "<head>"
#echo "      <center><h2>DIAGNOSTIC</h2></center>"
echo "</head>"
echo "<body>"

# PARSE QUERY STRING
if [ "$REQUEST_METHOD" == "POST" ];then
	DO_STORE="YES"
	read QUERY_STRING
	saveIFS=$IFS
	IFS='&'
	set -- $QUERY_STRING

	#echo "Number of param is $#<br>"
	#echo $1 | grep "variable1" > /dev/null
	#if [ "$?" == "0" ];then
	#	echo `cat /tmp/httpdport`
	#fi

	echo "$2" > /tmp/temperature_limits
	echo "$3" >> /tmp/temperature_limits
	# Backlight
	BACKG=$7
	BACKLIGHT_MAX_AT_MAXLIGHT=$10
	if ! [ -f /tmp/autobacklight_enable ]; then
		echo $7 | sed 's/BACKLIGHT_LEVEL=//g' > ${LVDS_CLASS_POINTER}/brightness
		. /tmp/backlight_limits
		B=`cat ${LVDS_CLASS_POINTER}/brightness`
		if [ $B -gt $BACKLIGHT_MAX_AT_MAXLIGHT ]; then
		        B=$BACKLIGHT_MAX_AT_MAXLIGHT
		fi
		echo $B > ${LVDS_CLASS_POINTER}/brightness
		BACKG=$B	
	fi
	BACKLIGHT_LEVEL=`cat ${LVDS_CLASS_POINTER}/brightness | sed 's/BACKLIGHT_LEVEL=//g'`
	#echo $8 | sed 's/AUTO_BACKLIGHT//g'
	if [ "$8" == "AUTO_BACKLIGHT=ON" ]; then
		echo "AUTO_BACKLIGHT=ON" > /tmp/autobacklight_enable
	else
		rm -f /tmp/autobacklight_enable
	fi
	if [ -f /tmp/autobacklight_enable ]; then
		AUTO_BACKLIGHT=ON
	else
		AUTO_BACKLIGHT=OFF
	fi
	AUTO_BACKLIGHT=`echo $8 | sed 's/AUTO_BACKLIGHT=//g' `
	#echo "$7" > /tmp/backlight_limits
	echo "BACKLIGHT_LEVEL=$BACKG" > /tmp/backlight_limits
	echo "$8" >> /tmp/backlight_limits
	echo "$9" >> /tmp/backlight_limits
	echo "$10" >> /tmp/backlight_limits
	echo "$11" >> /tmp/backlight_limits
	echo "$12" >> /tmp/backlight_limits
	echo "$15" >> /tmp/backlight_limits
	#echo "$16" >> /tmp/backlight_limits
	echo "BACKLIGHT_REF_DEFAULT=4" >> /tmp/backlight_limits
		
	xml ed --inplace -u "/data-set/lightParam/BACKLIGHT_MIN" -v $(echo $9 | sed 's/BACKLIGHT_MIN_AT_MINLIGHT=//g') /tmp/NovaConfig.xml
	xml ed --inplace -u "/data-set/lightParam/BACKLIGHT_MAX" -v $(echo ${10} | sed 's/BACKLIGHT_MAX_AT_MAXLIGHT=//g') /tmp/NovaConfig.xml
	xml ed --inplace -u "/data-set/lightParam/AMBIENTLIGHT_MIN" -v $(echo ${11} | sed 's/MIN_AMBIENT_LIGHT=//g') /tmp/NovaConfig.xml
	xml ed --inplace -u "/data-set/lightParam/AMBIENTLIGHT_MAX" -v $(echo ${12} | sed 's/MAX_AMBIENT_LIGHT=//g') /tmp/NovaConfig.xml
	xml ed --inplace -u "/data-set/lightParam/BACKLIGHT_FAULTY_AMBSENS" -v 1 /tmp/NovaConfig.xml

	# Address
	get_address
	echo "$SYS_ADDRESS" > /tmp/system_address
	# Ethernet
	echo "ENABLE_ETH_SWITCH=SIMULATED" > /tmp/net_variables
	echo "MAC=`cat /sys/class/net/eth0/address`" >> /tmp/net_variables
	. /etc/sysconfig/network
	if [ "$NET_USE_DHCP" == "NET_USE_DHCP=Y" ]; then 
		echo "NET_USE_DHCP=DHCP" >> /tmp/net_variables
	 else 
		echo "NET_USE_DHCP=STATIC" >> /tmp/net_variables  
	fi
	echo "NET_IP_ADDRESS=`ifconfig | grep Bcast | awk '{print $2}' | sed 's/addr://g'`" >> /tmp/net_variables
	echo "NET_MASK=$NET_MASK" >> /tmp/net_variables
	echo "NET_GATEWAY=$NET_GATEWAY" >> /tmp/net_variables
	echo "HOSTNAME=$NET_HOSTNAME" >> /tmp/net_variables
	echo "HTTPDPORT=8080" >> /tmp/net_variables
	# Alarms
	echo "$26" >  /tmp/gpio_settings
	echo "$27" >> /tmp/gpio_settings
	echo "$28" >> /tmp/gpio_settings
	echo "$29" >> /tmp/gpio_settings
	echo "$30" >> /tmp/gpio_settings
	if [ "$26" == "LEDALARM1=ON" ]; then
		set_bit_level $SW_FAULT 0
	else
		set_bit_level $SW_FAULT 1
	fi
	if [ "$27" == "LEDALARM2=ON" ]; then
		set_bit_level $URL_COM 0
	else
		set_bit_level $URL_COM 1
	fi
	if [ "$28" == "LEDALARM3=ON" ]; then
		set_bit_level $OVERTEMP 0
	else
		set_bit_level $OVERTEMP 1
	fi
	if [ "$29" == "LEDALARM4=ON" ]; then
		set_bit_level $PANEL_LIGHT_FAIL 0
	else
		set_bit_level $PANEL_LIGHT_FAIL 1
	fi
	if [ "$30" == "LEDALARM5=ON" ]; then
		set_bit_level $PANEL_LIGHT_FAIL 0
	else
		set_bit_level $PANEL_LIGHT_FAIL 1
	fi
	echo "$31" >> /tmp/alarms
	echo "$32" >> /tmp/alarms
	echo "$33" >> /tmp/alarms
	echo "$34" >> /tmp/alarms
	echo "$35" >> /tmp/alarms
	echo "$36" >> /tmp/alarms
	echo "$37" >> /tmp/alarms
	
	# RTC
	# $38  and $39
	echo $39 | grep TIME_READ=SET > /dev/null 2>&1
	if [ $? -eq 0 ]; then

	  DATE=`echo $38 | sed 's/TIME_SET=//g' | sed 's/%3A/:/g'`
	  date -s $DATE > /dev/null 2>&1
	  /tmp/www/logwrite.sh "DREC" "INFO" "WEB" "Saved new Date time:$DATE" &
	else
	  /tmp/www/logwrite.sh "DREC" "INFO" "WEB" "Saved new parameters" &
	fi
	


	#Voltages
	#$40 to $45
	echo "$42" > /tmp/voltages_readout
	echo "$43" >> /tmp/voltages_readout
	echo "$44" >> /tmp/voltages_readout
	echo "$45" >> /tmp/voltages_readout
	#Panel
	echo "$46" > /tmp/rgb_settings
	echo "$47" >> /tmp/rgb_settings
	echo "$48" >> /tmp/rgb_settings
	
	. /tmp/rgb_settings
	echo "$RED_GAIN,0,0" > /tmp/rgb_matrix
	echo "0,$GREEN_GAIN,0" >> /tmp/rgb_matrix
	echo "0,0,$BLUE_GAIN" >> /tmp/rgb_matrix
	echo "0,0,0" >> /tmp/rgb_matrix
	echo "2,2,2" >> /tmp/rgb_matrix
	/tmp/www/NovaCSC f /tmp/rgb_matrix > /dev/null 2>&1
	xml ed --inplace -u "/data-set/ColorTemp/RED_GAIN" -v $RED_GAIN /tmp/NovaConfig.xml
	xml ed --inplace -u "/data-set/ColorTemp/GREEN_GAIN" -v $GREEN_GAIN /tmp/NovaConfig.xml
	xml ed --inplace -u "/data-set/ColorTemp/BLUE_GAIN" -v $BLUE_GAIN /tmp/NovaConfig.xml

	#Versions from $49 to $53
	if [ "$54" == "BLON_TIME_RESET=1" ]; then
		echo "RESET" >  /tmp/backlight_on_reset
	fi
# > /tmp/event_list
	#$55 is monitor_on
	#EVENTLIST
	echo "$57" > /tmp/event_list
	#SETUP BOOT
	
	MAX_TIME_SQUARE=30
	MAX_WAIT_TIME_FOR_COMMUNICATIONS=120
	MAX_RETRY_TIME_COMMUNICATIONS=15
	YELLOWTIME=$(echo $58 | grep YELLOW_SQUARE_TIME | sed 's/YELLOW_SQUARE_TIME=//g')  
	GREENTIME=$(echo $59 | grep TCMS_GREEN_SQUARE_TIME | sed 's/TCMS_GREEN_SQUARE_TIME=//g')
	WAIT_TIME_FOR_COMMUNICATIONS=$(echo $60 | grep WAIT_TIME_FOR_COMMUNICATIONS | sed 's/WAIT_TIME_FOR_COMMUNICATIONS=//g')
	RETRY_TIME_COMMUNICATIONS=$(echo $61 | grep RETRY_TIME_COMMUNICATIONS | sed 's/RETRY_TIME_COMMUNICATIONS=//g')
	REDTIME=$(echo $62 | grep TIME_END_SQUARE | sed 's/TIME_END_SQUARE=//g')
	MIN_TIME_COMMUNICATIONS=3


	if [ "$YELLOWTIME" -gt "$MAX_TIME_SQUARE" ]; then                                                      
		echo "YELLOW_SQUARE_TIME=$MAX_TIME_SQUARE" > /tmp/setup_boot
	elif [ "$YELLOWTIME" -lt "$MIN_TIME_COMMUNICATIONS" ]; then
		echo "YELLOW_SQUARE_TIME=$MIN_TIME_COMMUNICATIONS" > /tmp/setup_boot
	else
		echo "$58" > /tmp/setup_boot                            
	fi

                                                                  
	if [ "$GREENTIME" -gt "$MAX_TIME_SQUARE" ]; then
		echo "TCMS_GREEN_SQUARE_TIME=$MAX_TIME_SQUARE" >> /tmp/setup_boot
	elif [ "$GREENTIME" -lt "$MIN_TIME_COMMUNICATIONS" ]; then
		echo "TCMS_GREEN_SQUARE_TIME=$MIN_TIME_COMMUNICATIONS" >> /tmp/setup_boot
	else
		echo "$59" >> /tmp/setup_boot
	fi


#	if [ "$WAIT_TIME_FOR_COMMUNICATIONS" -le "$MAX_WAIT_TIME_FOR_COMMUNICATIONS" ]; then
		echo "$60" >> /tmp/setup_boot
#	if [ "$WAIT_TIME_FOR_COMMUNICATIONS" -ge "$MIN_TIME_COMMUNICATIONS" ]; then
#		echo "$60" >> /tmp/setup_boot
#	else
#		echo "WAIT_TIME_FOR_COMMUNICATIONS=$MAX_WAIT_TIME_FOR_COMMUNICATIONS" >> /tmp/setup_boot
#	fi


#	if [ "$RETRY_TIME_COMMUNICATIONS" -le "$MAX_RETRY_TIME_COMMUNICATIONS" ]; then
		echo "$61" >> /tmp/setup_boot
#	else
#		echo "RETRY_TIME_COMMUNICATIONS=$MAX_RETRY_TIME_COMMUNICATIONS" >> /tmp/setup_boot
#	fi

	if [ "$REDTIME" -gt "$MAX_TIME_SQUARE" ]; then
		echo "TIME_END_SQUARE=$MAX_TIME_SQUARE" >> /tmp/setup_boot
	elif [ "$REDTIME" -lt "$MIN_TIME_COMMUNICATIONS" ]; then
		echo "TIME_END_SQUARE=$MIN_TIME_COMMUNICATIONS" >> /tmp/setup_boot
	else
		echo "$62" >> /tmp/setup_boot
	fi
        
	mkdir -p /tmp/store_mountpoint
	while true; do 
		mount /dev/mmcblk0p3 /tmp/store_mountpoint
		if [ $? -eq 0 ]; then  # test mount OK
			sync
			break
		else
			sleep 2       # wait 2 second to access resource
		fi
	done

	if [ -f /tmp/autobacklight_enable ]; then
		cp /tmp/autobacklight_enable /tmp/store_mountpoint/webparams
	else
		rm -f /tmp/store_mountpoint/webparams/autobacklight_enable
	fi
	cp /tmp/backlight_limits /tmp/store_mountpoint/webparams/tmpfile1
	cp /tmp/temperature_limits /tmp/store_mountpoint/webparams/tmpfile2
	cp /tmp/rgb_settings /tmp/store_mountpoint/webparams/tmpfile3
	cp /tmp/gpio_settings /tmp/store_mountpoint/webparams/tmpfile4
	cp /tmp/net_variables /tmp/store_mountpoint/webparams/tmpfile5
	cp /tmp/event_list /tmp/store_mountpoint/webparams/tmpfile6
	cp /tmp/setup_boot /tmp/store_mountpoint/webparams/tmpfile7
	cp /tmp/voltages_readout /tmp/store_mountpoint/webparams/tmpfile8
	cp /tmp/rgb_matrix /tmp/store_mountpoint/webparams/tmpfile9
	cp /tmp/NovaConfig.xml /tmp/store_mountpoint/webparams/tmpfile10
	
	mv /tmp/store_mountpoint/webparams/tmpfile1 /tmp/store_mountpoint/webparams/backlight_limits
	mv /tmp/store_mountpoint/webparams/tmpfile2 /tmp/store_mountpoint/webparams/temperature_limits
	mv /tmp/store_mountpoint/webparams/tmpfile3 /tmp/store_mountpoint/webparams/rgb_settings
	mv /tmp/store_mountpoint/webparams/tmpfile4 /tmp/store_mountpoint/webparams/gpio_settings
	mv /tmp/store_mountpoint/webparams/tmpfile5 /tmp/store_mountpoint/webparams/net_variables
	mv /tmp/store_mountpoint/webparams/tmpfile6 /tmp/store_mountpoint/webparams/event_list
	mv /tmp/store_mountpoint/webparams/tmpfile7 /tmp/store_mountpoint/webparams/setup_boot
	mv /tmp/store_mountpoint/webparams/tmpfile8 /tmp/store_mountpoint/webparams/voltages_readout
	mv /tmp/store_mountpoint/webparams/tmpfile9 /tmp/store_mountpoint/webparams/rgb_matrix
	mv /tmp/store_mountpoint/webparams/tmpfile10 /tmp/store_mountpoint/webparams/NovaConfig.xml
	
	. /tmp/hw_version
	. /tmp/sw_version

	umount /tmp/store_mountpoint
        e2fsck /dev/mmcblk0p3
fi

. /tmp/processor_temp
. /tmp/carrier_temp
. /tmp/voltages_readout
. /tmp/voltage24_value
. /tmp/voltage12_value
. /tmp/alarms
. /tmp/backlight_sensor_value
. /tmp/ambientlight_value
. /tmp/event_list
. /tmp/setup_boot
. /etc/sysconfig/network
. /etc/sysconfig/system_vars

if [ -f /tmp/autobacklight_enable ]; then
	. /tmp/autobacklight_enable
fi
if [ -f /tmp/backlight_limits ]; then
	. /tmp/backlight_limits
fi
if [ -f /tmp/temperature_limits ]; then
	. /tmp/temperature_limits
fi
if [ -f /tmp/rgb_settings ]; then
	. /tmp/rgb_settings
fi
if [ -f /tmp/gpio_settings ]; then
	. /tmp/gpio_settings
fi
if [ -f /tmp/net_variables ]; then
	. /tmp/net_variables
fi
if [ -f /tmp/hw_version ]; then
	. /tmp/hw_version
fi
if [ -f /tmp/sw_version ]; then
	. /tmp/sw_version
fi
if [ -f /tmp/system_address ]; then
	. /tmp/system_address
fi
if [ -f /tmp/alarms ]; then
	. /tmp/alarms
fi
BACKLIGHT_LEVEL=`cat ${LVDS_CLASS_POINTER}/brightness | sed 's/BACKLIGHT_LEVEL=//g'`

        BACKG=$7
        if ! [ -f /tmp/autobacklight_enable ]; then
                echo $7 | sed 's/BACKLIGHT_LEVEL=//g' > ${LVDS_CLASS_POINTER}/brightness
                . /tmp/backlight_limits
                B=`cat ${LVDS_CLASS_POINTER}/brightness`
                if [ $B -gt $BACKLIGHT_MAX_AT_MAXLIGHT ]; then
                        B=$BACKLIGHT_MAX_AT_MAXLIGHT
                fi
                echo $B > ${LVDS_CLASS_POINTER}/brightness
                BACKG=$B
        fi

TABLE_WIDTH=800
ROW_TITLE_WIDTH="50%"
ROW_VALUE_WIDTH="50%"
# Page access, no methods
echo "      <form action=\"view_rval.cgi\" method=\"POST\">"
echo "      <br>"
echo "      <button type=\"submit\" value=\"button\">Store values</button>" 
echo "      <a href=\"main_menu.cgi\"><input type=\"button\" value=\"Back\"></a>"
echo "       <table style=\"width:${TABLE_WIDTH}\">"
echo "          <td width=\"${TABLE_WIDTH}\"><center><h2>DIAGNOSTIC</h3></center></td>"
echo "      </table>"
echo "       <table style=\"width:${TABLE_WIDTH}\">"
echo "          <td width=\"${TABLE_WIDTH}\"><center><h3>TEMPERATURE</h3></center></td>"
echo "      </table>"
echo "       <table border=\"1\" style=\"width:${TABLE_WIDTH}\">"
echo "        <tr>"
echo "          <td width=\"${ROW_TITLE_WIDTH}\">Internal temperature</td>"
echo "          <td width=\"${ROW_VALUE_WIDTH}\"><input type=\"text\" name=\"INTERNAL_TEMPERATURE\" value=\"$INTERNAL_TEMPERATURE\"></td>"
echo "        </tr>"
echo "        <tr>"
echo "          <td width=\"${ROW_TITLE_WIDTH}\">Internal over temperature</td>"
echo "          <td width=\"${ROW_VALUE_WIDTH}\"><input type=\"text\" name=\"INTERNAL_OVERTEMPERATURE\" value=\"$INTERNAL_OVERTEMPERATURE\"></td>"
echo "        </tr>"
echo "        <tr>"
echo "          <td width=\"${ROW_TITLE_WIDTH}\">Internal under temperature</td>"
echo "          <td width=\"${ROW_VALUE_WIDTH}\"><input type=\"text\" name=\"INTERNAL_UNDERTEMPERATURE\" value=\"$INTERNAL_UNDERTEMPERATURE\"></td>"
echo "        </tr>"
echo "        <tr>"
echo "          <td width=\"${ROW_TITLE_WIDTH}\">CPU temperature</td>"
echo "          <td width=\"${ROW_VALUE_WIDTH}\"><input type=\"text\" name=\"CPU_TEMPERATURE\" value=\"$CPU_TEMPERATURE\"></td>"
echo "        </tr>"
echo "        <tr>"
echo "          <td width=\"${ROW_TITLE_WIDTH}\">CPU overtemperature frequency</td>"
echo "          <td width=\"${ROW_VALUE_WIDTH}\"><input type=\"text\" name=\"CPU_OVERTEMP_FREQ\" value=\"$CPU_OVERTEMP_FREQ\"></td>"
echo "        </tr>"
echo "        <tr>"
echo "          <td width=\"${ROW_TITLE_WIDTH}\">CPU overtemperature critical</td>"
echo "          <td width=\"${ROW_VALUE_WIDTH}\"><input type=\"text\" name=\"CPU_OVERTEMP_CRIT\" value=\"$CPU_OVERTEMP_CRIT\"></td>"
echo "        </tr>"
echo "      </table>"
echo "       <table style=\"width:${TABLE_WIDTH}\">"
echo "          <td width=\"${TABLE_WIDTH}\"><center><h3>BACKLIGHT</h3></center></td>"
echo "      </table>"
echo "       <table border=\"1\" style=\"width:${TABLE_WIDTH}\">"
echo "        <tr>"
BKG_LEVEL=`echo $BACKLIGHT_LEVEL | sed 's/BACKLIGHT_LEVEL=//g'`
echo "          <td width=\"${ROW_TITLE_WIDTH}\">Backlight Level</td>"
echo "          <td width=\"${ROW_VALUE_WIDTH}\"><input type=\"text\" name=\"BACKLIGHT_LEVEL\" value=\"$BKG_LEVEL\"></td>"
echo "        </tr>"
echo "        <tr>"
echo "          <td width=\"${ROW_TITLE_WIDTH}\">Auto Backlight</td>"
echo "          <td width=\"${ROW_VALUE_WIDTH}\"><input type=\"text\" name=\"AUTO_BACKLIGHT\" value=\"$AUTO_BACKLIGHT\"></td>"
echo "        </tr>"
echo "        <tr>"
echo "          <td width=\"${ROW_TITLE_WIDTH}\">Backlight min at min light</td>"
echo "          <td width=\"${ROW_VALUE_WIDTH}\"><input type=\"text\" name=\"BACKLIGHT_MIN_AT_MINLIGHT\" value=\"$BACKLIGHT_MIN_AT_MINLIGHT\"></td>"
echo "        </tr>"
echo "        <tr>"
echo "          <td width=\"${ROW_TITLE_WIDTH}\">Backlight max at max light</td>"
echo "          <td width=\"${ROW_VALUE_WIDTH}\"><input type=\"text\" name=\"BACKLIGHT_MAX_AT_MAXLIGHT\" value=\"$BACKLIGHT_MAX_AT_MAXLIGHT\"></td>"
echo "        </tr>"
echo "        <tr>"
echo "          <td width=\"${ROW_TITLE_WIDTH}\">Min ambient light</td>"
echo "          <td width=\"${ROW_VALUE_WIDTH}\"><input type=\"text\" name=\"MIN_AMBIENT_LIGHT\" value=\"$MIN_AMBIENT_LIGHT\"></td>"
echo "        </tr>"
echo "        <tr>"
echo "          <td width=\"${ROW_TITLE_WIDTH}\">Max ambient light</td>"
echo "          <td width=\"${ROW_VALUE_WIDTH}\"><input type=\"text\" name=\"MAX_AMBIENT_LIGHT\" value=\"$MAX_AMBIENT_LIGHT\"></td>"
echo "        </tr>"
echo "        <tr>"
echo "          <td width=\"${ROW_TITLE_WIDTH}\">Ambient light</td>"
echo "          <td width=\"${ROW_VALUE_WIDTH}\"><input type=\"text\" name=\"AMBIENT_LIGHT\" value=\"$AMBIENT_LIGHT\"></td>"
echo "        </tr>"
echo "        <tr>"
echo "          <td width=\"${ROW_TITLE_WIDTH}\">Backlight sensor</td>"
echo "          <td width=\"${ROW_VALUE_WIDTH}\"><input type=\"text\" name=\"BACKLIGHT_SENSOR\" value=\"$BACKLIGHT_SENSOR\"></td>"
echo "        </tr>"
echo "        <tr>"
echo "          <td width=\"${ROW_TITLE_WIDTH}\">Backlight alarm threshold</td>"
echo "          <td width=\"${ROW_VALUE_WIDTH}\"><input type=\"text\" name=\"BACKLIGHT_ALARM_THRESHOLD\" value=\"$BACKLIGHT_ALARM_THRESHOLD\"></td>"
echo "        </tr>"
echo "        <tr>"
echo "          <td width=\"${ROW_TITLE_WIDTH}\">Backlight ref default</td>"
echo "          <td width=\"${ROW_VALUE_WIDTH}\"><input type=\"text\" name=\"BACKLIGHT_REF_DEFAULT\" value=\"$BACKLIGHT_REF_DEFAULT\"></td>"
echo "        </tr>"
echo "      </table>"
echo "       <table style=\"width:${TABLE_WIDTH}\">"
echo "          <td width=\"${TABLE_WIDTH}\"><center><h3>ADDRESS</h3></center></td>"
echo "      </table>"
echo "       <table border=\"1\" style=\"width:${TABLE_WIDTH}\">"
echo "        <tr>"
echo "          <td width=\"${ROW_TITLE_WIDTH}\">Address number</td>"
get_address
echo "          <td width=\"${ROW_VALUE_WIDTH}\"><input type=\"text\" name=\"ADDRESS\" value=\"$SYS_ADDRESS\"></td>"
echo "        </tr>"
echo "      </table>"
echo "       <table style=\"width:${TABLE_WIDTH}\">"
echo "          <td width=\"${TABLE_WIDTH}\"><center><h3>ETHERNET</h3></center></td>"
echo "      </table>"
echo "       <table border=\"1\" style=\"width:${TABLE_WIDTH}\">"
echo "        <tr>"
echo "          <td width=\"${ROW_TITLE_WIDTH}\">Enable eth switch</td>"
echo "          <td width=\"${ROW_VALUE_WIDTH}\"><input type=\"text\" name=\"ENABLE_ETH_SWITCH\" value=\"UNMODIFIABLE\"></td>"
echo "        </tr>"
echo "        <tr>"
echo "          <td width=\"${ROW_TITLE_WIDTH}\">MAC address</td>"
MAC=`cat /sys/class/net/eth0/address`
echo "          <td width=\"${ROW_VALUE_WIDTH}\"><input type=\"text\" name=\"MAC\" value=\"$MAC\"></td>"
echo "        </tr>"
echo "        <tr>"
echo "          <td width=\"${ROW_TITLE_WIDTH}\">IP address mode</td>"
. /etc/sysconfig/network
		if [ "$NET_USE_DHCP" = "Y" ]; then
			NET_USE_DHCP_VALUE="DHCP"
		else
			NET_USE_DHCP_VALUE="STATIC"
		fi
echo "          <td width=\"${ROW_VALUE_WIDTH}\"><input type=\"text\" name=\"W_NET_USE_DHCP\" value=\"$NET_USE_DHCP_VALUE\"></td>"
echo "        </tr>"
echo "        <tr>"
echo "          <td width=\"${ROW_TITLE_WIDTH}\">IP address</td>"
		NET_IP_ADDRESS=`ifconfig | grep Bcast | awk '{print $2}' | sed 's/addr:/ /g'`
echo "          <td width=\"${ROW_VALUE_WIDTH}\"><input type=\"text\" name=\"NET_IP_ADDRESS\" value=\"$NET_IP_ADDRESS\"></td>"
echo "        </tr>"
echo "        <tr>"
echo "          <td width=\"${ROW_TITLE_WIDTH}\">Subnet mask</td>"
echo "          <td width=\"${ROW_VALUE_WIDTH}\"><input type=\"text\" name=\"NET_MASK\" value=\"$NET_MASK\"></td>"
echo "        </tr>"
echo "        <tr>"
echo "          <td width=\"${ROW_TITLE_WIDTH}\">Gateway</td>"
echo "          <td width=\"${ROW_VALUE_WIDTH}\"><input type=\"text\" name=\"NET_GATEWAY\" value=\"$NET_GATEWAY\"></td>"
echo "        </tr>"
echo "        <tr>"
echo "          <td width=\"${ROW_TITLE_WIDTH}\">Host name</td>"
echo "          <td width=\"${ROW_VALUE_WIDTH}\"><input type=\"text\" name=\"HOSTNAME\" value=\"$HOSTNAME\"></td>"
echo "        </tr>"
echo "        <tr>"
echo "          <td width=\"${ROW_TITLE_WIDTH}\">Web service port</td>"
echo "          <td width=\"${ROW_VALUE_WIDTH}\"><input type=\"text\" name=\"HTTPDPORT\" value=\"$HTTPDPORT\"></td>"
echo "        </tr>"
echo "      </table>"
echo "       <table style=\"width:${TABLE_WIDTH}\">"
echo "          <td width=\"${TABLE_WIDTH}\"><center><h3>DIAGNOSTIC LEDs</h3></center></td>"
echo "      </table>"
echo "       <table border=\"1\" style=\"width:${TABLE_WIDTH}\">"
echo "        <tr>"
echo "          <td width=\"${ROW_TITLE_WIDTH}\">SW Fault</td>" #( Led/Alarm 1 )
echo "          <td width=\"${ROW_VALUE_WIDTH}\"><input type=\"text\" name=\"LEDALARM1\" value=\"$LEDALARM1\"></td>"
echo "	      </td>"
echo "        </tr>"
echo "        <tr>"
echo "          <td width=\"${ROW_TITLE_WIDTH}\">URL/InfoT Comms</td>" #( Led/Alarm 2 )
echo "          <td width=\"${ROW_VALUE_WIDTH}\"><input type=\"text\" name=\"LEDALARM2\" value=\"$LEDALARM2\"></td>"
echo "        </tr>"
echo "        <tr>"
echo "          <td width=\"${ROW_TITLE_WIDTH}\">Over Temp</td>" #( Led/Alarm 3 )
echo "          <td width=\"${ROW_VALUE_WIDTH}\"><input type=\"text\" name=\"LEDALARM3\" value=\"$LEDALARM3\"></td>"
echo "        </tr>"
echo "        <tr>"
echo "          <td width=\"${ROW_TITLE_WIDTH}\">LED Not Used</td>" #( Led/Alarm 4 )
echo "          <td width=\"${ROW_VALUE_WIDTH}\"><input type=\"text\" name=\"LEDALARM4\" value=\"$LEDALARM4\"></td>"
echo "        </tr>"
echo "        <tr>"
echo "          <td width=\"${ROW_TITLE_WIDTH}\">Backlight Failure</td>" #( Led/Alarm 5 )
echo "          <td width=\"${ROW_VALUE_WIDTH}\"><input type=\"text\" name=\"LEDALARM5\" value=\"$LEDALARM5\"></td>"
echo "        </tr>"
echo "        <tr>"
echo "          <td width=\"${ROW_TITLE_WIDTH}\">Over volt 24V alarm</td>"
echo "          <td width=\"${ROW_VALUE_WIDTH}\"><input type=\"text\" name=\"D_OVERVOLTAGE24\" value=\"$D_OVERVOLTAGE24\"></td>"
echo "        </tr>"
echo "        <tr>"
echo "          <td width=\"${ROW_TITLE_WIDTH}\">Under volt 24V alarm</td>"
echo "          <td width=\"${ROW_VALUE_WIDTH}\"><input type=\"text\" name=\"D_UNDERVOLTAGE24\" value=\"$D_UNDERVOLTAGE24\"></td>"
echo "        </tr>"
echo "        <tr>"
echo "          <td width=\"${ROW_TITLE_WIDTH}\">Over volt 12V alarm</td>"
echo "          <td width=\"${ROW_VALUE_WIDTH}\"><input type=\"text\" name=\"D_OVERVOLTAGE12\" value=\"$D_OVERVOLTAGE12\"></td>"
echo "        </tr>"
echo "        <tr>"
echo "          <td width=\"${ROW_TITLE_WIDTH}\">Under volt 12V alarm</td>"
echo "          <td width=\"${ROW_VALUE_WIDTH}\"><input type=\"text\" name=\"D_UNDERVOLTAGE12\" value=\"$D_UNDERVOLTAGE12\"></td>"
echo "        </tr>"
echo "        <tr>"
echo "          <td width=\"${ROW_TITLE_WIDTH}\">BL lum alarm</td>"
echo "          <td width=\"${ROW_VALUE_WIDTH}\"><input type=\"text\" name=\"BL_LUM_ALARM\" value=\"$BL_LUM_ALARM\"></td>"
echo "        </tr>"
echo "        <tr>"
echo "          <td width=\"${ROW_TITLE_WIDTH}\">Self test alarm</td>"
echo "          <td width=\"${ROW_VALUE_WIDTH}\"><input type=\"text\" name=\"SELF_TEST_ALARM\" value=\"$SELF_TEST_ALARM\"></td>"
echo "        </tr>"
echo "        <tr>"
echo "          <td width=\"${ROW_TITLE_WIDTH}\">Address alarm</td>"
echo "          <td width=\"${ROW_VALUE_WIDTH}\"><input type=\"text\" name=\"ADDRESS_ALARM\" value=\"$ADDRESS_ALARM\"></td>"
echo "        </tr>"
echo "      </table>"
echo "       <table style=\"width:${TABLE_WIDTH}\">"
echo "          <td width=\"${TABLE_WIDTH}\"><center><h3>RTC</h3></center></td>"
echo "      </table>"
echo "       <table border=\"1\" style=\"width:${TABLE_WIDTH}\">"
echo "        <tr>"
echo "          <td width=\"${ROW_TITLE_WIDTH}\">Time set</td>"
echo "          <td width=\"${ROW_VALUE_WIDTH}\"><input type=\"text\" name=\"TIME_SET\" value=\"`date -u +%f`\"></td>"
echo "        </tr>"
echo "        <tr>"
echo "          <td width=\"${ROW_TITLE_WIDTH}\">Enable Command DATE (SET/UNSET)</td>"
echo "          <td width=\"${ROW_VALUE_WIDTH}\"><input type=\"text\" name=\"TIME_READ\" value=\"UNSET\"></td>"
echo "        </tr>"
echo "      </table>"
echo "       <table style=\"width:${TABLE_WIDTH}\">"
echo "          <td width=\"${TABLE_WIDTH}\"><center><h3>VOLTAGES</h3></center></td>"
echo "      </table>"

let V24V=$VOLTAGE24/1000
V24MV=`echo $VOLTAGE24 | cut -c3-5`
let V12V=$VOLTAGE12/1000
V12MV=`echo $VOLTAGE12 | cut -c3-5`

echo "       <table border=\"1\" style=\"width:${TABLE_WIDTH}\">"
echo "        <tr>"
echo "          <td width=\"${ROW_TITLE_WIDTH}\">Voltage 24V readout</td>"
echo "          <td width=\"${ROW_VALUE_WIDTH}\"><input type=\"text\" name=\"VOLTAGE24\" value=\"$V24V.$V24MV\"></td>"
echo "        </tr>"
echo "        <tr>"
echo "          <td width=\"${ROW_TITLE_WIDTH}\">Voltage 12V readout</td>"
echo "          <td width=\"${ROW_VALUE_WIDTH}\"><input type=\"text\" name=\"VOLTAGE12\" value=\"$V12V.$V12MV\"></td>"
echo "        </tr>"
echo "        <tr>"
echo "          <td width=\"${ROW_TITLE_WIDTH}\">Over volt 24V threshold</td>"
[ "$OVERVOLTAGE24" == "" ] && OVERVOLTAGE24="26.4"
[ "$OVERVOLTAGE12" == "" ] && OVERVOLTAGE12="13.2"
[ "$UNDERVOLTAGE24" == "" ] && UNDERVOLTAGE24="21.6"
[ "$UNDERVOLTAGE12" == "" ] && UNDERVOLTAGE12="10.8"
echo "          <td width=\"${ROW_VALUE_WIDTH}\"><input type=\"text\" name=\"OVERVOLTAGE24\" value=\"$OVERVOLTAGE24\"></td>"
echo "        </tr>"
echo "        <tr>"
echo "          <td width=\"${ROW_TITLE_WIDTH}\">Under volt 24V threshold</td>"
echo "          <td width=\"${ROW_VALUE_WIDTH}\"><input type=\"text\" name=\"UNDERVOLTAGE24\" value=\"$UNDERVOLTAGE24\"></td>"
echo "        </tr>"
echo "        <tr>"
echo "          <td width=\"${ROW_TITLE_WIDTH}\">Over volt 12V threshold</td>"
echo "          <td width=\"${ROW_VALUE_WIDTH}\"><input type=\"text\" name=\"OVERVOLTAGE12\" value=\"$OVERVOLTAGE12\"></td>"
echo "        </tr>"
echo "        <tr>"
echo "          <td width=\"${ROW_TITLE_WIDTH}\">Under volt 12V threshold</td>"
echo "          <td width=\"${ROW_VALUE_WIDTH}\"><input type=\"text\" name=\"UNDERVOLTAGE12\" value=\"$UNDERVOLTAGE12\"></td>"
echo "        </tr>"
echo "      </table>"

echo "       <table style=\"width:${TABLE_WIDTH}\">"
echo "          <td width=\"${TABLE_WIDTH}\"><center><h3>PANEL</h3></center></td>"
echo "      </table>"
echo "       <table border=\"1\" style=\"width:${TABLE_WIDTH}\">"
echo "        <tr>"
echo "          <td width=\"${ROW_TITLE_WIDTH}\">Red gain</td>"
echo "          <td width=\"${ROW_VALUE_WIDTH}\"><input type=\"text\" name=\"RED_GAIN\" value=\"$RED_GAIN\"></td>"
echo "        </tr>"
echo "        <tr>"
echo "          <td width=\"${ROW_TITLE_WIDTH}\">Green gain</td>"
echo "          <td width=\"${ROW_VALUE_WIDTH}\"><input type=\"text\" name=\"GREEN_GAIN\" value=\"$GREEN_GAIN\"></td>"
echo "        </tr>"
echo "        <tr>"
echo "          <td width=\"${ROW_TITLE_WIDTH}\">Blue gain</td>"
echo "          <td width=\"${ROW_VALUE_WIDTH}\"><input type=\"text\" name=\"BLUE_GAIN\" value=\"$BLUE_GAIN\"></td>"
echo "        </tr>"
echo "      </table>"

echo "       <table style=\"width:${TABLE_WIDTH}\">"
echo "          <td width=\"${TABLE_WIDTH}\"><center><h3>INFO</h3></center></td>"
echo "      </table>"
echo "       <table border=\"1\" style=\"width:${TABLE_WIDTH}\">"
echo "        <tr>"
echo "          <td width=\"${ROW_TITLE_WIDTH}\">Image rev</td>"
echo "          <td width=\"${ROW_VALUE_WIDTH}\"><input type=\"text\" name=\"IMAGE_REV\" value=\"$IMAGE_REV\"></td>"
echo "        </tr>"
echo "        <tr>"
echo "          <td width=\"${ROW_TITLE_WIDTH}\">Board s/n</td>"
echo "          <td width=\"${ROW_VALUE_WIDTH}\"><input type=\"text\" name=\"BOARD_REV\" value=\"$BOARD_REV\"></td>"
echo "        </tr>"
echo "        <tr>"
echo "          <td width=\"${ROW_TITLE_WIDTH}\">Monitor s/n</td>"
echo "          <td width=\"${ROW_VALUE_WIDTH}\"><input type=\"text\" name=\"MONITOR_SN\" value=\"$MONITOR_SN\"></td>"
echo "        </tr>"
echo "        <tr>"
echo "          <td width=\"${ROW_TITLE_WIDTH}\">Date production</td>"
echo "          <td width=\"${ROW_VALUE_WIDTH}\"><input type=\"text\" name=\"PRODUCTION_DATE\" value=\"$PRODUCTION_DATE\"></td>"
echo "        </tr>"
. /tmp/backlight_on_counter
echo "        <tr>"
echo "          <td width=\"${ROW_TITLE_WIDTH}\">BL on time</td>"
echo "          <td width=\"${ROW_VALUE_WIDTH}\"><input type=\"text\" name=\"BLON_TIME\" value=\"$BACKLIGHT_ON_COUNTER\"></td>"
echo "        </tr>"
echo "        <tr>"
echo "          <td width=\"${ROW_TITLE_WIDTH}\">BL on time reset</td>"
echo "          <td width=\"${ROW_VALUE_WIDTH}\"><input type=\"text\" name=\"BLON_TIME_RESET\" value=\"$BLON_TIME_RESET\"></td>"
echo "        </tr>"
. /tmp/monitor_on_counter
echo "        <tr>"
echo "          <td width=\"${ROW_TITLE_WIDTH}\">MONITOR on time</td>"
echo "          <td width=\"${ROW_VALUE_WIDTH}\"><input type=\"text\" name=\"MONITOR_ON_COUNTER\" value=\"$MONITOR_ON_COUNTER\"></td>"
echo "        </tr>"
echo "        <tr>"
echo "          <td width=\"${ROW_TITLE_WIDTH}\">Reboot Counter</td>"
echo "          <td width=\"${ROW_VALUE_WIDTH}\"><input type=\"text\" name=\"REBOOT_COUNTER\" value=\"$REBOOT_COUNTER\"></td>"
echo "        </tr>"
echo "      </table>"

echo "       <table style=\"width:${TABLE_WIDTH}\">"
echo "          <td width=\"${TABLE_WIDTH}\"><center><h3>EVENT LIST</h3></center></td>"
echo "      </table>"
echo "       <table border=\"1\" style=\"width:${TABLE_WIDTH}\">"
echo "        <tr>"
echo "          <td width=\"${ROW_TITLE_WIDTH}\">List of last 10 alarm</td>"
echo "          <td width=\"${ROW_VALUE_WIDTH}\"><input type=\"text\" name=\"EVENTLIST\" value=\"$EVENTLIST\"></td>"
echo "        </tr>"
echo "      </table>"

echo "       <table style=\"width:${TABLE_WIDTH}\">"
echo "          <td width=\"${TABLE_WIDTH}\"><center><h3>SETUP BOOT</h3></center></td>"
echo "      </table>"
echo "       <table border=\"1\" style=\"width:${TABLE_WIDTH}\">"
echo "        <tr>"
echo "          <td width=\"${ROW_TITLE_WIDTH}\">Self test yellow square time</td>"
echo "          <td width=\"${ROW_VALUE_WIDTH}\"><input type=\"text\" name=\"YELLOW_SQUARE_TIME\" value=\"$YELLOW_SQUARE_TIME\"></td>"
echo "        </tr>"
echo "        <tr>"
echo "          <td width=\"${ROW_TITLE_WIDTH}\">TCMS comm green square time</td>"
echo "          <td width=\"${ROW_VALUE_WIDTH}\"><input type=\"text\" name=\"TCMS_GREEN_SQUARE_TIME\" value=\"$TCMS_GREEN_SQUARE_TIME\"></td>"
echo "        </tr>"
echo "        <tr>"
echo "          <td width=\"${ROW_TITLE_WIDTH}\">Waiting time for communication</td>"
echo "          <td width=\"${ROW_VALUE_WIDTH}\"><input type=\"text\" name=\"WAIT_TIME_FOR_COMMUNICATIONS\" value=\"$WAIT_TIME_FOR_COMMUNICATIONS\"></td>"
echo "        </tr>"
echo "        <tr>"
echo "          <td width=\"${ROW_TITLE_WIDTH}\">Retry time communication</td>"
echo "          <td width=\"${ROW_VALUE_WIDTH}\"><input type=\"text\" name=\"RETRY_TIME_COMMUNICATIONS\" value=\"$RETRY_TIME_COMMUNICATIONS\"></td>"
echo "        </tr>"
echo "        <tr>"
echo "          <td width=\"${ROW_TITLE_WIDTH}\">Time end red square</td>"
echo "          <td width=\"${ROW_VALUE_WIDTH}\"><input type=\"text\" name=\"TIME_END_SQUARE\" value=\"$TIME_END_SQUARE\"></td>"
echo "        </tr>"
echo "      </table>"
echo "      </form>"

echo "<br>"
echo "</body>"                                                                                          
echo "</html>"
exit 0
