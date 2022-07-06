#!/bin/bash
# Program:
#	execute 20 receivers at the time
PATH=/bin:/sbin:/usr/bin:/usr/sbin:/usr/local/bin:/usr/local/sbin:~/bin
export PATH
area=2
while [ "$i" != "80" ]
do
echo "Client" $i
rand=`head -1 /dev/urandom | od -N 1 | awk '{ print $2 }'`
seconds=`expr $rand % $area`
echo $seconds
#sleep $seconds
./udtclient 140.117.170.97  5515 300 1 &
i=$(($i+1))
done

