#!/bin/sh
# selezionare un range di valori tramite il timestamp
xml sel -t -m "logging/LOG[@timestamp>=$1 and @timestamp<=$2]" -v . -n /tmp/www/gds_log.xml >> /tmp/logMessageSelected.txt

