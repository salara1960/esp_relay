#ifndef __TLS_SRV_H__
#define __TLS_SRV_H__

#include "hdr.h"

#ifdef SET_TLS_SRV

    #define SET_MD5

    #ifndef SET_MD5
        #include "esp32/sha.h"
        #define SET_SHA1
        #undef SET_SHA2_256
        #undef SET_SHA2_384
        #undef SET_SHA2_512
    #endif

    #include "mbedtls/config.h"
    #include "mbedtls/net.h"
    #include "mbedtls/debug.h"
    #include "mbedtls/ssl.h"
    #include "mbedtls/entropy.h"
    #include "mbedtls/ctr_drbg.h"
    #include "mbedtls/error.h"
    #include "mbedtls/certs.h"
    #include "mbedtls/md5.h"

    #define def_idle_count    60
    #define timeout_auth   30000
    #define timeout_def     2000
    #define timeout_max    60000


    #pragma pack(push,1)
    typedef struct {
        unsigned first : 1;
        unsigned first_send : 1;
        unsigned none : 6;
    } s_tls_flags;
    #pragma pack(pop)

    #pragma pack(push,1)
    typedef struct {
        unsigned char mip[4];
    } s_tls_client_ip;
    #pragma pack(pop)


    extern char tls_cli_ip_addr[32];

    extern const char *TAGTLS;
    extern uint8_t tls_start;
    extern uint8_t tls_hangup;
    extern time_t mk_hash(char *out, const char *part);
    extern void tls_task(void *arg);
#endif

#endif
