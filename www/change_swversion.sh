#!/bin/sh

echo "SW Version changer"

usage () {
	echo "usage : $0 <IMAGE_REV>"
	exit 0
}

if [ "$1" == "" ]; then
	usage
fi

IMAGE_REV=$1

rm -rf /tmp/store_mountpoint
mkdir /tmp/store_mountpoint
mount /dev/mmcblk0p3 /tmp/store_mountpoint
echo "IMAGE_REV=${IMAGE_REV}" > /tmp/store_mountpoint/webparams/sw_version
cp /tmp/store_mountpoint/webparams/sw_version /tmp/.
umount /tmp/store_mountpoint

echo "Software Version changed from user command" >> /tmp/gds_log
echo "IMAGE_REV=${IMAGE_REV}" >> /tmp/gds_log
