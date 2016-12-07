#/bin/sh
SWVERSION="1.0.0.0rc0"
cd /tmp
cp application_storage/www.tar .
tar xf www.tar

# Retrieve all parameters
rm -rf /tmp/store_mountpoint
mkdir /tmp/store_mountpoint
mount /dev/mmcblk0p3 /tmp/store_mountpoint

if [ -d /tmp/store_mountpoint/webparams ]; then
	FILES=`ls /tmp/www/defaults`
	for i in ${FILES}; do
		if [ -f /tmp/store_mountpoint/webparams/$i ]; then
			cp /tmp/store_mountpoint/webparams/$i /tmp/.
		else
			cp /tmp/www/defaults/$i /tmp/.
			cp /tmp/www/defaults/$i /tmp/store_mountpoint/webparams/.
		fi	
	done
	if [ -f /tmp/store_mountpoint/sysconfig/mac_addr ]; then
        	cp /tmp/store_mountpoint/sysconfig/mac_addr /tmp/.
	else
		cat /sys/class/net/eth0/address > /tmp/store_mountpoint/sysconfig/mac_addr
		cat /sys/class/net/eth0/address > /tmp/mac_addr
	fi
	cp /tmp/store_mountpoint/webparams/* /tmp/.
else
	mkdir /tmp/store_mountpoint/webparams
	cp /tmp/www/defaults/* /tmp/.
	cp /tmp/www/defaults/* /tmp/store_mountpoint/webparams/.
fi
echo "IMAGE_REV=${SWVERSION}" > /tmp/store_mountpoint/webparams/sw_version
cp /tmp/store_mountpoint/webparams/sw_version /tmp
# Default set for timezone
if [ -f /tmp/timezone ]; then
        cp /tmp/timezone /etc/timezone
else
        echo "Europe/Rome" > /etc/timezone
fi
# Initialize REBOOT_COUNTER
if [ -f /tmp/store_mountpoint/reboot_counter ]; then
	cp /tmp/store_mountpoint/reboot_counter /tmp/.
	. /tmp/reboot_counter
	let REBOOT_COUNTER=$REBOOT_COUNTER+1
	echo "REBOOT_COUNTER=$REBOOT_COUNTER" > /tmp/reboot_counter
	cp /tmp/reboot_counter /tmp/store_mountpoint/reboot_counter
else
	echo "REBOOT_COUNTER=1" > /tmp/store_mountpoint/reboot_counter
	cp /tmp/store_mountpoint/reboot_counter /tmp/.
fi
umount /tmp/store_mountpoint
echo "0" > /tmp/wdog_counter
echo "0" > /tmp/api_mod
echo "0" > /tmp/wdog_api_mod

# 30 seconds management for yellow square. Note : the kernel can't gurantee the minimu 3 secs required
export SDL_NOMOUSE=1
COUNT=0
LIMIT=`cat /tmp/setup_boot | grep YELLOW_SQUARE_TIME | sed 's/YELLOW_SQUARE_TIME=//g'`
while [ "$COUNT" -ge "$LIMIT" ]; do
	sleep 1
        let COUNT=$COUNT+1
done

# 120 secs max timeout communication at startup
TIMEOUTTCMS=0
WAITTIME=`cat /tmp/setup_boot | grep WAIT_TIME_FOR_COMMUNICATIONS | sed 's/WAIT_TIME_FOR_COMMUNICATIONS=//g'`
while [ ! -f /tmp/my_ip ]; do
        sleep 1
	let COUNT=$COUNT+1
	if [ "$COUNT" -ge "$WAITTIME" ]; then
		TIMEOUTTCMS=1
		break
	fi
done

# Timeout management with ERROR TYPE 1
/tmp/www/cgi-bin/find_lvds
if [ "$TIMEOUTTCMS" = "0" ]; then
	/tmp/www/POST_GreenSquare `cat /tmp/setup_boot | grep TCMS_GREEN_SQUARE_TIME | sed 's/TCMS_GREEN_SQUARE_TIME=//g'`
else
	/tmp/www/GdsScreenTest &
	sleep 1
        /tmp/www/GdsScreenTestWrite START
	sleep 1
	/tmp/www/GdsScreenTestWrite DIAG
	sleep 1
	exit 0
fi

TIMEOUT=15
if [ -f /usr/bin/startx ]; then
        while ! [ -f /tmp/system_ready ]; do
                sleep 1
                let TIMEOUT=${TIMEOUT}-1
                if [ "${TIMEOUT}" == "0" ]; then
                        break
                fi
        done
        kill -HUP `pidof X` >/dev/null 2>&1
        kill -HUP `pidof x11vnc` >/dev/null 2>&1
        export DISPLAY=":0.0"
        /usr/bin/startx &
        if [ -f /tmp/application_storage/bkgwm.png ]; then
                sleep 1
                fbsetbg -c /tmp/application_storage/bkgwm.png
        fi
fi


RED_GAIN=255
GREEN_GAIN=250
BLUE_GAIN=245

# This is for first start
if ! [ -f /tmp/rgb_matrix ]; then
        echo "$RED_GAIN,0,0" > /tmp/rgb_matrix
        echo "0,$GREEN_GAIN,0" >> /tmp/rgb_matrix
        echo "0,0,$BLUE_GAIN" >> /tmp/rgb_matrix
        echo "0,0,0"   >> /tmp/rgb_matrix
        echo "2,2,2"   >> /tmp/rgb_matrix
        echo "RED_GAIN=$RED_GAIN" > /tmp/rgb_settings
        echo "GREEN_GAIN=$GREEN_GAIN" >> /tmp/rgb_settings
        echo "BLUE_GAIN=$BLUE_GAIN" >> /tmp/rgb_settings
fi

# Finished

# setup web server
cp /tmp/www/lighttpd.conf /etc/lighttpd/.
cp /tmp/www/cgi.conf /etc/lighttpd/conf.d/.
/etc/init.d/S50lighttpd restart
sleep 1
touch /tmp/voltage24_value /tmp/voltage12_value
touch /tmp/voltages_readout
touch /tmp/processor_temp
touch /tmp/carrier_temp
touch /tmp/alarms
touch /tmp/backlight_sensor_value
touch /tmp/ambientlight_value

###############       Daemons          #################
# start application
# auto_backlight_bkg sets the brightness to MAX@MAXLIGHT
/tmp/www/auto_backlight_bkg &
# start counter
/tmp/www/monitor_counter.sh &
/tmp/www/backlight_counter.sh &
# start ntp monitor
/tmp/www/ntp_hwclock_update.sh &
# start chrome_starter
/tmp/www/chrome_starter.sh &

# set gamma default value
if [ -f /etc/sysconfig/GammaDefaultValue ]
then
 GAMMAVALUE=$(cat /etc/sysconfig/GammaDefaultValue)
 /tmp/www/GdsGamma $(echo $GAMMAVALUE)
 fi
# Finishing up
touch /tmp/backlight_on
/tmp/www/apply_rgbmatrix &


cd /tmp/www
./GDSBT_iptcom &

