# ISLab

### 運行需求
* SDL2  
* SDL2_image
* SDL_ttf
* can-uitls  

      sudo apt install libsdl2-dev libsdl2-image-dev libsdl2-ttf-dev can-utils

### 執行步驟
首先使用設定腳本開啟虛擬 can 的 interface (預設名稱是vcan0)

    ./setup_vcan.sh

接著開啟儀表板

    ./ICSim vcan0

然後打開所有設備

    ./ECU.sh vcan0
***
關閉時可以輸入腳本來快速關閉所有畫面

    ./close.sh
