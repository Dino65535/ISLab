#!/bin/bash

if [[ -z $1 ]] then
	echo "需要輸入介面名稱!"
	echo "舉例: ./ECU.sh vcan0"
	exit 1
fi

interface=$1

echo "按鍵"
echo "AC"
echo "    U : 開啟 / I : 關閉"
echo "Battery"
echo "    A : 動作一 / S : 動作二 / D : 與充電樁通訊"
echo "Brake"
echo "    Q : 不亮 / W : 黃燈 / E : 紅燈"
echo "Cardoor"
echo "    右shift : 開啟 / 左shift : 關閉 + A/B/X/Y"
echo "Dashboard"
echo "    方向鍵上 : 加速"
echo "Park"
echo "    R : 不亮 / T : 黃燈 / Y : 紅燈"
echo "Seatbelt"
echo "    F : 不亮 / G : 紅燈 / H : 開啟網址輸入"
echo "Turnsignal"
echo "    方向鍵右 : 右轉燈 / 方向鍵左 : 左轉燈" 

./AC $interface & ./Battery $interface & ./Brake $interface & ./Cardoor $interface & ./Dashboard $interface & ./Park $interface & ./Seatbelt $interface & ./Turnsignal $interface