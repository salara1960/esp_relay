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
App version 0.6.4 | SDK Version v4.1-dev-1572-g30372f5 | FreeMem 280336
[VFS] Started timer with period 100 ms, time since boot: 2527921/0
[VFS] DEVICE_ID='C405EF90'
[VFS] SNTP_SERVER '2.ru.pool.ntp.org' TIME_ZONE 'EET-2'
[VFS] WIFI_MODE (1): STA
[VFS] WIFI_STA_PARAM: 'ssid:password'
[VFS] TLS_PORT: 4545
[WIFI] WIFI_MODE - STA: 'ssid':'password'
[WIFI] Connected to AP 'ssid' auth(3):'AUTH_WPA2_PSK' chan:10 rssi:-25
[WIFI] Local ip_addr : 192.168.0.100
[LOG] Start NetLogServer task (port=8008)| FreeMem 239592
[LOG] Wait new log_client... | FreeMem 239144
[NTP] Start sntp_task | FreeMem 236732
[NTP] Getting time from SNTP server '2.ru.pool.ntp.org'.
[UART] Start serial_task | FreeMem 231944
[TLS] TLS server task starting...(port=4545) | FreeMem=223124
[TLS] Wait new connection...
[NTP] The current date/time is: Mon Dec 30 10:28:41 2019 EET-2
[NTP] Stop sntp_task | FreeMem 195988
30.12 10:28:42 [LOG] New log_client 192.168.0.101:36490 (soc=56) online | FreeMem 197860
30.12 10:28:56 [TLS] New client 192.168.0.101:46532 online (sock=57)
30.12 10:28:57 [TLS] MD5 hash=C82DB06387A27CEB91E316FC2B0E89C9
30.12 10:28:57 [TLS] {"ts":1577694537}
30.12 10:29:05 [TLS] Recv. data (45 bytes) from client:{"auth":"C82DB06387A27CEB91E316FC2B0E89C9"}
30.12 10:29:05 [TLS] For client 192.168.0.101 access granted !
30.12 10:29:25 [TLS] Recv. data (20 bytes) from client:{"cmd":"GET_STAT"}
30.12 10:29:25 [UART] [1] Send(10) : GET_STAT
30.12 10:29:25 [UART] [1] Recv(13) : 'GET_STAT : 00'
30.12 10:29:25 [TLS] {"DevID":"C405EF90","Time":1577694565,"FreeMem":197096,"cli":"192.168.0.101","Answer":"GET_STAT : 00"}
30.12 10:29:44 [TLS] Recv. data (22 bytes) from client:{"cmd":"SET_ON 1 5"}
30.12 10:29:45 [UART] [2] Send(12) : SET_ON 1 5
30.12 10:29:45 [UART] [2] Recv(15) : 'SET_ON 1 5 : OK'
30.12 10:29:45 [TLS] {"DevID":"C405EF90","Time":1577694585,"FreeMem":197092,"cli":"192.168.0.101","Answer":"SET_ON 1 5 : OK"}
30.12 10:30:05 [TLS] Recv. data (20 bytes) from client:{"cmd":"SET_ON 4"}
30.12 10:30:05 [UART] [3] Send(10) : SET_ON 4
30.12 10:30:05 [UART] [3] Recv(13) : 'SET_ON 4 : OK'
30.12 10:30:05 [TLS] {"DevID":"C405EF90","Time":1577694605,"FreeMem":197096,"cli":"192.168.0.101","Answer":"SET_ON 4 : OK"}
30.12 10:30:18 [TLS] Recv. data (20 bytes) from client:{"cmd":"GET_STAT"}
30.12 10:30:18 [UART] [4] Send(10) : GET_STAT
30.12 10:30:18 [UART] [4] Recv(13) : 'GET_STAT : 08'
30.12 10:30:18 [TLS] {"DevID":"C405EF90","Time":1577694618,"FreeMem":197096,"cli":"192.168.0.101","Answer":"GET_STAT : 08"}
30.12 10:30:35 [TLS] Recv. data (21 bytes) from client:{"cmd":"SET_OFF 4"}
30.12 10:30:35 [UART] [5] Send(11) : SET_OFF 4
30.12 10:30:35 [UART] [5] Recv(14) : 'SET_OFF 4 : OK'
30.12 10:30:36 [TLS] {"DevID":"C405EF90","Time":1577694635,"FreeMem":197088,"cli":"192.168.0.101","Answer":"SET_OFF 4 : OK"}
30.12 10:30:47 [TLS] Recv. data (20 bytes) from client:{"cmd":"GET_STAT"}
30.12 10:30:47 [UART] [6] Send(10) : GET_STAT
30.12 10:30:47 [UART] [6] Recv(13) : 'GET_STAT : 00'
30.12 10:30:47 [TLS] {"DevID":"C405EF90","Time":1577694647,"FreeMem":197096,"cli":"192.168.0.101","Answer":"GET_STAT : 00"}
30.12 10:30:54 [LOG] Socket 56 error 0 'Success'
30.12 10:30:54 [LOG] Closed connection. Wait new tcp client... | FreeMem 197464
30.12 10:30:56 [TLS] Client closed connection (0)
30.12 10:30:56 [TLS] Wait new connection...
```
