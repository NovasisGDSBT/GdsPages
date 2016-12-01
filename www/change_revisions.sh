#!/bin/sh

echo "HW Version changer"
usage () {
	echo "usage : $0 <BOARD_REV> <MONITOR_SN> <PRODUCTION_DATE>"
	exit 0
}

if [ "$1" == "" ]; then
	usage
fi
if [ "$2" == "" ]; then
	usage
fi
if [ "$3" == "" ]; then
	usage
fi

BOARD_REV=$1
MONITOR_SN=$2
PRODUCTION_DATE=$3

rm -rf /tmp/store_mountpoint
mkdir /tmp/store_mountpoint
mount /dev/mmcblk0p3 /tmp/store_mountpoint
echo "BOARD_REV=${BOARD_REV}" > /tmp/store_mountpoint/webparams/hw_version
echo "MONITOR_SN=${MONITOR_SN}" >> /tmp/store_mountpoint/webparams/hw_version
echo "PRODUCTION_DATE=${PRODUCTION_DATE}" >> /tmp/store_mountpoint/webparams/hw_version
cp /tmp/store_mountpoint/webparams/hw_version /tmp/.
umount /tmp/store_mountpoint

echo "HW Version changed from user command" >> /tmp/gds_log
echo "BOARD_REV=${BOARD_REV}" >> /tmp/gds_log
echo "MONITOR_SN=${MONITOR_SN}" >> /tmp/gds_log
echo "PRODUCTION_DATE=${PRODUCTION_DATE}" >> /tmp/gds_log
