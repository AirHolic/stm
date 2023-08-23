#include "demo.h"
#include "./atk_usart_net/atk_net.h"
#include "main.h"
#include "usart.h"
#include <rtthread.h>
#include <stdlib.h>

#define DEMO_TCPSERVER_IP "192.168.10.201"
#define DEMO_TCPSERVER_PORT "8080"



/**
 * @brief       显示IP地址
 * @param       无
 * @retval      无
 */
static void demo_show_ip(char *buf)
{
    rt_kprintf("IP: %s\r\n", buf);
}

/**
 * @brief       按键0功能，发送数据至TCP服务器
 * @param       无
 * @retval      无
 */
static void demo_key0_fun(void)
{
    atk_ide01_uart_printf("From ATK-IDE01.\r\n");
}

/**
 * @brief       转发接收自TCP服务器的数据至串口调试助手
 * @param       无
 * @retval      无
 */
static void demo_upload_data(void)
{
    rt_uint8_t *buf;

    buf = atk_ide01_uart_rx_get_frame();
    if (buf != NULL)
    {
        rt_kprintf("%s", buf);
        atk_ide01_uart_rx_restart();
    }
}

void demo_init(void)
{
    rt_uint8_t ret;
    char mac[18];
    char ip[16];

    /* 初始化ATK-IDE01模块 */
    ret = atk_ide01_init(115200);
    rt_kprintf("l_56_ret = %d\n",ret);
    /* 配置模块进入AT指令模式 */
    ret += atk_ide01_search_mac(mac);
    ret += atk_ide01_enter_at_mode(mac);
    rt_kprintf("l_60_ret = %d\n",ret);
    /* 打开指令回显，不然有些数据可能获取不到 */
    ret += atk_ide01_set_echo(ATK_IDE01_ECHO_ON);
    rt_kprintf("l_63_ret = %d\n",ret);
    /* 配置模块名称及串口相关配置 */
    ret += atk_ide01_set_moduname("ATK-IDE01");
    ret += atk_ide01_set_uart(ATK_IDE01_UART_BAUDRATE_115200, ATK_IDE01_UART_STOP_1, ATK_IDE01_UART_DATA_8, ATK_IDE01_UART_PARITY_NONE);
    ret += atk_ide01_set_uartpacklen(50);
    ret += atk_ide01_set_uartpacktime(50);
    rt_kprintf("l_69_ret = %d\n",ret);
    /* 配置模块工作模式及网络相关配置 */
    ret += atk_ide01_set_ethmod(ATK_IDE01_ETHMOD_TCP_CLIENT);
    ret += atk_ide01_set_dhcp(ATK_IDE01_DHCP_OFF);
    ret += atk_ide01_set_dns("114.114.114.114");
    ret += atk_ide01_set_remoteip(DEMO_TCPSERVER_IP);
    ret += atk_ide01_set_remoteport(DEMO_TCPSERVER_PORT);
    ret += atk_ide01_set_staticip("192.168.10.58");
    rt_thread_delay(1000);
    ret += atk_ide01_get_staticip(ip);

    /* 更新模块配置 */
    ret += atk_ide01_update();

    /* 检查ATK-IDE01模块的配置是否成功 */
    if (ret != 0)
    {
        rt_kprintf("ATK-IDE01 config failed!\r\n");
        //while (1)
        //{
                        HAL_GPIO_WritePin(LED1_GPIO_Port, LED1_Pin, 0);
                        rt_thread_delay(RT_TICK_PER_SECOND); // 延时1

                        //HAL_GPIO_WritePin(LED1_GPIO_Port, LED1_Pin, 1);
                        //rt_thread_delay(RT_TICK_PER_SECOND); // 延时1
            system("led led0=1\r\n");
        //}
    }
    demo_show_ip(ip);

    /* 重新开始接收新的一帧数据 */
    atk_ide01_uart_rx_restart();
}

/**
 * @brief       例程演示入口函数
 * @param       无
 * @retval      无
 */
void demo_run(void)
{

    rt_uint8_t key;
    demo_init();
    while (1)
    {
        key = 1;

        if (key == 1)
        {
            /* 发送数据至TCP服务器 */
            demo_key0_fun();
        }

        /* 转发接收自TCP服务器的数据至串口调试助手 */
        demo_upload_data();

        rt_thread_delay(10000);
    }
}

