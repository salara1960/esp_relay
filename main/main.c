#include "hdr.h"
#include "main.h"

#include "../version"

//*******************************************************************

uint8_t restart_flag = 0;

uint8_t setTimeOK = 0;

uint8_t total_task = 0;
uint8_t last_cmd = 255;

static const char *TAG = "MAIN";
static const char *TAGN = "NVS";
static const char *TAGT = "VFS";
static const char *TAGW = "WIFI";

static const char *wmode_name[] = {"NULL", "STA", "AP", "APSTA", "MAX"};
static uint8_t wmode = WIFI_MODE_STA;
static unsigned char wifi_param_ready = 0;
char work_ssid[wifi_param_len] = {0};
static char work_pass[wifi_param_len] = {0};
#ifdef SET_SNTP
    static char work_sntp[sntp_srv_len] = {0};
    char time_zone[sntp_srv_len] = {0};
#endif
EventGroupHandle_t wifi_event_group;
const int CONNECTED_BIT = BIT0;

uint8_t *macs = NULL;

static char sta_mac[18] = {0};
uint32_t cli_id = 0;
char localip[32] = {0};
esp_ip4_addr_t ip4;
ip_addr_t bca;

esp_err_t fs_ok = ESP_FAIL;;

uint32_t tls_client_ip = 0;


#ifdef UDP_SEND_BCAST
    static const char *TAGU = "UDP";
    uint8_t udp_start = 0;
    int8_t udp_flag = 0;
#endif

#ifdef SET_SNTP
    uint8_t sntp_go = 0;
#endif

uint16_t tls_port = 0;
#ifdef SET_WEB
uint16_t web_port = 0;
#endif
#ifdef SET_WS
uint16_t ws_port = 0;
#endif

static int s_retry_num = 0;

static uint32_t varta = 0;
static bool scr_ini_done = false;
//static uint8_t scr_len = 0;
//static char scr_line[32] = {0};

#ifdef SET_SERIAL
    xQueueHandle cmdq = NULL;
#endif

#ifdef SET_NET_LOG
    uint16_t net_log_port = 8008;
#endif
//***************************************************************************************************************

esp_err_t read_param(char *param_name, void *param_data, size_t len);
esp_err_t save_param(char *param_name, void *param_data, size_t len);

int save_rec_log(char *, int);
int read_rec_log(char *, uint32_t *);

//***************************************************************************************************************

