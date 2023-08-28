#include "usart_net.h"
#include "./atk_usart_net/atk_net.h"
#include "main.h"
#include "usart.h"
#include <rtthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define usart_net_TCPSERVER_IP "192.168.10.201"
#define usart_net_TCPSERVER_PORT "8080"

/**
 * @brief       命令解析函数,并将命令转发至对应邮箱
 * @param       buf为命令字符串
 * @retval      无
 */
void usart_net_commend_parsing(rt_uint8_t *buf)
{
    char *str = (char *)buf;
    char *command = strtok(str, " ");      // 灯系统(L)，阀门系统(V)或者报错命令
    char *parameters1 = strtok(NULL, " "); // 楼层数(01-16)
    char *parameters2 = strtok(NULL, " "); // 命令参数，关闭(0)，开启(1)，状态(2)

    // 判断命令是否为空
    if (command == NULL || command[0] == '\0' ||
        parameters1 == NULL || parameters1[0] == '\0' ||
        parameters2 == NULL || parameters2[0] == '\0')
    {
        rt_kprintf("Invalid command or parameters.\n");
        return;
    }

    struct msg rx;

    rx.storey = parameters1;
    rx.parameters = parameters2;

    struct msg *cmdpm = &rx;

    // 根据命令执行相应的代码

    if (!rt_strcmp(command, "L"))
    {
        rt_kprintf("%s,%s,%s\n", command, parameters1, parameters2);
        rt_mb_send(&led_mb, (rt_ubase_t)cmdpm);
        rt_thread_delay(1000);
    }
    else if (!rt_strcmp(command, "V"))
    {
        rt_kprintf("%s,%s,%s\n", command, parameters1, parameters2);
    }
    else if (!rt_strcmp(command, "U2E"))
    {
        rt_kprintf("与服务器断连，等待重连中...\n"); // 需通知重新发送消息
    }
    else
    {
        rt_kprintf("Unknown command.\n");
        return;
    }
}

/**
 * @brief       显示IP地址
 * @param       无
 * @retval      无
 */
static void usart_net_show_ip(char *buf)
{
    rt_kprintf("IP: %s\r\n", buf);
}

/**
 * @brief       发送数据至TCP服务器
 * @param       无
 * @retval      无
 */
static void usart_net_tx(int flag)
{
    atk_ide01_uart_printf("From STM32F407ZGT6. flag = %d\r\n", flag);
}

/**
 * @brief       转发接收自TCP服务器的数据至串口调试助手以及命令解析函数
 * @param       无
 * @retval      无
 */
static void usart_net_upload_data(void)
{
    rt_uint8_t *buf;

    buf = atk_ide01_uart_rx_get_frame();
    if (buf != NULL)
    {
        rt_kprintf("%s\n", buf);
        usart_net_commend_parsing(buf);
        atk_ide01_uart_rx_restart();
    }
}

/**
 * @brief       网口初始化配置函数，若使用上位机配置可不使用
 * @param       无
 * @retval      无
 */
void usart_net_init(void)
{
    rt_uint8_t ret;
    char mac[18];
    char ip[16];

    /* 初始化ATK-IDE01模块 */
    ret = atk_ide01_init(115200);
    rt_kprintf("l_56_ret = %d\n", ret);
    /* 配置模块进入AT指令模式 */
    ret += atk_ide01_search_mac(mac);
    ret += atk_ide01_enter_at_mode(mac);
    rt_kprintf("l_60_ret = %d\n", ret);
    /* 打开指令回显，不然有些数据可能获取不到 */
    ret += atk_ide01_set_echo(ATK_IDE01_ECHO_ON);
    rt_kprintf("l_63_ret = %d\n", ret);
    /* 配置模块名称及串口相关配置 */
    ret += atk_ide01_set_moduname("ATK-IDE01");
    ret += atk_ide01_set_uart(ATK_IDE01_UART_BAUDRATE_115200, ATK_IDE01_UART_STOP_1, ATK_IDE01_UART_DATA_8, ATK_IDE01_UART_PARITY_NONE);
    ret += atk_ide01_set_uartpacklen(50);
    ret += atk_ide01_set_uartpacktime(50);
    rt_kprintf("l_69_ret = %d\n", ret);
    /* 配置模块工作模式及网络相关配置 */
    ret += atk_ide01_set_ethmod(ATK_IDE01_ETHMOD_TCP_CLIENT);
    ret += atk_ide01_set_dhcp(ATK_IDE01_DHCP_OFF);
    ret += atk_ide01_set_dns("114.114.114.114");
    ret += atk_ide01_set_remoteip(usart_net_TCPSERVER_IP);
    ret += atk_ide01_set_remoteport(usart_net_TCPSERVER_PORT);
    ret += atk_ide01_set_staticip("192.168.10.58");
    rt_thread_delay(1000);
    ret += atk_ide01_get_staticip(ip);

    /* 更新模块配置 */
    ret += atk_ide01_update();

    /* 检查ATK-IDE01模块的配置是否成功 */
    if (ret != 0)
    {
        rt_kprintf("ATK-IDE01 config failed!\r\n");
        // while (1)
        //{
        HAL_GPIO_WritePin(LED1_GPIO_Port, LED1_Pin, 0);
        rt_thread_delay(RT_TICK_PER_SECOND); // 延时1

        // HAL_GPIO_WritePin(LED1_GPIO_Port, LED1_Pin, 1);
        // rt_thread_delay(RT_TICK_PER_SECOND); // 延时1
        system("led led0=1\r\n");
        //}
    }
    usart_net_show_ip(ip);
}

/**
 * @brief       例程演示入口函数
 * @param       无
 * @retval      无
 */
void usart_net_run(void)
{

    rt_uint8_t key;
    int flag = 1;
    // usart_net_init();
    /* 重新开始接收新的一帧数据 */
    atk_ide01_uart_rx_restart();
    while (1)
    {
        key = 1;

        if (key == 1)
        {
            /* 发送数据至TCP服务器 */
            usart_net_tx(flag);
        }

        /* 转发接收自TCP服务器的数据至串口调试助手 */
        usart_net_upload_data();
        flag++;
        rt_thread_delay(1000);
    }
}
