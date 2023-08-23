
#ifndef __ATK_NET_H
#define __ATK_NET_H

#include "usart.h"
#include "main.h"


/* 引脚定义 */
#ifndef ATK_NET

#define ATK_IDE01_TR_GPIO_PORT          GPIOF
#define ATK_IDE01_TR_GPIO_PIN           GPIO_PIN_6
#define ATK_IDE01_TR_GPIO_CLK_ENABLE()  do{ __HAL_RCC_GPIOF_CLK_ENABLE(); }while(0)
#define ATK_IDE01_DF_GPIO_PORT          GPIOC
#define ATK_IDE01_DF_GPIO_PIN           GPIO_PIN_0
#define ATK_IDE01_DF_GPIO_CLK_ENABLE()  do{ __HAL_RCC_GPIOC_CLK_ENABLE(); }while(0)


/* IO操作 */
#define ATK_IDE01_READ_TR()             HAL_GPIO_ReadPin(ATK_IDE01_TR_GPIO_PORT, ATK_IDE01_TR_GPIO_PIN)
#define ATK_IDE01_DF(x)                 do{ x ?                                                                                 \
                                            HAL_GPIO_WritePin(ATK_IDE01_DF_GPIO_PORT, ATK_IDE01_DF_GPIO_PIN, GPIO_PIN_SET) :    \
                                            HAL_GPIO_WritePin(ATK_IDE01_DF_GPIO_PORT, ATK_IDE01_DF_GPIO_PIN, GPIO_PIN_RESET);   \
                                        }while(0)
#endif

/* 等待ATK-IDE01模块AT指令超时时间 */
#define ATK_IDE01_AT_TIMEOUT            5000

/* ATK-IDE01模块网络工作模式枚举 */
typedef enum
{
    ATK_IDE01_ETHMOD_UDP_CLIENT = 0x00,     /* UDP客户端模式 */
    ATK_IDE01_ETHMOD_UDP_SERVER,            /* UDP服务器模式 */
    ATK_IDE01_ETHMOD_TCP_CLIENT,            /* TCP客户端模式 */
    ATK_IDE01_ETHMOD_TCP_SERVER,            /* TCP服务器模式 */
    ATK_IDE01_ETHMOD_TCP_CLOUD,             /* TCP_CLOUD模式 */
    ATK_IDE01_ETHMOD_UDP_MULTICAST,         /* UDP组播模式 */
} atk_ide01_ethmod_t;

/* ATK-IDE01模块指令回显枚举 */
typedef enum
{
    ATK_IDE01_ECHO_ON = 0x00,               /* 使能指令回显 */
    ATK_IDE01_ECHO_OFF,                     /* 关闭指令回显 */
} atk_ide01_echo_t;

/* ATK-IDE01模块DHCP状态枚举 */
typedef enum
{
    ATK_IDE01_DHCP_ON = 0x00,               /* 使能DHCP功能 */
    ATK_IDE01_DHCP_OFF,                     /* 关闭DHCP功能 */
} atk_ide01_dhcp_t;

/* ATK-IDE01模块串口波特率枚举 */
typedef enum
{
    ATK_IDE01_UART_BAUDRATE_2400 = 0x00,    /* 2400bps */
    ATK_IDE01_UART_BAUDRATE_4800,           /* 4800bps */
    ATK_IDE01_UART_BAUDRATE_9600,           /* 9600bps */
    ATK_IDE01_UART_BAUDRATE_14400,          /* 14400bps */
    ATK_IDE01_UART_BAUDRATE_19200,          /* 19200bps */
    ATK_IDE01_UART_BAUDRATE_38400,          /* 38400bps */
    ATK_IDE01_UART_BAUDRATE_43000,          /* 43000bps */
    ATK_IDE01_UART_BAUDRATE_57600,          /* 57600bps */
    ATK_IDE01_UART_BAUDRATE_76800,          /* 76800bps */
    ATK_IDE01_UART_BAUDRATE_115200,         /* 115200bps */
    ATK_IDE01_UART_BAUDRATE_128000,         /* 128000bps */
    ATK_IDE01_UART_BAUDRATE_230400,         /* 230400bps */
    ATK_IDE01_UART_BAUDRATE_256000,         /* 256000bps */
    ATK_IDE01_UART_BAUDRATE_460800,         /* 460800bps */
    ATK_IDE01_UART_BAUDRATE_921600,         /* 921600bps */
} atk_ide01_uart_baudrate_t;

