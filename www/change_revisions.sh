#!/bin/sh

usage () {
	echo "usage : $0 <IMAGE_REV> <BOARD_REV> <MONITOR_SN> <PRODUCTION_DATE>"
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
if [ "$4" == "" ]; then
	usage
fi

IMAGE_REV=$1
BOARD_REV=$2
MONITOR_SN=$3
PRODUCTION_DATE=$4

rm -rf /tmp/store_mountpoint
mkdir /tmp/store_mountpoint
mount /dev/mmcblk0p3 /tmp/store_mountpoint
echo "IMAGE_REV=${IMAGE_REV}" > /tmp/store_mountpoint/webparams/sw_version
echo "BOARD_REV=${BOARD_REV}" >> /tmp/store_mountpoint/webparams/hw_version
echo "MONITOR_SN=${MONITOR_SN}" >> /tmp/store_mountpoint/webparams/hw_version
echo "PRODUCTION_DATE=${PRODUCTION_DATE}" >> /tmp/store_mountpoint/webparams/hw_version
cp /tmp/store_mountpoint/webparams/sw_version /tmp/.
cp /tmp/store_mountpoint/webparams/hw_version /tmp/.
umount /tmp/store_mountpoint

echo "Version changed from user command" >> /tmp/gds_log
echo "IMAGE_REV=${IMAGE_REV}" >> /tmp/gds_log
echo "BOARD_REV=${BOARD_REV}" >> /tmp/gds_log
echo "MONITOR_SN=${MONITOR_SN}" >> /tmp/gds_log
echo "PRODUCTION_DATE=${PRODUCTION_DATE}" >> /tmp/gds_log
