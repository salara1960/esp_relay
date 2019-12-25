#include "hdr.h"

#ifdef SET_TLS_SRV

#include "main.h"

//******************************************************************************************

const char *instr = "alarm";
const char *TAGTLS = "TLS";
uint8_t tls_start = 0;
uint8_t tls_hangup = 0;
char tls_cli_ip_addr[32] = {0};


//******************************************************************************************
time_t mk_hash(char *out, const char *part)
{
#ifdef SET_MD5
    #define hash_len 16
    const char *mark = "MD5";
#else
    #if defined(SET_SHA2_256)
        #define hash_len 32
        const char *mark = "SHA2_256";
        esp_sha_type sha_type = SHA2_256;
    #elif defined(SET_SHA2_384)
        #define hash_len 48
        const char *mark = "SHA2_384";
        esp_sha_type sha_type = SHA2_384;
    #elif defined(SET_SHA2_512)
        #define hash_len 64
        const char *mark = "SHA2_512";
        esp_sha_type sha_type = SHA2_512;
    #else
        #define hash_len 20
        const char *mark = "SHA1";
        esp_sha_type sha_type = SHA1;
    #endif
#endif
unsigned char hash[hash_len] = {0};
time_t ret = time(NULL);

    char *ts = (char *)calloc(1, (strlen(part) << 1) + 32);
    if (ts) {
        sprintf(ts,"%s_%u_%s", part, (uint32_t)ret, part);
#ifdef SET_MD5
        mbedtls_md5((unsigned char *)ts, strlen(ts), hash);
#else
        esp_sha(sha_type, (const unsigned char *)ts, strlen(ts), hash);
#endif
        free(ts);
        for (uint8_t i = 0; i < hash_len; i++) sprintf(out+strlen(out),"%02X", hash[i]);
        char stx[128];
        sprintf(stx, "%s hash=%s\n", mark, out);
        print_msg(TAGTLS, NULL, stx, 1);
    } else ret = 0;

    return ret;
}
//******************************************************************************************
//                    TLS server (support one client ONLY)
//******************************************************************************************
void tls_task(void *arg)
{
tls_start = 1;
total_task++;

mbedtls_entropy_context  entropy;
mbedtls_ctr_drbg_context ctr_drbg;
mbedtls_ssl_context      ssl;
mbedtls_x509_crt         srvcert;
mbedtls_pk_context       pkey;
mbedtls_ssl_config       conf;
mbedtls_net_context      server_ctx, client_ctx;

int ret = 0, len = 0, err = 0, dl = 0;
uint8_t auth = 0, eot = 0;
uint32_t timeout = timeout_auth;
time_t cur_time = 0;
#ifdef SET_TIMEOUT60
    time_t wait_time = 0;
#endif
char *buf = NULL, *stx = NULL, *uk = NULL, *tbuf = NULL;
char ts[64] = {0}, hash_str[256] = {0}, str_tls_port[8] = {0};
char *auk = NULL;
#ifdef SET_SERIAL
    s_cmd evt = {NULL};
    s_cmd evt_ack = {NULL};
    char stk[128] = {0};
    uint32_t wait_ack = 0;
    const uint32_t wait_ack_def = _1s * 10;
#endif

s_tls_flags flags = {
    .first = 1,
    .first_send = 0,
    .none = 0,
};


    xEventGroupWaitBits(wifi_event_group, CONNECTED_BIT, false, true, portMAX_DELAY);

    sprintf(str_tls_port,"%u", *(uint16_t *)arg);

    ets_printf("%s[%s] TLS server task starting...(port=%s) | FreeMem=%u%s\n", START_COLOR, TAGTLS, str_tls_port, xPortGetFreeHeapSize(), STOP_COLOR);

    buf = (char *)calloc(1, BUF_SIZE);
    if (buf) stx = (char *)calloc(1, BUF_SIZE);
    if (stx) tbuf = (char *)calloc(1, BUF_SIZE);
    if (!buf || !stx || !tbuf) goto quit1;

    mbedtls_ssl_init(&ssl);
    mbedtls_x509_crt_init(&srvcert);
    mbedtls_pk_init(&pkey);
    mbedtls_ctr_drbg_init(&ctr_drbg);

    mbedtls_ssl_config_init(&conf);

    mbedtls_entropy_init(&entropy);
    if ((ret = mbedtls_ctr_drbg_seed(&ctr_drbg, mbedtls_entropy_func, &entropy, (const unsigned char *)TAGTLS, strlen(TAGTLS))) != 0) {
        ESP_LOGE(TAGTLS," failed  ! mbedtls_ctr_drbg_seed returned %d", ret);
        goto quit1;
    }

    ret = mbedtls_x509_crt_parse(&srvcert, (uint8_t *)&server_cert[0], (unsigned int)(server_cert_end - server_cert));
    if (ret < 0) {
        ESP_LOGE(TAGTLS," failed  !  mbedtls_x509_crt_parse returned -0x%x", -ret);
        goto quit1;
    }

    ret = mbedtls_pk_parse_key(&pkey, (uint8_t *)&server_key[0], (unsigned int)(server_key_end - server_key), NULL, 0);
    if (ret) {
        ESP_LOGE(TAGTLS," failed ! mbedtls_pk_parse_key returned - 0x%x", -ret);
        goto quit1;
    }

    if ((ret = mbedtls_ssl_config_defaults(&conf, MBEDTLS_SSL_IS_SERVER, MBEDTLS_SSL_TRANSPORT_STREAM, MBEDTLS_SSL_PRESET_DEFAULT)) != 0) {
        ESP_LOGE(TAGTLS," failed  ! mbedtls_ssl_config_defaults returned %d", ret);
        err = ret;
        goto quit1;
    }

    mbedtls_ssl_conf_ca_chain(&conf, srvcert.next, NULL);
    if ( ( ret = mbedtls_ssl_conf_own_cert(&conf, &srvcert, &pkey) ) != 0 ) {
        ESP_LOGE(TAGTLS," failed  ! mbedtls_ssl_conf_own_cert returned %d", ret );
        goto quit1;
    }

    mbedtls_ssl_conf_rng(&conf, mbedtls_ctr_drbg_random, &ctr_drbg);


    if ((ret = mbedtls_ssl_setup(&ssl, &conf)) != 0) {
        ESP_LOGE(TAGTLS," failed ! mbedtls_ssl_setup returned %d", ret);
        err = ret;
        goto quit1;
    }

    //---------         SET TIMEOUT FOR READ        -----------------------
    timeout = timeout_auth;//30000 msec
    mbedtls_ssl_conf_read_timeout(&conf, timeout);
    //---------------------------------------------------------------------

    tls_client_ip = 0;
    struct sockaddr_in peer_addr;
    socklen_t peer_addr_len = sizeof(struct sockaddr_in);

    while (!restart_flag) {

        memset(tls_cli_ip_addr, 0, sizeof(tls_cli_ip_addr));

        mbedtls_net_init(&server_ctx);
        mbedtls_net_init(&client_ctx);
        if (setTimeOK) {
            print_msg(TAGTLS, NULL, "Wait new connection...\n", 1);
        } else {
            ets_printf("[%s] Wait new connection...\n", TAGTLS);
        }
        // Bind
        ret = mbedtls_net_bind(&server_ctx, NULL, str_tls_port, MBEDTLS_NET_PROTO_TCP);
        if (ret) {
            ESP_LOGE(TAGTLS," failed ! mbedtls_net_bind returned %d", ret);
            err = ret;
            goto exit;
        }
        ret = mbedtls_net_set_nonblock(&server_ctx);
        if (ret) {
            ESP_LOGE(TAGTLS,"mbedtls_net_set_nonblock for server returned %d", ret);
        }

        // Accept
        ret = MBEDTLS_ERR_SSL_WANT_READ;
        while (ret == MBEDTLS_ERR_SSL_WANT_READ) {
            ret = mbedtls_net_accept(&server_ctx, &client_ctx, NULL, 0, NULL);
            if (restart_flag) {
                eot = 1;
                err = 0;
                goto exit1;
            } else vTaskDelay(250 / portTICK_RATE_MS);//500//1000
        }
        if (ret) {
            ESP_LOGE(TAGTLS," Failed to accept connection. Restarting.");
            mbedtls_net_free(&client_ctx);
            mbedtls_net_free(&server_ctx);
            continue;
        }
        getpeername(client_ctx.fd, (struct sockaddr *)&peer_addr, &peer_addr_len);
        strcpy(tls_cli_ip_addr, (char *)inet_ntoa(peer_addr.sin_addr));
        sprintf(stx, "New client %s:%u online (sock=%d)\n", tls_cli_ip_addr, htons(peer_addr.sin_port) , client_ctx.fd);
        print_msg(TAGTLS, NULL, stx, 1);

        mbedtls_ssl_set_bio(&ssl, &client_ctx, mbedtls_net_send, NULL, mbedtls_net_recv_timeout);//<- blocking I/O, f_recv == NULL, f_recv_timout != NULL

        // Handshake
        while ((ret = mbedtls_ssl_handshake(&ssl)) != 0) {
            if (ret != MBEDTLS_ERR_SSL_WANT_READ && ret != MBEDTLS_ERR_SSL_WANT_WRITE) {
                ESP_LOGE(TAGTLS," failed ! mbedtls_ssl_handshake returned -0x%x", -ret);
                err = ret;
                goto exit;
            }
        }

        tls_hangup = 0;
        flags.first = 1;
        flags.first_send = 0;
#ifdef SET_TIMEOUT60
        wait_time = time(NULL);
#endif
        // Read loop
        while (!tls_hangup && !restart_flag) {
            //
            if ((!auth) && (flags.first)) {
                memset(hash_str, 0, sizeof(hash_str));
                cur_time = mk_hash(hash_str, instr);
                len = sprintf(ts,"{\"ts\":%u}\r\n", (uint32_t)cur_time);
                while ((ret = mbedtls_ssl_write(&ssl, (unsigned char *)ts, len)) <= 0) {
                    if (ret != MBEDTLS_ERR_SSL_WANT_READ && ret != MBEDTLS_ERR_SSL_WANT_WRITE) {
                        ESP_LOGE(TAGTLS," failed ! mbedtls_ssl_write returned %d", ret);
                        err = ret;
                        break;
                    }
                }
                print_msg(TAGTLS, NULL, ts, 1);
                flags.first = 0;
            }
            //
            memset(stx, 0, BUF_SIZE);
            memset(buf, 0, BUF_SIZE);
            memset(tbuf,0, BUF_SIZE);
            len = BUF_SIZE - 1;
            int rtr = mbedtls_ssl_read(&ssl, (unsigned char *)buf, len);
            if (rtr > 0) {
#ifdef SET_TIMEOUT60
                wait_time = time(NULL);
#endif
                uk = strstr(buf + 2, "\r\n"); if (uk) *uk = '\0';
                sprintf(stx, "Recv. data (%d bytes) from client:%s\n", rtr, buf);
                print_msg(TAGTLS, NULL, stx, 1);
                eot = 0;
                //-----------------     Check auth    ------------------------------------
                if (!auth) {
                    auk = strstr(buf, "auth\":\"");
                    if (auk) {
                        if (!strncmp(auk + 7, hash_str, strlen(hash_str))) {
                            auth = 1;
                            sprintf(stx,"For client %s access granted !\n", tls_cli_ip_addr);
                            print_msg(TAGTLS, NULL, stx, 1);
#ifdef UDP_SEND_BCAST
                            if (udp_start) udp_flag = 0;
#endif
                        }
                        timeout = timeout_max;
                        mbedtls_ssl_conf_read_timeout(&conf, timeout);
                    }
                } else {
#ifdef SET_SERIAL
                    auk = strstr(buf, "cmd\":\"");
                    if (auk) {
                        evt.cmd = parser_json_str((const char *)buf);
                        if (evt.cmd) {
                            if (serial_start) {
                                if (xQueueSend(cmdq, (void *)&evt, (TickType_t)0) != pdPASS) {
                                    ESP_LOGE(TAGUS,"Error while sending to cmdq");
                                    free(evt.cmd);
                                } else {
                                    if (!wait_ack) wait_ack = get_tmr(wait_ack_def);
                                }
                            } else {
                                free(evt.cmd);
                                wait_ack = get_tmr(_100ms);
                            }
                        }
                        timeout = timeout_def;
                        mbedtls_ssl_conf_read_timeout(&conf, timeout);
                    }
#endif
                }
                //------------------------------------------------------------------------
                err = 0;
            } else if (!rtr) {
                sprintf(stx, "Client closed connection (%d)\n", rtr);
                print_msg(TAGTLS, NULL, stx, 1);
                err = 0;
                break;
            } else {// rtr < 0  -  no data from client
                err = rtr;
                if (rtr == MBEDTLS_ERR_SSL_TIMEOUT) {// -0x6800 The operation timed out
                    if (auth) {
#ifdef SET_TIMEOUT60
                        if ( ((uint32_t)time(NULL) - (uint32_t)wait_time) >= def_idle_count ) {
                            sprintf(stx, "Timeout...(no data from client %u sec). Server closed connection.\n", def_idle_count);
                            print_msg(TAGTLS, NULL, stx, 1);
                            break;
                        } else {
#endif
                            timeout = timeout_def;
                            mbedtls_ssl_conf_read_timeout(&conf, timeout);
#ifdef SET_TIMEOUT60
                        }
#endif
                    } else break;
                } else break;
            }

            // Write
            memset(tbuf, 0, BUF_SIZE);
            if (auth) {
                vTaskDelay(100 / portTICK_RATE_MS);
#ifdef SET_SERIAL
                if (serial_start) {
                    len = 0;
                    stk[0] = 0;
                    if (wait_ack) {
                        if (xQueueReceive(ackq, &evt_ack, 10/portTICK_RATE_MS) == pdTRUE) {
                            if (evt_ack.cmd != NULL) {
                                ssd1306_clear_line(6);
                                vTaskDelay(2 / portTICK_RATE_MS);
                                dl = sprintf(stk, "%s", evt_ack.cmd);
                                ssd1306_text_xy(stk, ssd1306_calcx(dl), 6);
                                dl = sprintf(stk, "%s", evt_ack.cmd);
                                free(evt_ack.cmd);
                                evt_ack.cmd = NULL;
                                //
                                len = sprintf(tbuf, "{\"DevID\":\"%08X\",\"Time\":%u,\"FreeMem\":%u,\"ipaddr\":\"%s\",\"Answer\":\"%s\"}\r\n",
                                                    cli_id, (uint32_t)time(NULL), xPortGetFreeHeapSize(), tls_cli_ip_addr, stk);
                                //
                                wait_ack = 0;
#ifdef SET_TIMEOUT60
                                wait_time = time(NULL);
#endif
                                timeout = timeout_max;
                                mbedtls_ssl_conf_read_timeout(&conf, timeout);
                            }
                        } else if (check_tmr(wait_ack)) {
                            wait_ack = 0;
                            len = sprintf(tbuf, "{\"DevID\":\"%08X\",\"Time\":%u,\"FreeMem\":%u,\"ipaddr\":\"%s\",\"Answer\":\"Timeout\"}\r\n",
                                                cli_id, (uint32_t)time(NULL), xPortGetFreeHeapSize(), tls_cli_ip_addr);
#ifdef SET_TIMEOUT60
                            wait_time = time(NULL);
#endif
                            timeout = timeout_max;
                            mbedtls_ssl_conf_read_timeout(&conf, timeout);
                        }
                    }
                } else {
                    len = sprintf(tbuf, "{\"DevID\":\"%08X\",\"Time\":%u,\"FreeMem\":%u,\"ipaddr\":\"%s\"}\r\n",
                             cli_id, (uint32_t)time(NULL), xPortGetFreeHeapSize(), tls_cli_ip_addr);
                }
#else
                len = sprintf(tbuf, "{\"DevID\":\"%08X\",\"Time\":%u,\"FreeMem\":%u,\"ipaddr\":\"%s\"}\r\n",
                             cli_id, (uint32_t)time(NULL), xPortGetFreeHeapSize(), tls_cli_ip_addr);
#endif
            } else len = sprintf(tbuf, "{\"status\":\"You are NOT auth. client, bye\"}\r\n");

            if (len) {
                while ((ret = mbedtls_ssl_write(&ssl, (unsigned char *)tbuf, len)) <= 0) {
                    if (ret != MBEDTLS_ERR_SSL_WANT_READ && ret != MBEDTLS_ERR_SSL_WANT_WRITE) {
                        ESP_LOGE(TAGTLS," failed ! mbedtls_ssl_write returned %d", ret);
                        err = ret;
                        break;
                    }
                }
                print_msg(TAGTLS, NULL, tbuf, 1);
            }
            if (!auth || eot) break;
            vTaskDelay(10 / portTICK_RATE_MS);
        }//while (!tls_hangup...)


exit1:
        auth = 0;
        tls_client_ip = 0;
        mbedtls_ssl_close_notify(&ssl);

exit:
        mbedtls_ssl_session_reset(&ssl);
        mbedtls_net_free(&client_ctx);
        mbedtls_net_free(&server_ctx);

        if (err) {
            memset(stx, 0, BUF_SIZE);
            mbedtls_strerror(err, stx, BUF_SIZE-1);
            ESP_LOGE(TAGTLS,"Last error %d  (%s)", err, stx);
            err = 0;
        }
        if (eot) break;
#ifdef SET_TIMEOUT60
        wait_time = time(NULL);
#endif
        timeout = timeout_auth;//30000 msec
        mbedtls_ssl_conf_read_timeout(&conf, timeout);

    }

quit1:

    if (tbuf) free(tbuf);
    if (stx) free(stx);
    if (buf) free(buf);

    ets_printf("%s[%s] TLS server task stop | FreeMem=%u%s\n", START_COLOR, TAGTLS, xPortGetFreeHeapSize(), STOP_COLOR);
    if (total_task) total_task--;
    tls_start = 0;
    vTaskDelete(NULL);

}
//******************************************************************************************

#endif
