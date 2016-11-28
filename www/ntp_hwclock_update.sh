#!/bin/sh
TRUE="1"
                            
while [ "$TRUE" == "1" ]; do
        sleep 5             
	/sbin/hwclock -w
done