/* ATK-IDE01模块串口停止位枚举 */
typedef enum
{
    ATK_IDE01_UART_STOP_1 = 0x00,           /* 1位停止位 */
    ATK_IDE01_UART_STOP_2,                  /* 2位停止位 */
} atk_ide01_uart_stop_t;

/* ATK-IDE01模块串口数据位枚举 */
typedef enum
{
    ATK_IDE01_UART_DATA_8 = 0x00,           /* 8位数据位 */
} atk_ide01_uart_data_t;

/* ATK-IDE01模块串口校验位枚举 */
typedef enum
{
    ATK_IDE01_UART_PARITY_NONE = 0x00,      /* 无校验 */
    ATK_IDE01_UART_PARITY_EVEN,             /* 偶校验 */
    ATK_IDE01_UART_PARITY_ODD,              /* 奇校验 */
} atk_ide01_uart_parity_t;

/* ATK-IDE01模块原子云启用状态枚举 */
typedef enum
{
    ATK_IDE01_CLOUDLINK_ON = 0x00,          /* 使能原子云 */
    ATK_IDE01_CLOUDLINK_OFF,                /* 关闭原子云 */
} atk_ide01_cloudlink_t;

/* ATK-IDE01模块当前网络模式的连接状态 */
typedef enum
{
    ATK_IDE01_NETLKSTA_ON = 0x00,           /* 当前网络模式的网络已连接成功 */
    ATK_IDE01_NETLKSTA_OFF,                 /* 当前网络模式的网络未连接成功 */
} atk_ide01_netlksta_t;

/* 错误代码 */
#define ATK_IDE01_EOK                   0   /* 没有错误 */
#define ATK_IDE01_ERROR                 1   /* 错误 */
#define ATK_IDE01_ETIMEOUT              2   /* 超时错误 */
#define ATK_IDE01_EINVAL                3   /* 参数错误 */

