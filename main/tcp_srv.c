#include "hdr.h"

#ifdef SET_NET_LOG

#include "main.h"


//------------------------------------------------------------------------------------------------------------

int tcpCli = -1;
static const char *TAGLOG = "LOG";

//------------------------------------------------------------------------------------------------------------
int get_socket_error_code(int socket)
{
int res;
socklen_t optlen = sizeof(int);

    if (getsockopt(socket, SOL_SOCKET, SO_ERROR, &res, &optlen) == -1) {
        ESP_LOGE(TAGLOG, "getsockopt for socket %d failed", socket);
        res = -1;
    }
    return res;
}
//------------------------------------------------------------------------------------------------------------
void show_socket_error_reason(int socket)
{
int err = get_socket_error_code(socket);

    char *stx = (char *)calloc(1, strlen(strerror(err)) + 64);
    if (stx) {
        sprintf(stx, "Socket %d error %d '%s'\n", socket, err, strerror(err));
        print_msg(TAGLOG, NULL, stx, 1);
        free(stx);
    }
}
//------------------------------------------------------------------------------------------------------------
void net_log_close(int *cli)
{
    if (*cli < 0) return;

    shutdown(*cli, 0);
    close(*cli);
    *cli = -1;
}
//------------------------------------------------------------------------------------------------------------
int create_tcp_server(u16_t prt)
{
int soc = -1, err = 0;

    soc = socket(AF_INET, SOCK_STREAM, 0);
    if (soc >= 0) {
        struct sockaddr_in server_addr;
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(prt);
        server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
        if (!bind(soc, (struct sockaddr*)&server_addr, sizeof(server_addr))) {
            if (listen(soc, 1)) err = 1;
        } else err = 1;

        if (err) {
            show_socket_error_reason(soc);
            net_log_close(&soc);
        }
    }

    return soc;
}
//------------------------------------------------------------------------------------------------------------
int sendMsg(int *cli)
{
s_net_msg evt;
int ret = -1, s = *cli;

    if (s < 0) return ret;

    if (xQueueReceive(msgq, &evt, (TickType_t)0) == pdTRUE) {//data for printing present !
        if (evt.msg != NULL) {
            int len = strlen(evt.msg);
            if (len) {
                ret = send(s, evt.msg, len, 0);
                if (ret < 0) {
                    show_socket_error_reason(s);
                    net_log_close(&s);
                    *cli = s;
                }
            }
            free(evt.msg);
        }
    } else {// no data for printing
        char sym;
        if (!recv(s, &sym, 1, O_NONBLOCK)) {//client offline check
            ret = -1;
            show_socket_error_reason(s);
            net_log_close(&s);
            *cli = s;
        } else ret = 0;
    }

    return ret;
}
//------------------------------------------------------------------------------------------------------------
int putMsg(char *st)
{
int ret = -1;

    if (!st) return ret;

    int len = strlen(st); if (!len) return ret;

    s_net_msg evt;
    evt.msg = (char *)calloc(1, len + 1);
    if (evt.msg) {
        memcpy(evt.msg, st, len);
        if (xQueueSend(msgq, (void *)&evt, (TickType_t)0) != pdPASS) {
            ESP_LOGE(TAGUS, "Error while sending to msg queue");
            free(evt.msg);
        } else ret = 0;
    }

    return ret;
}
//------------------------------------------------------------------------------------------------------------
void net_log_task(void *arg)
{
total_task++;

int srv = -1, res = 0;
struct sockaddr_in client_addr;
unsigned int socklen = sizeof(client_addr);
char stx[256];

    uint16_t tp = *(uint16_t *)arg;

    ets_printf("[%s] Start NetLogServer task (port=%u)| FreeMem %u\n", TAGLOG, tp, xPortGetFreeHeapSize());

    xEventGroupWaitBits(wifi_event_group, CONNECTED_BIT, false, true, portMAX_DELAY);


    srv = create_tcp_server(tp);
    if (srv >= 0) {
        fcntl(srv, F_SETFL, (fcntl(srv, F_GETFL, 0)) | O_NONBLOCK);
        ets_printf("[%s] Wait new log_client... | FreeMem %u\n", TAGLOG, xPortGetFreeHeapSize());
        while (!restart_flag) {
            tcpCli = accept(srv, (struct sockaddr*)&client_addr, &socklen);
            if (tcpCli >= 0) {
                sprintf(stx, "New log_client %s:%u (soc=%u) online | FreeMem %u\n",
                           (char *)inet_ntoa(client_addr.sin_addr),
                           htons(client_addr.sin_port),
                           tcpCli,
                           xPortGetFreeHeapSize());
                fcntl(tcpCli, F_SETFL, (fcntl(tcpCli, F_GETFL, 0)) | O_NONBLOCK);
                print_msg(TAGLOG, NULL, stx, 1);
                while (res >= 0) {
                    res = sendMsg(&tcpCli);
                    vTaskDelay(100 / portTICK_PERIOD_MS);
                }
                res = 0;
                sprintf(stx, "Closed connection. Wait new tcp client... | FreeMem %u\n", xPortGetFreeHeapSize());
                print_msg(TAGLOG, NULL, stx, 1);
            }
            vTaskDelay(1000 / portTICK_PERIOD_MS);
        }
    } else ESP_LOGE(TAGLOG, "ERROR create tcp server(%u)=%d", tp, srv);


    net_log_close(&tcpCli);
    net_log_close(&srv);

    ets_printf("[%s] NetLogServer task stop | FreeMem %u%s\n", TAGLOG, xPortGetFreeHeapSize());

    if (total_task) total_task--;

    vTaskDelete(NULL);
}
//------------------------------------------------------------------------------------------------------------

#endif