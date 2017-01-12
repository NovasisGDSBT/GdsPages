#!/bin/sh
MULTICAST_IP_ADDR="239.255.12.42"
VIDEO_UDP_PORT=5004

# PARSE QUERY STRING
[ "$REQUEST_METHOD" = "POST" ] && read QUERY_STRING
saveIFS=$IFS
IFS='&'
set -- $QUERY_STRING
MULTICAST_IP_ADDR=`echo $1 | sed 's/M=//g'`
VIDEO_UDP_PORT=`echo $2 | sed 's/P=//g'`
WIDTH=`echo $3 | sed 's/W=//g'`
HEIGHT=`echo $4 | sed 's/H=//g'`
TOP=`echo $5 | sed 's/T=//g'`
LEFT=`echo $6 | sed 's/L=//g'`

kill -9 `pidof gst-launch-0.10` >/dev/null 2>&1
sleep 1

if [ "${MULTICAST_IP_ADDR}" == "" ]; then
	echo "Please select the multicast addres to listen to<br>"
	echo "Typical command is :<br>"
	echo "<ip_address>:8080/cgi-bin/start.cgi?M=239.255.12.42&P=5004&W=960&H=190&T=790&L=960<br>"
	echo "will get from multicast address 239.255.12.42 on port 5004 with width of 960 and height of 190, on screen at 790 pixels from top and 960 pixels from left<br>"
	exit 0
fi
if [ "${VIDEO_UDP_PORT}" == "" ]; then
	echo "Please select the multicast port to listen to"
	echo "Typical command is :<br>"
	echo "<ip_address>:8080/cgi-bin/start.cgi?M=239.255.12.42&P=5004&W=960&H=190&T=790&L=960<br>"
	echo "will get from multicast address 239.255.12.42 on port 5004 with width of 960 and height of 190, on screen at 790 pixels from top and 960 pixels from left<br>"
	exit 0
fi

if [ "${WIDTH}" == "" ]; then
	echo "Running at full screen as no width parameter is given"
	gst-launch-0.10 udpsrc multicast-group=${MULTICAST_IP_ADDR} auto-multicast=true port=${VIDEO_UDP_PORT} \
	caps = 'application/x-rtp' \
	! rtph264depay ! vpudec ! mfw_v4lsink sync=false >/dev/null 2>&1 &
	echo "Started on ${MULTICAST_IP_ADDR}:${VIDEO_UDP_PORT} at full screen"
	exit 0
fi

if [ "${HEIGHT}" == "" ]; then
	echo "Running at full screen as no height parameter is given"
	gst-launch-0.10 udpsrc multicast-group=${MULTICAST_IP_ADDR} auto-multicast=true port=${VIDEO_UDP_PORT} \
	caps = 'application/x-rtp' \
	! rtph264depay ! vpudec ! mfw_v4lsink sync=false >/dev/null 2>&1 &
	echo "Started on ${MULTICAST_IP_ADDR}:${VIDEO_UDP_PORT} at full screen"
	exit 0
fi
if [ "${TOP}" == "" ]; then
        echo "Running with origin 0,0 as no top parameter is given"
        gst-launch-0.10 udpsrc multicast-group=${MULTICAST_IP_ADDR} auto-multicast=true port=${VIDEO_UDP_PORT} \
        caps = 'application/x-rtp' \
        ! rtph264depay ! vpudec ! mfw_v4lsink sync=false disp-width=${WIDTH} disp-height=${HEIGHT}>/dev/null 2>&1 &
        echo "Started on ${MULTICAST_IP_ADDR}:${VIDEO_UDP_PORT} at 0,0"
        exit 0
fi
if [ "${LEFT}" == "" ]; then
        echo "Running with origin 0,0 as no left parameter is given"
        gst-launch-0.10 udpsrc multicast-group=${MULTICAST_IP_ADDR} auto-multicast=true port=${VIDEO_UDP_PORT} \
        caps = 'application/x-rtp' \
        ! rtph264depay ! vpudec ! mfw_v4lsink sync=false disp-width=${WIDTH} disp-height=${HEIGHT}>/dev/null 2>&1 &
        echo "Started on ${MULTICAST_IP_ADDR}:${VIDEO_UDP_PORT} at 0,0"
        exit 0
fi


gst-launch-0.10 udpsrc multicast-group=${MULTICAST_IP_ADDR} auto-multicast=true port=${VIDEO_UDP_PORT} \
caps = 'application/x-rtp' \
! rtph264depay ! vpudec ! mfw_v4lsink sync=false disp-width=${WIDTH} disp-height=${HEIGHT} axis-top=${TOP} axis-left=${LEFT} >/dev/null 2>&1 &
echo "Started on ${MULTICAST_IP_ADDR}:${VIDEO_UDP_PORT} width=${WIDTH} height=${HEIGHT} axis-top=${TOP} axis-left=${LEFT}<br>"
echo "Command is :<br>"
echo "gst-launch-0.10 udpsrc multicast-group=${MULTICAST_IP_ADDR} auto-multicast=true port=${VIDEO_UDP_PORT} caps = 'application/x-rtp' ! rtph264depay ! vpudec ! mfw_v4lsink sync=false disp-width=${WIDTH} disp-height=${HEIGHT} axis-top=${TOP} axis-left=${LEFT}<br>"