//--------------------------------------------------------------------------------------
static uint32_t get_varta()
{
    return varta;
}
//--------------------------------------------------------------------------------------
static void periodic_timer_callback(void* arg)
{
    varta++; //100ms period
}
//--------------------------------------------------------------------------------------
uint32_t get_tmr(uint32_t tm)
{
    return (get_varta() + tm);
}
//--------------------------------------------------------------------------------------
int check_tmr(uint32_t tm)
{
    return (get_varta() >= tm ? 1 : 0);
}
//--------------------------------------------------------------------------------------
void print_msg(const char *tag, const char *tpc, const char *buf, uint8_t with)
{
    if (!tag || !buf) return;

    int len = strlen(tag) + strlen(buf) + 48;
    if (tpc) len += strlen(tpc);
    char *st = (char *)calloc(1, len);
    if (st) {
        if (with) {
            struct tm *ctimka;
            time_t it_ct = time(NULL);
            ctimka = localtime(&it_ct);
            sprintf(st, "%02d.%02d %02d:%02d:%02d | ",
                        ctimka->tm_mday, ctimka->tm_mon + 1,
                        ctimka->tm_hour, ctimka->tm_min, ctimka->tm_sec);
        }
        sprintf(st+strlen(st), "%s", tag);
        if (tpc != NULL) sprintf(st+strlen(st), " topic '%s'", tpc);
        sprintf(st+strlen(st), " : %s", buf);
        if (st[strlen(st) - 1] != '\n') strcat(st, "\n");
        printf(st);
#ifdef SET_NET_LOG
        if (tcpCli >= 0) putMsg(st);
#endif
        free(st);
    }
}
//------------------------------------------------------------------------------------------------------------
const char *wifi_auth_type(wifi_auth_mode_t m)
{

    switch (m) {
        case WIFI_AUTH_OPEN:// = 0
            return "AUTH_OPEN";
        case WIFI_AUTH_WEP:
            return "AUTH_WEP";
        case WIFI_AUTH_WPA_PSK:
            return "AUTH_WPA_PSK";
        case WIFI_AUTH_WPA2_PSK:
            return "AUTH_WPA2_PSK";
        case WIFI_AUTH_WPA_WPA2_PSK:
            return "AUTH_WPA_WPA2_PSK";
        case WIFI_AUTH_WPA2_ENTERPRISE:
            return "AUTH_WPA2_ENTERPRISE";
        default:
            return "AUTH_UNKNOWN";
    }

}
//------------------------------------------------------------------------------------------------------------
static void event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{

    if (event_base == WIFI_EVENT) {
        switch (event_id) {
            case WIFI_EVENT_STA_START :
                esp_wifi_connect();
            break;
            case WIFI_EVENT_STA_DISCONNECTED :
                if (s_retry_num < EXAMPLE_ESP_MAXIMUM_RETRY) {
                    esp_wifi_connect();
                    xEventGroupClearBits(wifi_event_group, CONNECTED_BIT);
                    s_retry_num++;
                    ets_printf("[%s] retry to connect to the AP\n", TAGW);
                } else ets_printf("[%s] connect to the AP fail\n", TAGW);
            break;
            case WIFI_EVENT_STA_CONNECTED :
            {
                wifi_ap_record_t wd;
                if (!esp_wifi_sta_get_ap_info(&wd)) {
                    ets_printf("[%s] Connected to AP '%s' auth(%u):'%s' chan:%u rssi:%d\n",
                               TAGW,
                               (char *)wd.ssid,
                               (uint8_t)wd.authmode, wifi_auth_type(wd.authmode),
                               wd.primary, wd.rssi);
                }
            }
            break;
        }
    } else if (event_base == IP_EVENT) {
        switch (event_id) {
            case IP_EVENT_STA_GOT_IP :
            {
                ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
                sprintf(localip, IPSTR, IP2STR(&event->ip_info.ip));
                ip4 = event->ip_info.ip;
                bca.u_addr.ip4.addr = ip4.addr | 0xff000000;
                ets_printf("[%s] Local ip_addr : %s\n", TAGW, localip);
                s_retry_num = 0;
                xEventGroupSetBits(wifi_event_group, CONNECTED_BIT);
            }
            break;
        };
    }

}
//------------------------------------------------------------------------------------------------------------
bool check_pin(uint8_t pin_num)
{
    gpio_pad_select_gpio(pin_num);
    gpio_pad_pullup(pin_num);
    if (gpio_set_direction(pin_num, GPIO_MODE_INPUT) != ESP_OK) return true;

    return (bool)(gpio_get_level(pin_num));
}
//------------------------------------------------------------------------------------------------------------
#ifdef UDP_SEND_BCAST
#define max_line 256
void udp_task(void *par)
{
udp_start = 1;
err_t err = ERR_OK;
u16_t len, port = 8004;
struct netconn *conn = NULL;
struct netbuf *buf = NULL;
void *data = NULL;
uint32_t cnt = 1;
uint32_t tmr_sec;
char line[max_line] = {0};
bool loop = true;

    total_task++;

    ets_printf("%s[%s] BROADCAST task started | FreeMem %u%s\n", START_COLOR, TAGU, xPortGetFreeHeapSize(), STOP_COLOR);

    xEventGroupWaitBits(wifi_event_group, CONNECTED_BIT, false, true, portMAX_DELAY);


    while (loop && !restart_flag) {

        if (!udp_flag) { loop = false; break; }

        conn = netconn_new(NETCONN_UDP);
        if (!conn) {
            vTaskDelay(1000 / portTICK_PERIOD_MS);
            loop = false;
        }

        tmr_sec = get_tmr(_1s);

        while (loop) {
            if (!udp_flag) { loop = false; break; }
            if (check_tmr(tmr_sec)) {
                buf = netbuf_new();
                if (buf) {
                    len = sprintf(line,"{\"DevID\":\"%08X\",\"Time\":%u,\"FreeMem\":%u,\"Ip\":\"%s\",\"To\":\"%s\",\"Pack\":%u}\r\n",
                                        cli_id,
                                        (uint32_t)time(NULL),
                                        xPortGetFreeHeapSize(),
                                        localip, ip4addr_ntoa(&bca.u_addr.ip4), cnt);
                    data = netbuf_alloc(buf, len);
                    if (data) {
                        memset((char *)data, 0, len);
                        memcpy((char *)data, line, len);
                        err = netconn_sendto(conn, buf, (const ip_addr_t *)&bca, port);
                        if (err != ERR_OK) {
                            ESP_LOGE(TAGU,"Sending '%.*s' error=%d '%s'", len, (char *)data, (int)err, lwip_strerr(err));
                        } else {
                            print_msg(TAGU, NULL, line, 1);
                            cnt++;
                        }
                    }
                    netbuf_delete(buf); buf = NULL;
                }
                tmr_sec = get_tmr(_10s);//10 sec
            } else vTaskDelay(50 / portTICK_PERIOD_MS);
            if (restart_flag) { loop = false; break; }
        }

        if (conn) { netconn_delete(conn); conn = NULL; }

    }

    udp_start = 0;
    if (total_task) total_task--;
    ets_printf("[%s] BROADCAST task stoped | FreeMem %u\n", TAGU, xPortGetFreeHeapSize());

    vTaskDelete(NULL);
}
#endif
//--------------------------------------------------------------------------------------------------
void initialize_wifi(wifi_mode_t w_mode)
{

    wifi_event_group = xEventGroupCreate();

    ESP_ERROR_CHECK(esp_netif_init());

//    tcpip_adapter_set_hostname(TCPIP_ADAPTER_IF_STA, "esp32.dev");

    ESP_ERROR_CHECK(esp_event_loop_create_default());
         if (w_mode == WIFI_MODE_STA) esp_netif_create_default_wifi_sta();
    else if (w_mode == WIFI_MODE_AP) esp_netif_create_default_wifi_ap();
    else {
        ets_printf("[%s] unknown wi-fi mode - %d | FreeMem %u\n", TAGW, w_mode, xPortGetFreeHeapSize());
        return;
    }


    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
    ESP_ERROR_CHECK(esp_wifi_set_mode(w_mode));


    switch ((uint8_t)w_mode) {
        case WIFI_MODE_STA :
        {
           ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID,    &event_handler, NULL));
           ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT,   IP_EVENT_STA_GOT_IP, &event_handler, NULL));

            wifi_config_t wifi_config;
            memset((uint8_t *)&wifi_config, 0, sizeof(wifi_config_t));
            if (wifi_param_ready) {
                memcpy(wifi_config.sta.ssid, work_ssid, strlen(work_ssid));
                memcpy(wifi_config.sta.password, work_pass, strlen(work_pass));
                ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
                ets_printf("[%s] WIFI_MODE - STA: '%s':'%s'\n",
                           TAGW, wifi_config.sta.ssid, wifi_config.sta.password);
            }
            ESP_ERROR_CHECK(esp_wifi_start());
        }
        break;
        case WIFI_MODE_AP:
        {
            ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL));

            wifi_config_t wifi_configap;
            memset((uint8_t *)&wifi_configap, 0, sizeof(wifi_config_t));
            wifi_configap.ap.ssid_len = strlen(sta_mac);
            memcpy(wifi_configap.ap.ssid, sta_mac, wifi_configap.ap.ssid_len);
            memcpy(wifi_configap.ap.password, work_pass, strlen(work_pass));
            wifi_configap.ap.channel = 6;
            wifi_configap.ap.authmode = WIFI_AUTH_WPA_WPA2_PSK;//WIFI_AUTH_WPA_WPA2_PSK
            wifi_configap.ap.ssid_hidden = 0;
            wifi_configap.ap.max_connection = 4;
            wifi_configap.ap.beacon_interval = 100;
            ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_AP, &wifi_configap));
            ESP_ERROR_CHECK(esp_wifi_start());
            ets_printf("[%s] WIFI_MODE - AP: '%s':'%s'\n", TAGW, wifi_configap.ap.ssid, work_pass);
        }
        break;
    }
}
//********************************************************************************************************************
esp_err_t read_param(char *param_name, void *param_data, size_t len)//, int8_t type)
{
nvs_handle mhd;

    esp_err_t err = nvs_open(STORAGE_NAMESPACE, NVS_READONLY, &mhd);
    if (err != ESP_OK) {
#ifdef SET_ERROR_PRINT
        ESP_LOGE(TAGN, "%s(%s): Error open '%s'", __func__, param_name, STORAGE_NAMESPACE);
#endif
    } else {//OK
        err = nvs_get_blob(mhd, param_name, param_data, &len);
        if (err != ESP_OK) {
#ifdef SET_ERROR_PRINT
            ESP_LOGE(TAGN, "%s: Error read '%s' from '%s'", __func__, param_name, STORAGE_NAMESPACE);
#endif
        }
        nvs_close(mhd);
    }

    return err;
}
//--------------------------------------------------------------------------------------------------
esp_err_t save_param(char *param_name, void *param_data, size_t len)
{
nvs_handle mhd;

    esp_err_t err = nvs_open(STORAGE_NAMESPACE, NVS_READWRITE, &mhd);
    if (err != ESP_OK) {
#ifdef SET_ERROR_PRINT
        ESP_LOGE(TAGN, "%s(%s): Error open '%s'", __func__, param_name, STORAGE_NAMESPACE);
#endif
    } else {
        err = nvs_set_blob(mhd, param_name, (uint8_t *)param_data, len);
        if (err != ESP_OK) {
#ifdef SET_ERROR_PRINT
            ESP_LOGE(TAGN, "%s: Error save '%s' with len %u to '%s'", __func__, param_name, len, STORAGE_NAMESPACE);
#endif
        } else err = nvs_commit(mhd);
        nvs_close(mhd);
    }

    return err;
}
//************************************************************************************************************
void app_main()
{

    total_task = 0;

    vTaskDelay(1500 / portTICK_RATE_MS);

    ets_printf("\nAppication version %s | SDK Version %s | FreeMem %u\n", Version, esp_get_idf_version(), xPortGetFreeHeapSize());

    esp_err_t err = nvs_flash_init();
    if (err != ESP_OK) {
        ESP_LOGE(TAGN, "1: nvs_flash_init() ERROR (0x%x) !!!", err);
        nvs_flash_erase();
        err = nvs_flash_init();
        if (err != ESP_OK) {
            ESP_LOGE(TAGN, "2: nvs_flash_init() ERROR (0x%x) !!!", err);
            while (1);
        }
    }

    vTaskDelay(1000 / portTICK_RATE_MS);

    macs = (uint8_t *)calloc(1, 6);
    if (macs) {
        esp_efuse_mac_get_default(macs);
        sprintf(sta_mac, MACSTR, MAC2STR(macs));
        memcpy(&cli_id, &macs[2], 4);
        cli_id = ntohl(cli_id);
    }

    //--------------------------------------------------------

    esp_log_level_set("wifi", ESP_LOG_WARN);

    //--------------------------------------------------------


    bool rt = check_pin(GPIO_WIFI_PIN);//pin17
    if (!rt) ets_printf("%s[%s] === CHECK_WIFI_REWRITE_PIN %d LEVEL IS %d ===%s\n",
                        GREEN_COLOR, TAGT, GPIO_WIFI_PIN, rt, STOP_COLOR);

    //--------------------------------------------------------

    const esp_timer_create_args_t periodic_timer_args = {
        .callback = &periodic_timer_callback,
        /* name is optional, but may help identify the timer when debugging */
        .name = "periodic"
    };
    esp_timer_handle_t periodic_timer;
    ESP_ERROR_CHECK(esp_timer_create(&periodic_timer_args, &periodic_timer));

    ESP_ERROR_CHECK(esp_timer_start_periodic(periodic_timer, 100000));//100000 us = 100 ms
    ets_printf("[%s] Started timer with period 1 ms, time since boot: %lld/%llu\n",
               TAGT, esp_timer_get_time(), get_varta() * 100);
    //--------------------------------------------------------


    //--------------------------------------------------------
    //    log_mutex = xSemaphoreCreateMutex();
    //--------------------------------------------------------

    //CLI_ID
    err = read_param(PARAM_CLIID_NAME, (void *)&cli_id, sizeof(uint32_t));
    if (err != ESP_OK) save_param(PARAM_CLIID_NAME, (void *)&cli_id, sizeof(uint32_t));
    ets_printf("%s[%s] DEVICE_ID='%08X'%s\n", GREEN_COLOR, TAGT, cli_id, WHITE_COLOR);

#ifdef SET_SNTP
    //SNTP + TIME_ZONE
    memset(work_sntp, 0, sntp_srv_len);
    err = read_param(PARAM_SNTP_NAME, (void *)work_sntp, sntp_srv_len);
    if (err != ESP_OK) {
        memset(work_sntp, 0, sntp_srv_len);
        memcpy(work_sntp, sntp_server_def, strlen((char *)sntp_server_def));
        save_param(PARAM_SNTP_NAME, (void *)work_sntp, sntp_srv_len);
    }
    memset(time_zone, 0, sntp_srv_len);
    err = read_param(PARAM_TZONE_NAME, (void *)time_zone, sntp_srv_len);
    if (err != ESP_OK) {
        memset(time_zone, 0, sntp_srv_len);
        memcpy(time_zone, sntp_tzone_def, strlen((char *)sntp_tzone_def));
        save_param(PARAM_TZONE_NAME, (void *)time_zone, sntp_srv_len);
    }
    ets_printf("[%s] SNTP_SERVER '%s' TIME_ZONE '%s'\n", TAGT, work_sntp, time_zone);
#endif

    //WIFI
    //   MODE
    bool rt0 = check_pin(GPIO_WMODE_PIN);//pin16
    if (!rt0) {
        ets_printf("%s[%s] === CHECK_WIFI_MODE_PIN %d LEVEL IS %d ===%s\n",
                   GREEN_COLOR, TAGT, GPIO_WMODE_PIN, rt0, STOP_COLOR);
    }


    uint8_t wtmp;
    err = read_param(PARAM_WMODE_NAME, (void *)&wtmp, sizeof(uint8_t));
    if (rt0) {//set wifi_mode from flash
        if (err == ESP_OK) {
            wmode = wtmp;
        } else {
            wmode = WIFI_MODE_AP;
            save_param(PARAM_WMODE_NAME, (void *)&wmode, sizeof(uint8_t));
        }
    } else {//set AP wifi_mode
        wmode = WIFI_MODE_AP;
        if (err == ESP_OK) {
            if (wtmp != wmode) save_param(PARAM_WMODE_NAME, (void *)&wmode, sizeof(uint8_t));
        } else {
            save_param(PARAM_WMODE_NAME, (void *)&wmode, sizeof(uint8_t));
        }
    }
/* Set STA mode !!! */
    wmode = WIFI_MODE_STA;
    save_param(PARAM_WMODE_NAME, (void *)&wmode, sizeof(uint8_t));
/**/
    ets_printf("[%s] WIFI_MODE (%d): %s\n", TAGT, wmode, wmode_name[wmode]);


#ifdef UDP_SEND_BCAST
    if (wmode == WIFI_MODE_STA) udp_flag = 1;
#endif

    //   SSID
    memset(work_ssid, 0, wifi_param_len);
    err = read_param(PARAM_SSID_NAME, (void *)work_ssid, wifi_param_len);
    if ((err != ESP_OK) || (!rt)) {
        memset(work_ssid,0,wifi_param_len);
        memcpy(work_ssid, EXAMPLE_WIFI_SSID, strlen(EXAMPLE_WIFI_SSID));
        save_param(PARAM_SSID_NAME, (void *)work_ssid, wifi_param_len);
    }
    //   KEY
    memset(work_pass, 0, wifi_param_len);
    err = read_param(PARAM_KEY_NAME, (void *)work_pass, wifi_param_len);
    if ((err != ESP_OK) || (!rt)) {
        memset(work_pass,0,wifi_param_len);
        memcpy(work_pass, EXAMPLE_WIFI_PASS, strlen(EXAMPLE_WIFI_PASS));
        save_param(PARAM_KEY_NAME, (void *)work_pass, wifi_param_len);
    }
    ets_printf("[%s] WIFI_STA_PARAM: '%s:%s'\n", TAGT, work_ssid, work_pass);

    wifi_param_ready = 1;


#ifdef SET_TLS_SRV
    err = read_param(PARAM_TLS_PORT, (void *)&tls_port, sizeof(uint16_t));
    if ((err != ESP_OK) || (!rt)) {
        tls_port = TLS_PORT_DEF;
        save_param(PARAM_TLS_PORT, (void *)&tls_port, sizeof(uint16_t));
    }
    ets_printf("[%s] TLS_PORT: %u\n", TAGT, tls_port);
#endif


//******************************************************************************************************

    initialize_wifi(wmode);
    vTaskDelay(2000 / portTICK_RATE_MS);

//******************************************************************************************************

#ifdef SET_NET_LOG
    msgq = xQueueCreate(8, sizeof(s_net_msg));//create msg queue

    if (xTaskCreatePinnedToCore(&net_log_task, "net_log_task", 4*STACK_SIZE_1K, &net_log_port, 6, NULL, 1) != pdPASS) {//7,NULL,1
        #ifdef SET_ERROR_PRINT
            ESP_LOGE(TAGS, "Create net_log_task failed | FreeMem %u", xPortGetFreeHeapSize());
        #endif
    }
    vTaskDelay(1000 / portTICK_RATE_MS);
#endif


#ifdef SET_SNTP
    if (wmode & 1) {// WIFI_MODE_STA) || WIFI_MODE_APSTA
        if (xTaskCreatePinnedToCore(&sntp_task, "sntp_task", STACK_SIZE_2K, work_sntp, 5, NULL, 0) != pdPASS) {//7,NULL,1
            #ifdef SET_ERROR_PRINT
                ESP_LOGE(TAGS, "Create sntp_task failed | FreeMem %u", xPortGetFreeHeapSize());
            #endif
        }
        vTaskDelay(1000 / portTICK_RATE_MS);
    }
#endif

#ifdef SET_SSD1306
    i2c_ssd1306_init();

    ssd1306_on(false);
    vTaskDelay(500 / portTICK_RATE_MS);

    esp_err_t ssd_ok = ssd1306_init();
    if (ssd_ok == ESP_OK) ssd1306_pattern();
    //ssd1306_invert();
    ssd1306_clear();

    scr_ini_done = true;

    char stk[128] = {0};
    struct tm *dtimka;
    int tu, tn;
    time_t dit_ct;

    uint32_t adc_tw = get_tmr(0);
#endif


#ifdef SET_SERIAL
    cmdq = xQueueCreate(8, sizeof(s_cmd));//create cmd queue
    ackq = xQueueCreate(8, sizeof(s_cmd));//create ack queue

    if (serial_init() == ESP_OK) {
        vTaskDelay(500 / portTICK_RATE_MS);
        if (xTaskCreatePinnedToCore(&serial_task, "serial_task", STACK_SIZE_2K, NULL, 7, NULL, 0) != pdPASS) {//7
            #ifdef SET_ERROR_PRINT
                ESP_LOGE(TAGUS, "Create serial_task failed | FreeMem %u", xPortGetFreeHeapSize());
            #endif
        }
    } else {
        #ifdef SET_ERROR_PRINT
            ESP_LOGE(TAGUS, "Error init serial port | FreeMem %u", xPortGetFreeHeapSize());
        #endif
    }
    vTaskDelay(500 / portTICK_RATE_MS);
#endif


#ifdef SET_TLS_SRV
    if (xTaskCreatePinnedToCore(&tls_task, "tls_task", 8*STACK_SIZE_1K, &tls_port, 8, NULL, 0) != pdPASS) {//6,NULL,1)
        #ifdef SET_ERROR_PRINT
            ESP_LOGE(TAGTLS, "Create tls_task failed | FreeMem %u", xPortGetFreeHeapSize());
        #endif
    }
    vTaskDelay(1000 / portTICK_RATE_MS);
    static uint8_t screen = 0;
#endif




    while (!restart_flag) {//main loop

#ifdef SET_SSD1306
        if (check_tmr(adc_tw)) {
            if (ssd_ok == ESP_OK) {
                dit_ct = time(NULL);
                dtimka = localtime(&dit_ct);
                sprintf(stk,"%02d.%02d %02d:%02d:%02d\n",
                                    dtimka->tm_mday, dtimka->tm_mon + 1,
                                    dtimka->tm_hour, dtimka->tm_min, dtimka->tm_sec);
                tu = strlen(localip);
                if ((tu > 0) && (tu <= 16)) {
                    tn = (16 - tu ) / 2;
                    if ((tn > 0) && (tn < 8)) sprintf(stk+strlen(stk),"%*.s",tn," ");
                    sprintf(stk+strlen(stk),"%s", localip);
                }
                strcat(stk,"\n");
#ifdef SET_TLS_SRV
                if (strlen(tls_cli_ip_addr)) {
                    sprintf(stk+strlen(stk), "TLS client adr:\n %s", tls_cli_ip_addr);
                    screen = 1;
                } else {
                    if (screen) {
                        screen = 0;
                        for (tu = 0; tu < 16; tu++) sprintf(stk+strlen(stk), " ");
                        strcat(stk,"\n");
                        for (tu = 0; tu < 16; tu++) sprintf(stk+strlen(stk), " ");
                        strcat(stk,"\n");
                    }
                }
#endif
                ssd1306_text_xy(stk, 2, 1);
            }
            adc_tw = get_tmr(_1s);
        }
#endif

        if (wmode == WIFI_MODE_STA) {// WIFI_MODE_STA || WIFI_MODE_APSTA
#ifdef SET_SNTP
            if (sntp_go) {
                if (!sntp_start) {
                    sntp_go = 0;
                    if (xTaskCreatePinnedToCore(&sntp_task, "sntp_task", STACK_SIZE_2K, work_sntp, 5, NULL, 0)  != pdPASS) {//5//7 core=1
                    #ifdef SET_ERROR_PRINT
                        ESP_LOGE(TAGS, "Create sntp_task failed | FreeMem %u", xPortGetFreeHeapSize());
                    #endif
                    }
                    vTaskDelay(500 / portTICK_RATE_MS);
                } else vTaskDelay(50 / portTICK_RATE_MS);
            }
#endif

#ifdef UDP_SEND_BCAST
            if ((!udp_start) && (udp_flag == 1)) {
                if (xTaskCreatePinnedToCore(&udp_task, "disk_task", STACK_SIZE_1K5, NULL, 5, NULL, 0) != pdPASS) {//5,NULL,1)
                #ifdef SET_ERROR_PRINT
                    ESP_LOGE(TAGU, "Create udp_task failed | FreeMem %u", xPortGetFreeHeapSize());
                #endif
                }
                vTaskDelay(500 / portTICK_RATE_MS);
            }
#endif
        }


    }

    vTaskDelay(1000 / portTICK_RATE_MS);

    uint8_t cnt = 20;
    ets_printf("%s[%s] Waiting for all task closed...%d sec.%s\n", GREEN_COLOR, TAG, cnt, STOP_COLOR);
    while (total_task) {
        cnt--; if (!cnt) break;
        vTaskDelay(1000 / portTICK_RATE_MS);
    }
    ets_printf("%s[%s] (%02d) DONE. Total unclosed task %d%s\n", GREEN_COLOR, TAG, cnt, total_task, STOP_COLOR);


    if (macs) free(macs);

    ets_printf("%s[%s] Waiting wifi stop...%s\n", GREEN_COLOR, TAG, STOP_COLOR);

    ESP_ERROR_CHECK(esp_wifi_stop());
    ESP_ERROR_CHECK(esp_wifi_deinit());

    //save_param(PARAM_WMODE_NAME, (void *)&wmode, sizeof(uint8_t));

    esp_restart();

}
