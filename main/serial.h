#ifndef __SERIAL_H__
#define __SERIAL_H__

#include "hdr.h"

#ifdef SET_SERIAL


    #define TIME_READ        (5 / portTICK_RATE_MS)


    #define GPIO_U2TXD 17 //pin17 - U2TXD
    #define GPIO_U2RXD 16 //pin16 - U2RXD
    #define UART_NUMBER UART_NUM_2
    #define UART_SPEED 115200

    #define max_cmd 4
    #define max_rel 8

    typedef struct {
        char *cmd;
    } s_cmd;

    extern uint8_t serial_start;
    extern const char *cmd[];

    extern const char *TAGUS;

    extern char *parser_json_str(const char *st);
    extern esp_err_t serial_init();
    extern void serial_task(void *arg);

#endif

#endif

