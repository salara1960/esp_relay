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
App version 0.8 | SDK Version v4.1-dev-1795-gca8fac8 | FreeMem 280296
[VFS] Started timer with period 100 ms, time since boot: 2527905/0
[VFS] DEVICE_ID='C405EF90'
[VFS] SNTP_SERVER '2.ru.pool.ntp.org' TIME_ZONE 'EET-2'
[VFS] WIFI_MODE (1): STA
[VFS] WIFI_STA_PARAM: 'ssid:password'
[VFS] TLS_PORT: 4545
[WIFI] WIFI_MODE - STA: 'ssid':'password'
[WIFI] Connected to AP 'ssid' auth(3):'AUTH_WPA2_PSK' chan:2 rssi:-41
[WIFI] Local ip_addr : 192.168.0.102
[LOG] Start NetLogServer task (port=8008)| FreeMem 239428
[LOG] Wait new log_client... | FreeMem 239064
[NTP] Start sntp_task | FreeMem 236652
[NTP] Getting time from SNTP server '2.ru.pool.ntp.org'.
[UART] Start serial_task | FreeMem 231868
[TLS] TLS server task starting...(port=4545) | FreeMem=222964
[TLS] Wait new connection...
[NTP] The current date/time is: Fri Jan 10 12:22:26 2020 EET-2
[NTP] Stop sntp_task | FreeMem 196972
10.01 12:22:30 [LOG] New log_client 192.168.0.101:52144 (soc=56) online | FreeMem 198960
10.01 12:22:40 [TLS] New client 192.168.0.101:41564 online (sock=57)
10.01 12:22:42 [TLS] MD5 hash=C899DA77A3CE2061EB7948EE18EA76F2
10.01 12:22:42 [TLS] {"ts":1578651762}
10.01 12:22:49 [TLS] Recv. data (45 bytes) from client:{"auth":"C899DA77A3CE2061EB7948EE18EA76F2"}
10.01 12:22:49 [TLS] For client 192.168.0.101 access granted !
10.01 12:23:05 [TLS] Recv. data (20 bytes) from client:{"cmd":"GET_STAT"}
10.01 12:23:05 [UART] [1] Send(10) : GET_STAT

10.01 12:23:05 [UART] [1] Recv(13) : 'GET_STAT : 00'
10.01 12:23:05 [TLS] {"DevID":"C405EF90","Time":1578651785,"FreeMem":198280,"cli":"192.168.0.101","Vcc":3.276,"Temp":36.00,"Answer":"GET_STAT : 00"}
10.01 12:23:16 [TLS] Recv. data (20 bytes) from client:{"cmd":"GET_STAT"}
10.01 12:23:16 [UART] [2] Send(10) : GET_STAT

10.01 12:23:16 [UART] [2] Recv(13) : 'GET_STAT : 00'
10.01 12:23:16 [TLS] {"DevID":"C405EF90","Time":1578651796,"FreeMem":198076,"cli":"192.168.0.101","Vcc":3.276,"Temp":36.00,"Answer":"GET_STAT : 00"}
10.01 12:23:20 [TLS] Client closed connection (0)
10.01 12:23:20 [TLS] Wait new connection...
10.01 12:23:25 [LOG] Socket 56 error 0 'Success'
10.01 12:23:25 [LOG] Closed connection. Wait new tcp client... | FreeMem 197488

```
