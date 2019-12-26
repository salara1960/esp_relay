#include "hdr.h"

#ifdef SET_SERIAL

#include "main.h"

uint8_t serial_start = 0;
const char *cmd_name = "cmd";

const char *TAGUS = "UART";
const int unum = UART_NUMBER;
const int uspeed = UART_SPEED;
const int BSIZE = 256;


//******************************************************************************************
//******************************************************************************************
//******************************************************************************************

esp_err_t serial_init()
{

    uart_config_t uart_conf = {
        .baud_rate = uspeed,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_APB,
    };

    esp_log_level_set(TAGUS, ESP_LOG_INFO);

    esp_err_t ret = uart_driver_install(unum, BSIZE, BSIZE, 0, NULL, 0);
    if (ret != ESP_OK) return ret;

    ret = uart_param_config(unum, &uart_conf);
    if (ret != ESP_OK) return ret;

    ret = uart_set_pin(unum, GPIO_U2TXD, GPIO_U2RXD, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);//Set UART2 pins(TX, RX, RTS, CTS)
    if (ret != ESP_OK) return ret;

    ret = uart_flush(unum);
    if (ret != ESP_OK) return ret;

    ret = uart_flush_input(unum);

    return ret;
}
//-----------------------------------------------------------------------------------------
char *get_json_str(cJSON *tp)
{
    if (tp->type != cJSON_String) return NULL;

    char *st = cJSON_Print(tp);
    if (st) {
        if (*st == '"') {
            int len = strlen(st);
            memcpy(st, st + 1, len - 1);
            *(st + len - 2) = '\0';
        }
    }

    return st;
}
//-----------------------------------------------------------------------------------------
char *parser_json_str(const char *st)
{
char *ret = NULL, *val = NULL;


    cJSON *obj = cJSON_Parse(st);
    if (!obj) {
        ESP_LOGE(TAGUS, "Error make json object from string '%s'\n", st);
        return ret;
    }

    cJSON *tmp = cJSON_GetObjectItem(obj, cmd_name);
    if (tmp) {
        if (tmp->type == cJSON_String) val = get_json_str(tmp);
        if (val) {
            int dl = strlen(val);
            ret = (char *)calloc(1, dl + 1);
            if (ret) memcpy(ret, val, dl);
            free(val); val = NULL;
        }
    }

    cJSON_Delete(obj);

    return ret;
}
//-----------------------------------------------------------------------------------------
void serial_task(void *arg)
{
serial_start = 1;
total_task++;
uint8_t out = 0, wait_ack = 0;
size_t buf_len = BSIZE;
char stk[128];
char stx[BSIZE];
s_cmd evt = {NULL};
s_cmd evt_ack = {NULL};
uint32_t rxc = 0, txc = 0;
int dl = 0;
char *us = NULL;


    ets_printf("%s[%s] Start serial_task | FreeMem %u%s\n", START_COLOR, TAGUS, xPortGetFreeHeapSize(), STOP_COLOR);

    char *data = (char *)calloc(1, buf_len + 1);
    if (data) {

#ifdef SET_SELECT_MODE
        int fd = -1;
        if ((fd = open("/dev/uart/2", O_RDWR)) == -1) {
            ESP_LOGE(TAGUS, "Cannot open UART. OutOfJob !\n");
            vTaskDelay(500 / portTICK_PERIOD_MS);
            out = 1;
        }

        fd_set rfds;
        char cha;
        struct timeval tv = {
            .tv_sec  = 0,
            .tv_usec = 10000
        };
        int resa = 0;
#endif

        int uk = 0, tmp = 0, recv = 0, rdy = 0;

        while (!out) {

            if (xQueueReceive(cmdq, &evt, (TickType_t)0) == pdTRUE) {
                if (evt.cmd != NULL) {
                    ssd1306_clear_line(5);
                    vTaskDelay(2 / portTICK_PERIOD_MS);
                    dl = sprintf(stk,"%s", evt.cmd);
                    ssd1306_text_xy(stk, ssd1306_calcx(dl), 5);
                    dl = sprintf(stk, "%s\r\n", evt.cmd);
                    free(evt.cmd);
                    evt.cmd = NULL;
                    if (dl > 2) {
#ifdef SET_SELECT_MODE
                        if (write(fd, stk, dl) != dl) {
#else
                        if (uart_write_bytes(unum, stk, dl) != dl) {
#endif
                          sprintf(stx, "[%u] Error Send(%d) : %s", ++txc, dl, stk);
                        } else {
                            wait_ack = 1;
                            sprintf(stx, "[%u] Send(%d) : %s", ++txc, dl, stk);
                        }
                        print_msg(TAGUS, NULL, stx, 1);
                    }
                }
            }
#ifdef SET_SELECT_MODE
            FD_ZERO(&rfds);
            FD_SET(fd, &rfds);
            resa = select(fd + 1, &rfds, NULL, NULL, &tv);
            if (resa > 0) {
                if (FD_ISSET(fd, &rfds)) {
                    tmp = read(fd, &cha, 1);
                    if (tmp > 0) {
                        *(data + uk) = cha;
                        recv += tmp;
                        uk   += tmp;
                        if ((us = strchr(data, '\n')) != NULL) {
                            *us = '\0';
                            recv--;
                            if (!recv) {
                                memset(data, 0, buf_len);
                                uk = tmp = 0;
                            } else rdy = 1;
                        } else if (recv >= buf_len - 1) rdy = 1;
                    }
                }
            }
#else
            tmp = uart_read_bytes(unum, (uint8_t *)(data + uk), 1, TIME_READ);//5 msec
            if (tmp > 0) {
                recv += tmp;
                uk   += tmp;
                if ((us = strchr(data, '\n')) != NULL) {
                    *us = '\0';
                    recv--;
                    if (!recv) {
                        memset(data, 0, buf_len);
                        uk = tmp = 0;
                    } else rdy = 1;
                } else if (recv >= buf_len - 1) rdy = 1;
            }
#endif
            if (rdy) {
                recv = strlen(data);
                if (*(data + recv - 1) == '\r') { *(data + recv - 1) = '\0'; recv--; }
                sprintf(stx, "[%u] Recv(%d) : '%s'\n", ++rxc, recv, data);
                print_msg(TAGUS, NULL, stx, 1);
                if ((recv > 0) && wait_ack) {
                    evt_ack.cmd = (char *)calloc(1, recv + 1);
                    if (evt_ack.cmd) {
                        memcpy(evt_ack.cmd, data, recv);
                        if (xQueueSend(ackq, (void *)&evt_ack, (TickType_t)0) != pdPASS) {
                            ESP_LOGE(TAGUS,"Error while sending to ackq");
                        }
                    }
                    wait_ack = 0;
                }
                memset(data, 0, buf_len);
                uk = tmp = 0;
                recv = rdy = 0;
            }
        }//while(!out)
#ifdef SET_SELECT_MODE
        if (fd > 0) close(fd);
#endif
        free(data);
    }

    ets_printf("%s[%s] Stop serial_task | FreeMem %u%s\n", START_COLOR, TAGUS, xPortGetFreeHeapSize(), STOP_COLOR);
    serial_start = 0;
    if (total_task) total_task--;

    vTaskDelete(NULL);
}

//******************************************************************************************

#endif
