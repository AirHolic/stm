/*
 * Copyright (c) 2006-2023, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2023-08-16     RT-Thread    first version
 */

#include "main.h"
#include "./atk_usart_net/atk_net.h"
#include "./spi_flash/spi_flash.h"
#include "./atk_usart_net/usart_net.h"
#define DBG_TAG "main"
#define DBG_LVL DBG_LOG
#include <rtdbg.h>

// # define THREAD_PRIORITY 25
// # define THREAD_STACK_SIZE 512
// # define THREAD_TIMESLICE 5
// static rt_thread_t tid1 = RT_NULL;

// 要写入到 W25Q128 的字符串数组
const rt_uint8_t TEXT_Buffer[] = {"Explorer STM32F4 SPI TEST"};
#define SIZE sizeof(TEXT_Buffer)
rt_uint8_t datatemp[SIZE];
rt_uint16_t i = 0;
rt_uint32_t FLASH_SIZE;

static struct rt_thread led0_thread;      // 线程控制块
static struct rt_thread led1_thread;      // 线程控制块
static struct rt_thread usart_net_thread; // 线程控制块
static struct rt_thread mail_deal_thread;

ALIGN(RT_ALIGN_SIZE)
static rt_uint8_t rt_led0_thread_stack[256];       // 线程栈
static rt_uint8_t rt_led1_thread_stack[256];        // 线程栈
static rt_uint8_t rt_usart_net_thread_stack[1024]; // 线程栈
static rt_uint8_t rt_mail_deal_thread_stack[512];//若打开其他线程，建议将入口函数放在所有入口函数最下方

///* 线 程 1 的 入 口 函 数 */
// static void thread1_entry(void *parameter)
//{
//     rt_uint32_t count = 0;
//     while (1)
//     {
//         /* 线 程 1 采 用 低 优 先 级 运 行， 一 直 打 印 计 数 值 */
//         rt_kprintf("thread1 count: %d\n", count ++);
//         rt_thread_mdelay(500);
//     }
// }

// 线程LED0
static void led0_thread_entry(void *parameter)
{
    while(1){
        HAL_GPIO_WritePin(LED0_GPIO_Port, LED0_Pin, 0);
        rt_thread_delay(RT_TICK_PER_SECOND); // 延时

        HAL_GPIO_WritePin(LED0_GPIO_Port, LED0_Pin, 1);
        rt_thread_delay(RT_TICK_PER_SECOND); // 延时
    }
}

// 线程LED1
static void led1_thread_entry(void *parameter)
{
    while(1){
        HAL_GPIO_WritePin(LED1_GPIO_Port, LED1_Pin, 0);
        rt_thread_delay(RT_TICK_PER_SECOND); // 延时1

        HAL_GPIO_WritePin(LED1_GPIO_Port, LED1_Pin, 1);
        rt_thread_delay(RT_TICK_PER_SECOND); // 延时1
    }
}

// 线程usart_net
static void usart_net_thread_entry(void *parameter)
{
    while (1)
    {
        usart_net_run();
    }
}

void led_close(int flag)
{
    if (flag == 0)
        rt_thread_control(&led0_thread, RT_THREAD_CTRL_CLOSE, 0);
    else if (flag == 1)
        rt_thread_control(&led1_thread, RT_THREAD_CTRL_CLOSE, 0);
    else
        rt_kprintf("please check your commend!");
}

