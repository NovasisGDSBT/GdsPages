#/bin/sh
SWVERSION="1.0.0.0rc0"
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
# Initialize REBOOT_COUNTER
if [ -f /tmp/store_mountpoint/webparams/reboot_counter ]; then
	cp /tmp/store_mountpoint/webparams/reboot_counter /tmp/.
else
	echo "REBOOT_COUNTER=1" > /tmp/store_mountpoint/webparams/reboot_counter
	cp /tmp/store_mountpoint/webparams/reboot_counter /tmp/.
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
		break
	fi
done

# test if page exists
. /etc/sysconfig/chromium_var
wget -s $CHROMIUM_SERVER
if [ "$?" = "0" ]; then
	PAGE_EXISTS=1
else
	PAGE_EXISTS=0
        echo "CHROMIUM_SERVER=\"http://127.0.0.1:8080/test_default_page/default_page.html\"" > /etc/sysconfig/chromium_var
fi

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
