#!/bin/bash

if [[ -z $1 ]] then
	echo "需要輸入介面名稱!"
	echo "舉例: ./ECU.sh vcan0"
	exit 1
fi

interface=$1

echo "按鍵"
echo "AC: e : open /r : close"
echo "Battery: q/w"
echo "Brake: u/i/j"
echo "Cardoor: L_shift/R_shift + a/b/x/y"
echo "Dashboard: UP/DOWN"
echo "park: o/p"
echo "seatbelt: k/l"
echo "turnsignal: RIGHT/LEFT"

./AC $interface & ./Battery $interface & ./Brake $interface & ./Cardoor $interface & ./Dashboard $interface & ./park $interface & ./seatbelt $interface & ./turnsignal $interface