void led_open(int flag)
{
    if (flag == 0)
    {
        // 创建静态线程
        rt_thread_init(&led0_thread,                 // 线程控制块
                       "led0",                       // 线程名字，在shell里面可以看到
                       led0_thread_entry,            // 线程入口函数
                       RT_NULL,                      // 线程入口函数参数
                       &rt_led0_thread_stack[0],     // 线程栈起始地址
                       sizeof(rt_led0_thread_stack), // 线程栈大小
                       23,                           // 线程的优先级
                       20);                          // 线程时间片
        rt_thread_control(&led0_thread, RT_THREAD_CTRL_STARTUP, 0);
    }
    else if (flag == 1)
    {
        rt_thread_init(&led1_thread,                 // 线程控制块
                       "led1",                       // 线程名字，在shell里面可以看到
                       led1_thread_entry,            // 线程入口函数
                       RT_NULL,                      // 线程入口函数参数
                       &rt_led1_thread_stack[0],     // 线程栈起始地址
                       sizeof(rt_led1_thread_stack), // 线程栈大小
                       22,                           // 线程的优先级
                       20);
        rt_thread_control(&led1_thread, RT_THREAD_CTRL_STARTUP, 0);
    }
    else
        rt_kprintf("please check your commend!");
}
//线程mail_deal入口函数
 static void mail_deal_thread_entry(void *parameter)
{
     struct msg *cmdpm = NULL;

     while (1)
     {
         // rt_kprintf("thread1: try to recv a mail\n");

         /* 从邮箱中收取邮件 */
         if (rt_mb_recv(&led_mb, (rt_ubase_t *)&cmdpm, RT_WAITING_FOREVER) == RT_EOK)
         {
             rt_kprintf("the content:%s,%s\n", cmdpm->storey, cmdpm->parameters);
             if(!rt_strcmp(cmdpm->storey, "0"))
             {
                 if(!rt_strcmp(cmdpm->parameters, "0"))
                 {
                     led_close(0);
                 }
                 else if (!rt_strcmp(cmdpm->parameters, "1")) {
                     led_open(0);
                 }
                 else {
                     rt_kprintf("valid parameters\n");
                 }

             }
             else if (!rt_strcmp(cmdpm->storey, "1")) {
                 if(!rt_strcmp(cmdpm->parameters, "0"))
                 {
                     led_close(1);
                 }
                 else if (!rt_strcmp(cmdpm->parameters, "1")) {
                     led_open(1);
                 }
                 else {
                     rt_kprintf("valid parameters\n");
                 }
             }
             else{
                 rt_kprintf("valid storey\n");

             }
             /* 延时 100ms */
             rt_thread_mdelay(1000);
         }
     }
 }



long led(int argc, char **argv)
{
    if (argc < 2)
    {
        rt_kprintf("Please input'atcmd <server|client>'\n");
        return 0;
    }

    if (!rt_strcmp(argv[1], "led0=0"))
    {
        led_close(0);
        atk_ide01_uart_rx_restart();
        atk_ide01_uart_printf("led_close(0)\r\n");
    }
    else if (!rt_strcmp(argv[1], "led0=1"))
    {
        led_open(0);
        atk_ide01_uart_rx_restart();
        atk_ide01_uart_printf("led_open(0)\r\n");
    }
    else if (!rt_strcmp(argv[1], "led1=0"))
    {
        led_close(1);
        atk_ide01_uart_rx_restart();
        atk_ide01_uart_printf("led_close(1)\r\n");
    }
    else if (!rt_strcmp(argv[1], "led1=1"))
    {
        led_open(1);
        atk_ide01_uart_rx_restart();
        atk_ide01_uart_printf("led_open(1)\r\n");
    }
    else
    {
        rt_kprintf("Please input'atcmd <server|client>'\n");
        return -1;
    }
    return 0;
}
MSH_CMD_EXPORT(led, led sample
               : led<led0 | 1 = 0 | 1>);

