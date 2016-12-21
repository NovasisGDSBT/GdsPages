#/bin/sh
SWVERSION="1.0.0.0rc0"
cd /tmp
cp application_storage/www.tar .
tar xf www.tar
cp /tmp/application_storage/xinitrc /root/.xinitrc

# Retrieve all parameters
rm -rf /tmp/store_mountpoint
mkdir /tmp/store_mountpoint
mount /dev/mmcblk0p3 /tmp/store_mountpoint

if [ -d /tmp/store_mountpoint/webparams ]; then
	cp -r /tmp/store_mountpoint/webparams/* /tmp/.
else
	mkdir /tmp/store_mountpoint/webparams
	cp /tmp/www/defaults/* /tmp/.
	cp /tmp/www/defaults/* /tmp/store_mountpoint/webparams/.
fi


#retrieve log file
mkdir -p /tmp/log_mountpoint
mount /dev/mmcblk0p2 /tmp/log_mountpoint

 if [ $? -eq 0 ]  # test mount OK
 then
    if [ -f /tmp/log_mountpoint/gds_log.xml ]; then
	    cp /tmp/log_mountpoint/gds_log.xml /tmp/www/
    fi
    umount /tmp/log_mountpoint
    sync
 fi

# Default page, downloadable
if [ -d /tmp/store_mountpoint/default_page ]; then
	cp /tmp/store_mountpoint/default_page/* /tmp/www/test_default_page.
else
	mkdir /tmp/store_mountpoint/default_page
	cp -r /tmp/www/test_default_page/* /tmp/store_mountpoint/default_page/.
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
else
	echo "REBOOT_COUNTER=1" > /tmp/store_mountpoint/reboot_counter
	cp /tmp/store_mountpoint/reboot_counter /tmp/.
fi
umount /tmp/store_mountpoint

! [ -f /tmp/wdog_counter ] && echo "0" > /tmp/wdog_counter
echo "0" > /tmp/api_mod
echo "0" > /tmp/wdog_api_mod


# setup web server
cp /tmp/www/lighttpd.conf /etc/lighttpd/.
cp /tmp/www/lighttpd.modules.conf /etc/lighttpd/modules.conf
cp -r /tmp/www/lighttpd.conf.d /etc/lighttpd/conf.d
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

/tmp/www/cgi-bin/find_lvds
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

# start configuration HW and SW
/tmp/www/loadHwSwVersion.sh &

# set gamma default value
if [ -f /etc/sysconfig/GammaDefaultValue ]
then
 GAMMAVALUE=$(cat /etc/sysconfig/GammaDefaultValue)
 /tmp/www/GdsGamma $(echo $GAMMAVALUE)
 fi
# Finishing up
touch /tmp/backlight_on
cd /tmp/www


############### Watch Dogs Management #################
./GDS_WdtFuncs &

############### GDS APP IPTCOM #################

./GDSBT_iptcom &

# 120 secs max timeout communication at startup
TIMEOUTTCMS=0
WAITTIME=`cat /tmp/setup_boot | grep WAIT_TIME_FOR_COMMUNICATIONS | sed 's/WAIT_TIME_FOR_COMMUNICATIONS=//g'`
# The file /tmp/www/POST_enable is created by iptcom app when tcms is not reachable
while [ ! -f /tmp/www/POST_enable ]; do
        sleep 1
	let COUNT=$COUNT+1
	if [ "$COUNT" -ge "$WAITTIME" ]; then
		TIMEOUTTCMS=1
		break
	fi
done

# 30 seconds management for yellow square. Note : the kernel can't gurantee the minimum 3 secs required
export SDL_NOMOUSE=1
/tmp/www/POST_UpperLeftSquare `cat /tmp/setup_boot | grep YELLOW_SQUARE_TIME | sed 's/YELLOW_SQUARE_TIME=//g'` YELLOW
# Timeout management with ERROR TYPE 1
if [ "$TIMEOUTTCMS" = "0" ]; then
	/tmp/www/POST_UpperLeftSquare `cat /tmp/setup_boot | grep TCMS_GREEN_SQUARE_TIME | sed 's/TCMS_GREEN_SQUARE_TIME=//g'` GREEN
else
	/tmp/www/POST_UpperLeftSquare `cat /tmp/setup_boot | grep TIME_END_SQUARE | sed 's/TIME_END_SQUARE=//g'` RED
	echo "CHROMIUM_SERVER=\"http://127.0.0.1:8080/test_default_page/default_page.html\"" > /etc/sysconfig/chromium_var
        /usr/bin/startx &
	kill -9 `pidof GDSBT_iptcom`
	# Disconnects from the net, bypass relè off
	echo 0 > /sys/class/gpio/gpio24/value
	exit 0
fi

###############       X - Chrome          #################
TIMEOUT=15
if [ -f /usr/bin/startx ]; then
        while ! [ -f /tmp/system_ready ]; do
                sleep 1
                let TIMEOUT=${TIMEOUT}-1
                if [ "${TIMEOUT}" == "0" ]; then
                        break
                fi
        done
	/tmp/www/apply_rgbmatrix
        kill -HUP `pidof X` >/dev/null 2>&1
        kill -HUP `pidof x11vnc` >/dev/null 2>&1
        export DISPLAY=":0.0"
        /usr/bin/startx &
fi

###############  rolling log file Management #######
/tmp/www/check_xml_fileSize.sh &

#./watch_dog_IPTCOM.sh &
#./chrome_keepalive.sh &
#/tmp/www/apply_rgbmatrix &

