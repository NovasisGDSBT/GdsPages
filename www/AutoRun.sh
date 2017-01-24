#/bin/sh
SWVERSION="1.0.0.0rc2"
cd /tmp
cp application_storage/www.tar .
tar xf www.tar
cp /tmp/www/xinitrc /root/.xinitrc

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
TIMEZONE=`cat /etc/timezone`
HERE=`pwd`
cd /etc
rm localtime
ln -s ../usr/share/zoneinfo/$TIMEZONE localtime
cd $HERE
# Initialize REBOOT_COUNTER
if [ -f /tmp/store_mountpoint/reboot_counter ]; then
	cp /tmp/store_mountpoint/reboot_counter /tmp/.
else
	echo "REBOOT_COUNTER=1" > /tmp/store_mountpoint/reboot_counter
	cp /tmp/store_mountpoint/reboot_counter /tmp/.
fi
if [ -f /tmp/store_mountpoint/wdog_counter ]; then
	cp /tmp/store_mountpoint/wdog_counter /tmp/.
else
	echo "WATCHDOG_COUNTER=0" > /tmp/store_mountpoint/store_wdog_counter
	cp /tmp/store_mountpoint/wdog_counter /tmp/.
fi

umount /tmp/store_mountpoint

echo "0" > /tmp/api_mod
echo "0" > /tmp/wdog_api_mod


# setup web server
cp /tmp/www/lighttpd.conf /etc/lighttpd/.
cp /tmp/www/etcX11xinitrc /etc/X11/xinit/xinitrc
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
if [ -f /etc/sysconfig/GammaDefaultValue ]; then
	GAMMAVALUE=$(cat /etc/sysconfig/GammaDefaultValue)
	/tmp/www/GdsGamma $(echo $GAMMAVALUE)
fi

# finishing up
touch /tmp/backlight_on
cd /tmp/www

# wd management
./GDS_WdtFuncs &

# wait for dhcp as MTE seems not so fast as it should and we need to have an IP to check the page presence
COUNT=0
WAITIP=45
while [ ! -f /tmp/my_ip ]; do
        sleep 1
	let COUNT=$COUNT+1
	if [ "$COUNT" -ge "$WAITIP" ]; then
		# Assign a fake address so X can start
		ifconfig eth0 10.0.0.199 up
		echo "10.0.0.199" > /tmp/my_ip
		#kill -9 `pidof udhcpc`
		echo "No dhcp server found, default to 10.0.0.199"
		break
	fi
done

# test if page exists
sleep 5
. /etc/sysconfig/chromium_var
wget -s -T 10 $CHROMIUM_SERVER
if [ "$?" = "0" ]; then
	PAGE_EXISTS=1
	cat /etc/sysconfig/chromium_var | sed 's/CHROMIUM_SERVER=//g' > /tmp/www/url.txt
else
	PAGE_EXISTS=0
        echo "CHROMIUM_SERVER=\"http://127.0.0.1:8080/test_default_page/default_page.html\"" > /etc/sysconfig/chromium_var
        echo "http://127.0.0.1:8080/test_default_page/default_page.html" > /tmp/www/url.txt
fi
cat /etc/sysconfig/chromium_var

############### GDS APP IPTCOM #################
# will start X 
#
export SDL_NOMOUSE=1
TIMEOUTTCMS=`cat /tmp/setup_boot | grep WAIT_TIME_FOR_COMMUNICATIONS | sed 's/WAIT_TIME_FOR_COMMUNICATIONS=//g'`
YELLOW_TIME=`cat /tmp/setup_boot | grep YELLOW_SQUARE_TIME | sed 's/YELLOW_SQUARE_TIME=//g'`
GREEN_TIME=`cat /tmp/setup_boot | grep TCMS_GREEN_SQUARE_TIME | sed 's/TCMS_GREEN_SQUARE_TIME=//g'`
RED_TIME=`cat /tmp/setup_boot | grep TIME_END_SQUARE | sed 's/TIME_END_SQUARE=//g'`
./GDSBT_iptcom $YELLOW_TIME $GREEN_TIME $RED_TIME $TIMEOUTTCMS $PAGE_EXISTS &

/tmp/www/check_xml_fileSize.sh &