/* 操作函数 */
rt_uint8_t atk_ide01_init(rt_uint32_t baudrate);                                                                                                                  /* ATK-IDE01初始化 */
rt_uint8_t atk_ide01_send_at_cmd(char *cmd, char *ack, rt_uint32_t timeout);                                                                                      /* ATK-IDE01发送AT指令 */
rt_uint8_t atk_ide01_search_mac(char *mac);                                                                                                                    /* 搜索ATK-IDE01模块的MAC地址 */
rt_uint8_t atk_ide01_enter_at_mode(char *mac);                                                                                                                 /* 配置ATK-IDE01模块进入AT指令模式 */
rt_uint8_t atk_ide01_exit_at_mode(char *mac);                                                                                                                  /* 配置ATK-IDE01模块退出AT指令模式 */
rt_uint8_t atk_ide01_get_netmask(char *netmask);                                                                                                               /* 获取ATK-IDE01模块的子网掩码 */
rt_uint8_t atk_ide01_set_netmask(char *netmask);                                                                                                               /* 设置ATK-IDE01模块的子网掩码 */
rt_uint8_t atk_ide01_get_mac(char *mac);                                                                                                                       /* 获取ATK-IDE01模块的MAC地址 */
rt_uint8_t atk_ide01_set_mac(char *mac);                                                                                                                       /* 设置ATK-IDE01模块的MAC地址 */
rt_uint8_t atk_ide01_get_gateway(char *gateway);                                                                                                               /* 获取ATK-IDE01模块的网关地址 */
rt_uint8_t atk_ide01_set_gateway(char *gateway);                                                                                                               /* 设置ATK-IDE01模块的网关地址 */
rt_uint8_t atk_ide01_get_ethmod(atk_ide01_ethmod_t *ethmod);                                                                                                   /* 获取ATK-IDE01模块的网络工作模式 */
rt_uint8_t atk_ide01_set_ethmod(atk_ide01_ethmod_t ethmod);                                                                                                    /* 设置ATK-IDE01模块的网络模式 */
rt_uint8_t atk_ide01_get_dns(char *dns);                                                                                                                       /* 获取ATK-IDE01模块的DNS服务器地址 */
rt_uint8_t atk_ide01_set_dns(char *dns);                                                                                                                       /* 设置ATK-IDE01模块的DNS服务器地址 */
rt_uint8_t atk_ide01_get_localport(char *localport);                                                                                                           /* 获取ATK-IDE01模块的本地开放端口 */
rt_uint8_t atk_ide01_set_localport(char *localport);                                                                                                           /* 设置ATK-IDE01模块的本地开放端口 */
rt_uint8_t atk_ide01_get_remoteport(char *remoteport);                                                                                                         /* 获取ATK-IDE01模块要连接的远程端口 */
rt_uint8_t atk_ide01_set_remoteport(char *remoteport);                                                                                                         /* 设置ATK-IDE01模块要连接的远程端口 */
rt_uint8_t atk_ide01_get_remoteip(char *remoteip);                                                                                                             /* 获取ATK-IDE01模块要连接的远程IP地址 */
rt_uint8_t atk_ide01_set_remoteip(char *remoteip);                                                                                                             /* 设置ATK-IDE01模块要连接的远程IP地址 */
rt_uint8_t atk_ide01_get_staticip(char *staticip);                                                                                                             /* 获取ATK-IDE01模块的静态IP地址 */
rt_uint8_t atk_ide01_set_staticip(char *staticip);                                                                                                             /* 设置ATK-IDE01模块的静态IP地址 */
rt_uint8_t atk_ide01_get_echo(atk_ide01_echo_t *echo);                                                                                                         /* 获取ATK-IDE01模块的指令回显状态 */
rt_uint8_t atk_ide01_set_echo(atk_ide01_echo_t echo);                                                                                                          /* 设置ATK-IDE01模块的指令回显状态 */
rt_uint8_t atk_ide01_get_dhcp(atk_ide01_dhcp_t *dhcp);                                                                                                         /* 获取ATK-IDE01模块的DHCP状态 */
rt_uint8_t atk_ide01_set_dhcp(atk_ide01_dhcp_t dhcp);                                                                                                          /* 设置ATK-IDE01模块的DHCP状态 */
rt_uint8_t atk_ide01_reset(void);                                                                                                                              /* 复位ATK-IDE01模块 */
rt_uint8_t atk_ide01_update(void);                                                                                                                             /* 更新ATK-IDE01模块配置并保存 */
rt_uint8_t atk_ide01_get_version(char *version);                                                                                                               /* 获取ATK-IDE01模块的软硬件版本 */
rt_uint8_t atk_ide01_get_moduname_len(rt_uint16_t *len);                                                                                                          /* 获取ATK-IDE01模块名称的长度 */
rt_uint8_t atk_ide01_get_moduname(char *moduname);                                                                                                             /* 获取ATK-IDE01模块的名称 */
rt_uint8_t atk_ide01_set_moduname(char *moduname);                                                                                                             /* 设置ATK-IDE01模块的名称 */
rt_uint8_t atk_ide01_get_xcomport(char *xcomport);                                                                                                             /* 获取ATK-IDE01模块的上位机配置端口 */
rt_uint8_t atk_ide01_set_xcomport(char *xcomport);                                                                                                             /* 设置ATK-IDE01模块的上位机配置端口 */
rt_uint8_t atk_ide01_get_uart(atk_ide01_uart_baudrate_t *baudrate, atk_ide01_uart_stop_t *stop, atk_ide01_uart_data_t *data, atk_ide01_uart_parity_t *parity); /* 获取ATK-IDE01模块的串口配置参数 */
rt_uint8_t atk_ide01_set_uart(atk_ide01_uart_baudrate_t baudrate, atk_ide01_uart_stop_t stop, atk_ide01_uart_data_t data, atk_ide01_uart_parity_t parity);     /* 设置ATK-IDE01模块的串口配置参数 */
rt_uint8_t atk_ide01_get_uartpacktime(rt_uint16_t *uartpacktime);                                                                                                 /* 获取ATK-IDE01模块的串口打包时间 */
rt_uint8_t atk_ide01_set_uartpacktime(rt_uint16_t uartpacktime);                                                                                                  /* 设置ATK-IDE01模块的串口打包时间 */
rt_uint8_t atk_ide01_get_uartpacklen(rt_uint16_t *uartpacklen);                                                                                                   /* 获取ATK-IDE01模块的串口打包长度 */
rt_uint8_t atk_ide01_set_uartpacklen(rt_uint16_t uartpacklen);                                                                                                    /* 设置ATK-IDE01模块的串口打包长度 */
rt_uint8_t atk_ide01_get_cloudlinken(atk_ide01_cloudlink_t *cloudlink);                                                                                        /* 获取ATK-IDE01模块的原子云启用状态 */
rt_uint8_t atk_ide01_set_cloudlinken(atk_ide01_cloudlink_t cloudlink);                                                                                         /* 设置ATK-IDE01模块的原子云启用状态 */
rt_uint8_t atk_ide01_get_cloudport(char *cloudport);                                                                                                           /* 获取ATK-IDE01模块指定连接域名的连接端口 */
rt_uint8_t atk_ide01_set_cloudport(char *cloudport);                                                                                                           /* 设置ATK-IDE01模块指定连接域名的连接端口 */
rt_uint8_t atk_ide01_get_clouddomain_len(rt_uint16_t *len);                                                                                                       /* 获取ATK-IDE01模块指定连接云的域名或IP地址的长度 */
rt_uint8_t atk_ide01_get_clouddomain(char *clouddomain);                                                                                                       /* 获取ATK-IDE01模块指定连接云的域名或IP地址 */
rt_uint8_t atk_ide01_set_clouddomain(char *clouddomain);                                                                                                       /* 设置ATK-IDE01模块指定连接云的域名或IP地址 */
rt_uint8_t atk_ide01_get_atklinkpwd(char *atklinkpwd);                                                                                                         /* 获取ATK-IDE01模块原子云透传设备密码 */
rt_uint8_t atk_ide01_set_atklinkpwd(char *atklinkpwd);                                                                                                         /* 设置ATK-IDE01模块原子云透传设备密码 */
rt_uint8_t atk_ide01_get_atkyundevid(char *atkyundevid);                                                                                                       /* 获取ATK-IDE01模块原子云透传设备ID */
rt_uint8_t atk_ide01_set_atkyundevid(char *atkyundevid);                                                                                                       /* 设置ATK-IDE01模块原子云透传设备ID */
rt_uint8_t atk_ide01_get_netlksta(atk_ide01_netlksta_t *netlksta);                                                                                             /* 获取ATK-IDE01模块当前网络模式的连接状态 */
rt_uint8_t atk_ide01_ping(char *domain);                                                                                                                       /* 获取ATK-IDE01模块与指定的域名或IP是否连通 */
rt_uint8_t atk_ide01_get_linknum(rt_uint16_t *linknum);                                                                                                           /* 获取ATK-IDE01模块已建立的连接数 */
rt_uint8_t atk_ide01_get_remotelinkinfo(rt_uint8_t num, char *remoteip, char *remoteport);                                                                        /* 获取ATK-IDE01模块连接的远程IP和端口 */
rt_uint8_t atk_ide01_get_locallinkinfo(rt_uint8_t num, char *localip, char *localport);                                                                           /* 获取ATK-IDE01模块连接的本地IP和端口 */
rt_uint8_t atk_ide01_get_multcastip(char *multcastip);                                                                                                         /* 获取ATK-IDE01模块的UDP组播IP地址 */
rt_uint8_t atk_ide01_set_multcastip(char *multcastip);                                                                                                         /* 设置ATK-IDE01模块的UDP组播IP地址 */
rt_uint8_t atk_ide01_get_multcastport(char *multcastport);                                                                                                     /* 获取ATK-IDE01模块的UDP组播端口 */
rt_uint8_t atk_ide01_set_multcastport(char *multcastport);                                                                                                     /* 设置ATK-IDE01模块的UDP组播端口 */

#endif
