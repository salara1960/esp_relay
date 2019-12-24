# esp_relay - управление блоком из 8-ми реле 
(эмулятор реле тут https://github.com/salara1960/relay_box)

esp32(DevKitC board) + ssd1306(i2c) : uart + sntp_server + udp_server + tls_server + log_server

# Файлы пакета:

* sdkconfing     - файл конфигурации проекта

* Makefile       - make файл (файл сценария компиляции проекта)

* version        - файл версии ПО

* README.md      - файл справки

* main/          - папка исходников

* partitions.csv - файл конфигурации разделов dataflah


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


