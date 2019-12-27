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
App version 0.6.1 | SDK Version v4.1-dev-1572-g30372f5 | FreeMem 280376
[VFS] Started timer with period 100 ms, time since boot: 2527916/0
[VFS] DEVICE_ID='C405EF90'
[VFS] SNTP_SERVER '2.ru.pool.ntp.org' TIME_ZONE 'EET-2'
[VFS] WIFI_MODE (1): STA
[VFS] WIFI_STA_PARAM: 'armLinux:password'
[VFS] TLS_PORT: 4545
[WIFI] WIFI_MODE - STA: 'armLinux':'password'
[WIFI] Connected to AP 'armLinux' auth(3):'AUTH_WPA2_PSK' chan:2 rssi:-28
[WIFI] Local ip_addr : 192.168.0.102
[LOG] Start NetLogServer task (port=8008)| FreeMem 239936
[LOG] Wait new log_client... | FreeMem 239480
[NTP] Start sntp_task | FreeMem 237068
[NTP] Getting time from SNTP server '2.ru.pool.ntp.org'.
[NTP] The current date/time is: Fri Dec 27 10:35:56 2019 EET-2
[NTP] Stop sntp_task | FreeMem 234224
[UART] Start serial_task | FreeMem 234224
[TLS] TLS server task starting...(port=4545) | FreeMem=225404
27.12 10:35:57 | TLS : Wait new connection...
27.12 10:36:43 | TLS : New client 192.168.0.101:57030 online (sock=56)
27.12 10:36:45 | TLS : MD5 hash=788B9AF6984AA1AC82A2119BE16D1B87
27.12 10:36:45 | TLS : {"ts":1577435805}
27.12 10:36:53 | TLS : Recv. data (45 bytes) from client:{"auth":"788B9AF6984AA1AC82A2119BE16D1B87"}
27.12 10:36:53 | TLS : For client 192.168.0.101 access granted !
27.12 10:37:19 | TLS : Recv. data (20 bytes) from client:{"cmd":"GET_STAT"}
27.12 10:37:19 | UART : [1] Send(10) : GET_STAT
27.12 10:37:19 | UART : [1] Recv(13) : 'GET_STAT : 00'
27.12 10:37:19 | TLS : {"DevID":"C405EF90","Time":1577435839,"FreeMem":197776,"ip":"192.168.0.101","Answer":"GET_STAT : 00"}
27.12 10:37:34 | TLS : Recv. data (20 bytes) from client:{"cmd":"SET_ON 1"}
27.12 10:37:34 | UART : [2] Send(10) : SET_ON 1
27.12 10:37:34 | UART : [2] Recv(13) : 'SET_ON 1 : OK'
27.12 10:37:34 | TLS : {"DevID":"C405EF90","Time":1577435854,"FreeMem":197780,"ip":"192.168.0.101","Answer":"SET_ON 1 : OK"}
27.12 10:37:47 | TLS : Recv. data (20 bytes) from client:{"cmd":"GET_STAT"}
27.12 10:37:47 | UART : [3] Send(10) : GET_STAT
27.12 10:37:47 | UART : [3] Recv(13) : 'GET_STAT : 01'
27.12 10:37:47 | TLS : {"DevID":"C405EF90","Time":1577435867,"FreeMem":197780,"ip":"192.168.0.101","Answer":"GET_STAT : 01"}
27.12 10:38:02 | TLS : Recv. data (21 bytes) from client:{"cmd":"SET_OFF 1"}
27.12 10:38:02 | UART : [4] Send(11) : SET_OFF 1
27.12 10:38:02 | UART : [4] Recv(14) : 'SET_OFF 1 : OK'
27.12 10:38:02 | TLS : {"DevID":"C405EF90","Time":1577435882,"FreeMem":197780,"ip":"192.168.0.101","Answer":"SET_OFF 1 : OK"}
27.12 10:38:23 | TLS : Recv. data (20 bytes) from client:{"cmd":"GET_STAT"}
27.12 10:38:23 | UART : [5] Send(10) : GET_STAT
27.12 10:38:23 | UART : [5] Recv(13) : 'GET_STAT : 00'
27.12 10:38:23 | TLS : {"DevID":"C405EF90","Time":1577435903,"FreeMem":197776,"ip":"192.168.0.101","Answer":"GET_STAT : 00"}
27.12 10:38:48 | LOG : New log_client 192.168.0.101:42508 (soc=57) online | FreeMem 197236
27.12 10:38:57 | TLS : Recv. data (20 bytes) from client:{"cmd":"GET_STAT"}
27.12 10:38:57 | UART : [6] Send(10) : GET_STAT
27.12 10:38:57 | UART : [6] Recv(13) : 'GET_STAT : 00'
27.12 10:38:57 | TLS : {"DevID":"C405EF90","Time":1577435937,"FreeMem":197148,"ip":"192.168.0.101","Answer":"GET_STAT : 00"}
27.12 10:39:02 | LOG : Socket 57 error 0 'Success'
27.12 10:39:02 | LOG : Closed connection. Wait new tcp client... | FreeMem 197524
27.12 10:39:26 | TLS : Client closed connection (0)
27.12 10:39:26 | TLS : Wait new connection...
```
