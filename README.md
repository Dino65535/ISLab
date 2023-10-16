# ISLab

### 運行需求
* SDL2  
* SDL2_image
* SDL_ttf
* can-uitls  

      sudo apt install libsdl2-dev libsdl2-image-dev libsdl2-ttf-dev can-utils

### 執行步驟
首先編譯檔案，cd到資料夾內  

    make

之後使用設定腳本開啟虛擬 can 的 interface (預設名稱是vcan0)

    ./setup_vcan.sh

打開所有視窗

    ./ISLab.sh vcan0
***
關閉時可以輸入腳本來快速關閉所有畫面

    ./close.sh

***
腳本執行不了，可能是沒有權限  

    chmod +x ISLab.sh
