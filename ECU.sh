#!/bin/bash

if [[ -z $1 ]] then
	echo "需要輸入介面名稱!"
	echo "舉例: ./ECU.sh vcan0"
	exit 1
fi

interface=$1

echo "按鍵"
echo "AC: e : open /r : close"
echo "Battery: q/w + b"
echo "Brake: u/i/j"
echo "Cardoor: L_shift/R_shift + a/b/x/y"
echo "Dashboard: UP/DOWN"
echo "Park: o/p"
echo "Seatbelt: k/l + a"
echo "Turnsignal: RIGHT/LEFT"

./AC $interface & ./Battery $interface & ./Brake $interface & ./Cardoor $interface & ./Dashboard $interface & ./Park $interface & ./Seatbelt $interface & ./Turnsignal $interface