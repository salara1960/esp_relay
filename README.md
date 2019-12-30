# esp_relay - модуль управление блоком из 8-ми реле
(эмулятор реле тут https://github.com/salara1960/relay_box)

esp32(DevKitC board) + ssd1306(i2c) : uart + FreeRTOS + sntp_server + udp_server + tls_server + log_server

# Файлы пакета:

* sdkconfing     - файл конфигурации проекта

* Makefile       - make файл (файл сценария компиляции проекта)

* version        - файл версии ПО

* README.md      - файл справки

* main/          - папка исходников

* partitions.csv - файл конфигурации разделов dataflah

* utils.tar.bz2  - архив утилит для тестирование (tls-клиент, эмулятор блока реле)


Требуемые компоненты:
```
- Cross compiler xtensa-esp32-elf (http://esp-idf-fork.readthedocs.io/en/stable/linux-setup.html#step-0-prerequisites)
- SDK esp-idf (https://github.com/espressif/esp-idf)
- Python2 (https://www.python.org/)
```


# Компиляция и загрузка

make menuconfig - конфигурация проекта

make app        - компиляция проекта

make flash      - запись бинарного кода проекта в dataflash


# Пример логов при работе модуля :
```
App version 0.7 | SDK Version v4.1-dev-1572-g30372f5 | FreeMem 280328
[VFS] Started timer with period 100 ms, time since boot: 2527907/0
[VFS] DEVICE_ID='C405EF90'
[VFS] SNTP_SERVER '2.ru.pool.ntp.org' TIME_ZONE 'EET-2'
[VFS] WIFI_MODE (1): STA
[VFS] WIFI_STA_PARAM: 'armLinux:armLinux32'
[VFS] TLS_PORT: 4545
[WIFI] WIFI_MODE - STA: 'armLinux':'armLinux32'
[WIFI] Connected to AP 'armLinux' auth(3):'AUTH_WPA2_PSK' chan:2 rssi:-27
[WIFI] Local ip_addr : 192.168.0.100
[LOG] Start NetLogServer task (port=8008)| FreeMem 239500
[LOG] Wait new log_client... | FreeMem 239052
[NTP] Start sntp_task | FreeMem 236640
[NTP] Getting time from SNTP server '2.ru.pool.ntp.org'.
[UART] Start serial_task | FreeMem 231852
[TLS] TLS server task starting...(port=4545) | FreeMem=222948
[TLS] Wait new connection...
[NTP] The current date/time is: Mon Dec 30 18:27:37 2019 EET-2
[NTP] Stop sntp_task | FreeMem 195432
30.12 18:27:37 [LOG] New log_client 192.168.0.101:43614 (soc=56) online | FreeMem 195352
30.12 18:27:50 [TLS] New client 192.168.0.101:44246 online (sock=57)
30.12 18:27:52 [TLS] MD5 hash=926E81FF8478B227A77F208544BCDB10
30.12 18:27:52 [TLS] {"ts":1577723272}
30.12 18:27:59 [TLS] Recv. data (45 bytes) from client:{"auth":"926E81FF8478B227A77F208544BCDB10"}
30.12 18:27:59 [TLS] For client 192.168.0.101 access granted !
30.12 18:28:13 [TLS] Recv. data (20 bytes) from client:{"cmd":"GET_STAT"}
30.12 18:28:13 [UART] [1] Send(10) : GET_STAT
30.12 18:28:13 [UART] [1] Recv(13) : 'GET_STAT : 00'
30.12 18:28:13 [TLS] {"DevID":"C405EF90","Time":1577723293,"FreeMem":197004,"cli":"192.168.0.101","Vcc":3.276,"Temp":38.00,"Answer":"GET_STAT : 00"}
30.12 18:28:29 [TLS] Recv. data (22 bytes) from client:{"cmd":"SET_ON 1 5"}
30.12 18:28:29 [UART] [2] Send(12) : SET_ON 1 5
30.12 18:28:29 [UART] [2] Recv(15) : 'SET_ON 1 5 : OK'
30.12 18:28:29 [TLS] {"DevID":"C405EF90","Time":1577723309,"FreeMem":196784,"cli":"192.168.0.101","Vcc":3.276,"Temp":38.00,"Answer":"SET_ON 1 5 : OK"}
30.12 18:28:51 [TLS] Recv. data (20 bytes) from client:{"cmd":"SET_ON 2"}
30.12 18:28:51 [UART] [3] Send(10) : SET_ON 2
30.12 18:28:51 [UART] [3] Recv(13) : 'SET_ON 2 : OK'
30.12 18:28:51 [TLS] {"DevID":"C405EF90","Time":1577723331,"FreeMem":195384,"cli":"192.168.0.101","Vcc":3.276,"Temp":38.00,"Answer":"SET_ON 2 : OK"}
30.12 18:29:03 [TLS] Recv. data (20 bytes) from client:{"cmd":"GET_STAT"}
30.12 18:29:03 [UART] [4] Send(10) : GET_STAT
30.12 18:29:03 [UART] [4] Recv(13) : 'GET_STAT : 02'
30.12 18:29:03 [TLS] {"DevID":"C405EF90","Time":1577723343,"FreeMem":195384,"cli":"192.168.0.101","Vcc":3.276,"Temp":38.00,"Answer":"GET_STAT : 02"}
30.12 18:29:22 [TLS] Recv. data (21 bytes) from client:{"cmd":"SET_OFF 2"}
30.12 18:29:22 [UART] [5] Send(11) : SET_OFF 2
30.12 18:29:22 [UART] [5] Recv(14) : 'SET_OFF 2 : OK'
30.12 18:29:22 [TLS] {"DevID":"C405EF90","Time":1577723362,"FreeMem":196784,"cli":"192.168.0.101","Vcc":3.276,"Temp":38.00,"Answer":"SET_OFF 2 : OK"}
30.12 18:29:45 [TLS] Recv. data (20 bytes) from client:{"cmd":"GET_STAT"}
30.12 18:29:45 [UART] [6] Send(10) : GET_STAT
30.12 18:29:45 [UART] [6] Recv(13) : 'GET_STAT : 00'
30.12 18:29:45 [TLS] {"DevID":"C405EF90","Time":1577723385,"FreeMem":195384,"cli":"192.168.0.101","Vcc":3.276,"Temp":38.00,"Answer":"GET_STAT : 00"}
30.12 18:30:28 [TLS] Recv. data (20 bytes) from client:{"cmd":"GET_STAT"}
30.12 18:30:28 [UART] [7] Send(10) : GET_STAT
30.12 18:30:28 [UART] [7] Recv(13) : 'GET_STAT : 00'
30.12 18:30:28 [TLS] {"DevID":"C405EF90","Time":1577723428,"FreeMem":196788,"cli":"192.168.0.101","Vcc":3.276,"Temp":38.00,"Answer":"GET_STAT : 00"}
30.12 18:30:55 [TLS] Client closed connection (0)
30.12 18:30:55 [TLS] Wait new connection...
30.12 18:31:01 [LOG] Socket 56 error 0 'Success'
30.12 18:31:01 [LOG] Closed connection. Wait new tcp client... | FreeMem 196216
```
