#!/bin/sh
rrdtool create pressure.rrd \
 --step 300 \
 DS:pressure:GAUGE:600:500:1500 \
 RRA:LAST:0.5:1:2016
