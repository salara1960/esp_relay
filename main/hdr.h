#undef UDP_SEND_BCAST

#define SET_NET_LOG
#define SET_SERIAL
#define SET_SSD1306
#define SET_SNTP
#define SET_TLS_SRV
#define SET_ERROR_PRINT

#ifdef SET_TLS_SRV
    #undef SET_TIMEOUT60
#endif

#ifdef SET_SERIAL
    #undef SET_SELECT_MODE
#endif