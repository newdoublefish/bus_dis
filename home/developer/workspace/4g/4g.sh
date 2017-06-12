#!/bin/bash
pppCard="ppp0"
dns="www.baidu.com"
while true
do
#isExsit=`ls /sys/class/net | grep $pppCard`
  ls /sys/class/net | grep $pppCard
  if [ "$?" != "0" ] 
  then
	echo "$pppCard off"
	killall pppd
	pppd call wcdma
  else
	echo "$pppCard on"
  fi
  sleep 3
done
