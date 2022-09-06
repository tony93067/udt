#!/bin/bash
# Program:
#	execute 1000 receivers at the time
PATH=/bin:/sbin:/usr/bin:/usr/sbin:/usr/local/bin:/usr/local/sbin:~/bin
export PATH
i=1
while [ "$i" -le "1000" ]
do
echo "Client" $i "generate"
./background_client_uplink 140.117.171.182 &
sleep 0.75
i=$(($i+1))
done
echo "Background Create Finish"

