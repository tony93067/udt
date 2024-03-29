#!/bin/bash
# Program:
#	0 BK TCP Server
PATH=/bin:/sbin:/usr/bin:/usr/sbin:/usr/local/bin:/usr/local/sbin:~/bin
LB_PATH=/home/tony/論文code/論文code/實驗二/server/udt4/src
TCP_PATH=/home/tony/論文code/論文code/實驗一/TCP/server/memcpy/test_tcp
UDT_PATH=/home/tony/論文code/論文code/實驗二/server/udt4/app
MSS=("1500" "1250" "1000" "750" "500" "250" "100")
BK=0
export PATH 	
for (( c=2; c<=2; c++ ))
do
	for str in ${MSS[@]}
	do
		export LD_LIBRARYPATH=$LB_PATH
		cd $UDT_PATH
		./udtserver 5000 $str 1 $c $BK
		ps
		sleep 10
	done
	
done

