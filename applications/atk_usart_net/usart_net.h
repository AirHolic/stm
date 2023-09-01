#ifndef __USART_NET_H
#define __USART_NET_H

typedef struct
{
  char *device_id;//"设备ID"
  char *port_status;//"当前端口状态"
  char *error_code;//错误码,
  char *timestamp; //"时间戳"
}tx_json_t;


void usart_net_run();
void usart_net_json_tx(tx_json_t *json);

#endif