long SPI_op(int argc, char **argv)
{
    if (argc < 2)
    {
        rt_kprintf("Please input'atcmd <server|client>'\n");
        return 0;
    }
    if (!rt_strcmp(argv[1], "spi_wr"))
    {

        LOG_D("Start Write W25Q128....");
        W25QXX_Write((uint8_t *)TEXT_Buffer, FLASH_SIZE - 100, SIZE);
        // 从倒数第 100 个地址处开始,写入 SIZE 长度的数据
        LOG_D("W25Q128 Write Finished!"); // 提示完成
    }
    else if (!rt_strcmp(argv[1], "spi_re"))
    {
        LOG_D("Start Read W25Q128.... ");
        W25QXX_Read(datatemp, FLASH_SIZE - 100, SIZE);
        // 从倒数第 100 个地址处开始,读出 SIZE 个字节
        LOG_D("The Data Readed Is: "); // 提示传送完成
        rt_kprintf("%s/n", datatemp);  // 显示读到的字符串
    }
    return 0;
}
MSH_CMD_EXPORT(SPI_op, SPI_op sample
               : SPI_op + "spi_wr or spi_re");

long usart_net_open()
{
    // 创建静态线程
    rt_thread_init(&usart_net_thread,                 // 线程控制块
                   "usart_net",                       // 线程名字，在shell里面可以看到
                   usart_net_thread_entry,            // 线程入口函数
                   RT_NULL,                           // 线程入口函数参数
                   &rt_usart_net_thread_stack[0],     // 线程栈起始地址
                   sizeof(rt_usart_net_thread_stack), // 线程栈大小
                   12,                                 // 线程的优先级
                   20);                               // 线程时间片
    rt_thread_control(&usart_net_thread, RT_THREAD_CTRL_STARTUP, 0);
    return 0;
}
MSH_CMD_EXPORT(usart_net_open, usart_net_open sample);

long mail_deal_open()
{
    // 创建静态线程
    rt_thread_init(&mail_deal_thread,                 // 线程控制块
                   "mail_deal",                       // 线程名字，在shell里面可以看到
                   mail_deal_thread_entry,            // 线程入口函数
                   RT_NULL,                           // 线程入口函数参数
                   &rt_mail_deal_thread_stack[0],     // 线程栈起始地址
                   sizeof(rt_mail_deal_thread_stack), // 线程栈大小
                   13,                                 // 线程的优先级
                   20);                               // 线程时间片
    rt_thread_control(&mail_deal_thread, RT_THREAD_CTRL_STARTUP, 0);
    return 0;
}
MSH_CMD_EXPORT(mail_deal_open, mail_deal_open sample);

int main(void)
{
    HAL_Init();
    MX_GPIO_Init();
    W25QXX_Init(); // W25QXX 初始化
    MX_DMA_Init();
    MX_USART3_UART_Init();

    if (W25QXX_ReadID() != W25Q128) // 检测不到 W25Q128
    {
        LOG_D("W25Q128 Check Failed!");
        rt_thread_delay(500);
        LOG_D("Please Check! ");
        rt_thread_delay(500);
    }

    rt_err_t result;

    /* 初始化一个 mailbox */
    result = rt_mb_init(&led_mb,
                        "mbt",               /* 名称是 mbt */
                        &led_mb_pool[0],         /* 邮箱用到的内存池是 mb_pool */
                        sizeof(led_mb_pool) / 4, /* 邮箱中的邮件数目，因为一封邮件占 4 字节 */
                        RT_IPC_FLAG_FIFO);   /* 采用 FIFO 方式进行线程等待 */
    if (result != RT_EOK)
    {
        rt_kprintf("init mailbox failed.\n");
        return -1;
    }
    //    /* 动态创 建 线 程 1， 名 称 是 thread1， 入 口 是 thread1_entry*/
    //    tid1 = rt_thread_create("thread1",
    //    thread1_entry, RT_NULL,
    //    THREAD_STACK_SIZE,
    //    THREAD_PRIORITY, THREAD_TIMESLICE);
    //    /* 如 果 获 得 线 程 控 制 块， 启 动 这 个 线 程 */
    //    if (tid1 != RT_NULL)
    //        rt_thread_startup(tid1);
    //    return 0;

    //    int count = 1;
    //
    //    while (count++)
    //    {
    //        LOG_D("Hello RT-Thread!");
    //        rt_thread_mdelay(1000);
    //    }
    //
    //    return RT_EOK;
}
