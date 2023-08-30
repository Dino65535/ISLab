#!/bin/bash

if [[ -z $1 ]] then
	echo "需要輸入介面名稱!"
	echo "舉例: ./ECU.sh vcan0"
	exit 1
fi

interface=$1

echo "按鍵"
echo "AC: e/r"
echo "Battery: q/w"
echo "brake: u/i"
echo "cardoor: L_shift/R_shift + a/b/x/y"
echo "Dashboard: UP/DOWN"
echo "park: o/p"
echo "seatbelt: k/l"
echo "turnsignal: RIGHT/LEFT"

./AC $interface & ./Battery $interface & ./brake $interface & ./cardoor $interface & ./Dashboard $interface & ./park $interface & ./seatbelt $interface & ./turnsignal $interface