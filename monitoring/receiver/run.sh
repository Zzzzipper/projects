#!/bin/sh

RECEIVER_ROOT=~/receiver
RECEIVER_BIN=$RECEIVER_ROOT/bin
RECEIVER_NAME="Receiver_#0"
HTTP_PORT=8088
TESTERS_PORT=10101


pid=$(ps aux | grep '[r]eceiver ' | awk '{print $2}')
now=$(date)

if  [ $pid ]
then
#	echo "Receiver is running pid="$pid", killing process.."
	if [ "$1" == "stop" ]
	then
		kill -2 $pid
		echo "Receiver stopped.."
		exit
	fi
	echo "Receiver is running.."
else
	echo "Receiver stopped, start .."
	echo "Relaunched "$now >> error.log
	export LD_LIBRARY_PATH=$RECEIVER_BIN
	$RECEIVER_BIN/receiver --h $RECEIVER_BIN/../htdocs --w $HTTP_PORT  \
		--n $RECEIVER_NAME --p $TESTERS_PORT --l $RECEIVER_BIN/../logs --d $RECEIVER_ROOT/db.conf </dev/null &>/dev/null &
fi

