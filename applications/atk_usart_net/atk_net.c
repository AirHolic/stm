#include "atk_usart_net/atk_net.h"
#include <rtthread.h>
#include "usart.h"
#include <stdlib.h>


/**
 * @brief       ATK-IDE01硬件初始化
 * @param       无
 * @retval      无
 */
#ifndef ATK_NET

static void atk_ide01_hw_init(void)
{
    GPIO_InitTypeDef gpio_init_struct = {0};
    
    /* 使能时钟 */
    ATK_IDE01_TR_GPIO_CLK_ENABLE();
    ATK_IDE01_DF_GPIO_CLK_ENABLE();
    
    /* 初始化TR引脚 */
    gpio_init_struct.Pin    = ATK_IDE01_TR_GPIO_PIN;
    gpio_init_struct.Mode   = GPIO_MODE_INPUT;
    gpio_init_struct.Pull   = GPIO_PULLUP;
    gpio_init_struct.Speed  = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(ATK_IDE01_TR_GPIO_PORT, &gpio_init_struct);
    
    /* 初始化DF引脚 */
    gpio_init_struct.Pin    = ATK_IDE01_DF_GPIO_PIN;
    gpio_init_struct.Mode   = GPIO_MODE_OUTPUT_PP;
    gpio_init_struct.Pull   = GPIO_PULLUP;
    gpio_init_struct.Speed  = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(ATK_IDE01_DF_GPIO_PORT, &gpio_init_struct);
    
    ATK_IDE01_DF(1);
}

#endif
/**
 * @brief       以ATK-IDE01模块AT指令的格式获取src中的第param_index个参数到dts
 * @param       src        : ATK-IDE01模块的AT响应
 *              param_num  : 参数的索引
 *              param_index: 找到的参数在src中的索引
 *              param_len  : 参数的长度
 * @retval      ATK_IDE01_EOK   : 找到指定参数
 *              ATK_IDE01_EINVAL: 函数参数错误
 */
static rt_uint8_t atk_ide01_get_parameter(rt_uint8_t *src, rt_uint8_t param_num, rt_uint16_t *param_index, rt_uint16_t *param_len)
{
    rt_uint8_t src_index = 0;
    rt_uint16_t _param_index;
    rt_uint16_t _param_len;
    rt_uint8_t param_flag = 0;
    
    if (param_num == 0)
    {
        return ATK_IDE01_EINVAL;
    }
    
    while ((src[src_index] != '\0') && (src[src_index] != '='))
    {
        src_index++;
    }
    
    if (src[src_index] != '=')
    {
        return ATK_IDE01_EINVAL;
    }
    
    while (src[src_index] != '\0')
    {
        if (src[src_index] == '"')
        {
            if ((param_flag & (1 << 0)) != (1 << 0))
            {
                param_flag |= (1 << 0);
                if (param_num == 1)
                {
                    param_flag |= (1 << 1);
                    _param_index = src_index + 1;
                    if (param_len == NULL)
                    {
                        break;
                    }
                }
            }
            else
            {
                param_flag &= ~(1 << 0);
                param_num--;
                if ((param_flag & (1 << 1)) == (1 << 1))
                {
                    _param_len = src_index - _param_index;
                    break;
                }
            }
        }
        src_index++;
    }
    
    if (param_index != NULL)
    {
        *param_index = _param_index;
    }
    
    if (param_len != NULL)
    {
        *param_len = _param_len;
    }
    
    return ATK_IDE01_EOK;
}

/**
 * @brief       ATK-IDE01初始化
 * @param       baudrate: ATK-IDE01 UART通讯波特率
 * @retval      ATK_IDE01_EOK  : ATK-IDE01初始化成功
 *              ATK_IDE01_ERROR: ATK-IDE01初始化失败
 */
rt_uint8_t atk_ide01_init(rt_uint32_t baudrate)
{
    rt_uint8_t ret;
    char mac[18];
#ifndef ATK_NET

    atk_ide01_hw_init();
    atk_ide01_uart_init(115200);

#else
#endif    

    ret = atk_ide01_search_mac(mac);
    if (ret != ATK_IDE01_EOK)
    {
        rt_kprintf("atk_net line136\n");
        return ATK_IDE01_ERROR;
    }
    
    return ATK_IDE01_EOK;
}

/**
 * @brief       ATK-IDE01发送AT指令
 * @param       cmd    : 待发送的AT指令
 *              ack    : 等待的响应
 *              timeout: 等待超时时间
 * @retval      ATK_IDE01_EOK     : 函数执行成功
 *              ATK_IDE01_ETIMEOUT: 等待期望应答超时，函数执行失败
 */
rt_uint8_t atk_ide01_send_at_cmd(char *cmd, char *ack, rt_uint32_t timeout)
{
    rt_uint8_t *ret = NULL;
    
    atk_ide01_uart_rx_restart();
    atk_ide01_uart_printf("%s\r\n", cmd);
    
    if (timeout == 0)
    {
        return ATK_IDE01_EOK;
    }
    else
    {
        while (timeout > 0)
        {
            ret = atk_ide01_uart_rx_get_frame();
            if (ret != NULL)
            {
                if (ack != NULL)
                {
                    if (rt_strstr((const char *)ret, ack) != NULL)
                    {
                        return ATK_IDE01_EOK;
                    }
                    else
                    {
                        atk_ide01_uart_rx_restart();
                    }
                }
                else
                {
                    return ATK_IDE01_EOK;
                }
            }
            timeout--;
            rt_thread_delay(1);
        }
        rt_kprintf("atk_net line188\n");
        return ATK_IDE01_ETIMEOUT;
    }
}

/**
 * @brief       搜索ATK-IDE01模块的MAC地址
 * @note        搜索到的MAC地址格式为“FF-FF-FF-FF-FF-FF\0”（18字节）
 * @param       mac: ATK-IDE01模块的MAC地址（至少有18字节的空间）
 * @retval      ATK_IDE01_EOK   : 搜索ATK-IDE01模块的MAC地址成功
 *              ATK_IDE01_ERROR : 搜索ATK-IDE01模块的MAC地址失败
 *              ATK_IDE01_EINVAL: 函数参数错误
 */
rt_uint8_t atk_ide01_search_mac(char *mac)
{
    rt_uint8_t ret;
    char cmd[11];
    rt_uint8_t *buf;
    rt_uint16_t param_index;
    rt_uint16_t param_len;
    
    if (mac == NULL)
    {
        rt_kprintf("atk_net line211\n");
        return ATK_IDE01_EINVAL;
    }
    
    rt_sprintf(cmd, "AT+SEARCH?");
    rt_kprintf("cmd = %s\n", &cmd);
    ret = atk_ide01_send_at_cmd(cmd, "OK", ATK_IDE01_AT_TIMEOUT);
    if (ret != 0)
    {
        rt_kprintf("atk_net line219\n");
        return ATK_IDE01_ERROR;
    }
    
    buf = atk_ide01_uart_rx_get_frame();
    ret = atk_ide01_get_parameter(buf, 2, &param_index, &param_len);
    if (ret != ATK_IDE01_EOK)
    {
        rt_kprintf("atk_net line227\n");
        return ATK_IDE01_ERROR;
    }
    rt_memcpy(mac, &buf[param_index], param_len);
    mac[17] = '\0';
    
    return ATK_IDE01_EOK;
}

/**
 * @brief       配置ATK-IDE01模块进入AT指令模式
 * @param       mac: ATK-IDE01模块的MAC地址
 * @retval      ATK_IDE01_EOK   : 配置ATK-IDE01模块进入AT指令模式成功
 *              ATK_IDE01_ERROR : 配置ATK-IDE01模块进入AT指令模式失败
 */
rt_uint8_t atk_ide01_enter_at_mode(char *mac)
{
    rt_uint8_t ret;
    char cmd[32];
    
    rt_sprintf(cmd, "AT+SEARCH?");
    ret = atk_ide01_send_at_cmd(cmd, "OK", ATK_IDE01_AT_TIMEOUT);
    if (ret != ATK_IDE01_EOK)
    {
        rt_kprintf("atk_net line251\n");
        return ATK_IDE01_ERROR;
    }
    
    rt_sprintf(cmd, "AT+AT_INTO=\"%s\"", mac);
    ret = atk_ide01_send_at_cmd(cmd, "OK", ATK_IDE01_AT_TIMEOUT);
    if (ret != ATK_IDE01_EOK)
    {
        rt_kprintf("atk_net line259\n");
        return ATK_IDE01_ERROR;
    }
    
    return ATK_IDE01_EOK;
}

/**
 * @brief       配置ATK-IDE01模块退出AT指令模式
 * @param       mac: ATK-IDE01模块的MAC地址
 * @retval      ATK_IDE01_EOK   : 配置ATK-IDE01模块退出AT指令模式成功
 *              ATK_IDE01_ERROR : 配置ATK-IDE01模块退出AT指令模式失败
 */
rt_uint8_t atk_ide01_exit_at_mode(char *mac)
{
    rt_uint8_t ret;
    char cmd[32];
    
    rt_sprintf(cmd, "AT+AT_EXIT=\"%s\"", mac);
    ret = atk_ide01_send_at_cmd(cmd, "OK", ATK_IDE01_AT_TIMEOUT);
    if (ret != ATK_IDE01_EOK)
    {
        return ATK_IDE01_ERROR;
    }
    
    return ATK_IDE01_EOK;
}

/**
 * @brief       获取ATK-IDE01模块的子网掩码
 * @note        获取到的子网掩码格式为“255.255.255.255\0”（16字节）
 * @param       netmask: ATK-IDE01模块的子网掩码（至少有16字节的空间）
 * @retval      ATK_IDE01_EOK   : 获取ATK-IDE01模块的子网掩码成功
 *              ATK_IDE01_ERROR : 获取ATK-IDE01模块的子网掩码失败
 *              ATK_IDE01_EINVAL: 函数参数错误
 */
rt_uint8_t atk_ide01_get_netmask(char *netmask)
{
    rt_uint8_t ret;
    char cmd[12];
    rt_uint8_t *buf;
    rt_uint16_t param_index;
    rt_uint16_t param_len;
    
    if (netmask == NULL)
    {
        return ATK_IDE01_EINVAL;
    }
    
    rt_sprintf(cmd, "AT+NETMASK?");
    ret = atk_ide01_send_at_cmd(cmd, "OK", ATK_IDE01_AT_TIMEOUT);
    if (ret != ATK_IDE01_EOK)
    {
        return ATK_IDE01_ERROR;
    }
    
    buf = atk_ide01_uart_rx_get_frame();
    ret = atk_ide01_get_parameter(buf, 1, &param_index, &param_len);
    if (ret != ATK_IDE01_EOK)
    {
        return ATK_IDE01_ERROR;
    }
    rt_memcpy(netmask, &buf[param_index], param_len);
    netmask[param_len] = '\0';
    
    return ATK_IDE01_EOK;
}

/**
 * @brief       设置ATK-IDE01模块的子网掩码
 * @param       netmask: ATK-IDE01模块的子网掩码
 * @retval      ATK_IDE01_EOK   : 设置ATK-IDE01模块的子网掩码成功
 *              ATK_IDE01_ERROR : 设置ATK-IDE01模块的子网掩码失败
 */
rt_uint8_t atk_ide01_set_netmask(char *netmask)
{
    rt_uint8_t ret;
    char cmd[29];
    
    rt_sprintf(cmd, "AT+NETMASK=\"%s\"", netmask);
    ret = atk_ide01_send_at_cmd(cmd, "OK", ATK_IDE01_AT_TIMEOUT);
    if (ret != ATK_IDE01_EOK)
    {
        return ATK_IDE01_ERROR;
    }
    
    return ATK_IDE01_EOK;
}

/**
 * @brief       获取ATK-IDE01模块的MAC地址
 * @note        获取到的MAC地址格式为“FF-FF-FF-FF-FF-FF\0”（18字节）
 * @param       mac: ATK-IDE01模块的MAC地址（至少有18字节的空间）
 * @retval      ATK_IDE01_EOK   : 获取ATK-IDE01模块的MAC地址成功
 *              ATK_IDE01_ERROR : 获取ATK-IDE01模块的MAC地址失败
 *              ATK_IDE01_EINVAL: 函数参数错误
 */
rt_uint8_t atk_ide01_get_mac(char *mac)
{
    rt_uint8_t ret;
    char cmd[11];
    rt_uint8_t *buf;
    rt_uint16_t buf_len;
    rt_uint16_t buf_index = 0;
    
    if (mac == NULL)
    {
        return ATK_IDE01_EINVAL;
    }
    
    rt_sprintf(cmd, "AT+MAC_ID?");
    ret = atk_ide01_send_at_cmd(cmd, "OK", ATK_IDE01_AT_TIMEOUT);
    if (ret != ATK_IDE01_EOK)
    {
        return ATK_IDE01_ERROR;
    }
    
    buf = atk_ide01_uart_rx_get_frame();
    buf_len = atk_ide01_uart_rx_get_frame_len();
    while ((buf[buf_index] != '=') && (buf_index < buf_len))
    {
        buf_index++;
    }
    if (buf[buf_index] == '\0')
    {
        return ATK_IDE01_ERROR;
    }
    rt_memcpy(mac, &buf[buf_index + 1], 17);
    mac[17] = '\0';
    
    return ATK_IDE01_EOK;
}

/**
 * @brief       设置ATK-IDE01模块的MAC地址
 * @param       mac: ATK-IDE01模块的MAC地址
 * @retval      ATK_IDE01_EOK   : 设置ATK-IDE01模块的MAC地址成功
 *              ATK_IDE01_ERROR : 设置ATK-IDE01模块的MAC地址失败
 */
rt_uint8_t atk_ide01_set_mac(char *mac)
{
    rt_uint8_t ret;
    char cmd[30];
    
    rt_sprintf(cmd, "AT+MAC_ID=\"%s\"", mac);
    ret = atk_ide01_send_at_cmd(cmd, "OK", ATK_IDE01_AT_TIMEOUT);
    if (ret != ATK_IDE01_EOK)
    {
        return ATK_IDE01_ERROR;
    }
    
    return ATK_IDE01_EOK;
}

/**
 * @brief       获取ATK-IDE01模块的网关地址
 * @note        获取到的网关地址格式为“255.255.255.255\0”（16字节）
 * @param       gateway: ATK-IDE01模块的网关地址（至少有16字节的空间）
 * @retval      ATK_IDE01_EOK   : 获取ATK-IDE01模块的网关地址成功
 *              ATK_IDE01_ERROR : 获取ATK-IDE01模块的网关地址失败
 *              ATK_IDE01_EINVAL: 函数参数错误
 */
rt_uint8_t atk_ide01_get_gateway(char *gateway)
{
    rt_uint8_t ret;
    char cmd[12];
    rt_uint8_t *buf;
    rt_uint16_t param_index;
    rt_uint16_t param_len;
    
    if (gateway == NULL)
    {
        return ATK_IDE01_EINVAL;
    }
    
    rt_sprintf(cmd, "AT+GATEWAY?");
    ret = atk_ide01_send_at_cmd(cmd, "OK", ATK_IDE01_AT_TIMEOUT);
    if (ret != ATK_IDE01_EOK)
    {
        return ATK_IDE01_ERROR;
    }
    
    buf = atk_ide01_uart_rx_get_frame();
    ret = atk_ide01_get_parameter(buf, 1, &param_index, &param_len);
    if (ret != ATK_IDE01_EOK)
    {
        return ATK_IDE01_ERROR;
    }
    rt_memcpy(gateway, &buf[param_index], param_len);
    gateway[param_len] = '\0';
    
    return ATK_IDE01_EOK;
}

/**
 * @brief       设置ATK-IDE01模块的网关地址
 * @param       gateway: ATK-IDE01模块的网关地址
 * @retval      ATK_IDE01_EOK   : 设置ATK-IDE01模块的网关地址成功
 *              ATK_IDE01_ERROR : 设置ATK-IDE01模块的网关地址失败
 */
rt_uint8_t atk_ide01_set_gateway(char *gateway)
{
    rt_uint8_t ret;
    char cmd[29];
    
    rt_sprintf(cmd, "AT+GATEWAY=\"%s\"", gateway);
    ret = atk_ide01_send_at_cmd(cmd, "OK", ATK_IDE01_AT_TIMEOUT);
    if (ret != ATK_IDE01_EOK)
    {
        return ATK_IDE01_ERROR;
    }
    
    return ATK_IDE01_EOK;
}

/**
 * @brief       获取ATK-IDE01模块的网络工作模式
 * @param       ethmod: ATK_IDE01_ETHMOD_UDP_CLIENT   : UDP客户端模式
 *                      ATK_IDE01_ETHMOD_UDP_SERVER   : UDP服务器模式
 *                      ATK_IDE01_ETHMOD_TCP_CLIENT   : TCP客户端模式
 *                      ATK_IDE01_ETHMOD_TCP_SERVER   : TCP服务器模式
 *                      ATK_IDE01_ETHMOD_TCP_CLOUD    : TCP_CLOUD模式
 *                      ATK_IDE01_ETHMOD_UDP_MULTICAST: UDP组播模式
 * @retval      ATK_IDE01_EOK   : 获取ATK-IDE01模块的网络工作模式成功
 *              ATK_IDE01_ERROR : 获取ATK-IDE01模块的网络工作模式失败
 *              ATK_IDE01_EINVAL: 函数参数错误
 */
rt_uint8_t atk_ide01_get_ethmod(atk_ide01_ethmod_t *ethmod)
{
    rt_uint8_t ret;
    char cmd[12];
    rt_uint8_t *buf;
    rt_uint16_t param_index;
    rt_uint16_t param_len;
    rt_uint8_t _ethmod[14];
    
    if (ethmod == NULL)
    {
        return ATK_IDE01_EINVAL;
    }
    
    rt_sprintf(cmd, "AT+ETH_MOD?");
    ret = atk_ide01_send_at_cmd(cmd, "OK", ATK_IDE01_AT_TIMEOUT);
    if (ret != ATK_IDE01_EOK)
    {
        return ATK_IDE01_ERROR;
    }
    
    buf = atk_ide01_uart_rx_get_frame();
    ret = atk_ide01_get_parameter(buf, 1, &param_index, &param_len);
    if (ret != ATK_IDE01_EOK)
    {
        return ATK_IDE01_ERROR;
    }
    rt_memcpy(_ethmod, &buf[param_index], param_len);
    _ethmod[param_len] = '\0';
    
    if (rt_strcmp((char *)_ethmod, "UDP_CLIENT") == 0)
    {
        *ethmod = ATK_IDE01_ETHMOD_UDP_CLIENT;
    }
    else if (rt_strcmp((char *)_ethmod, "UDP_SERVER") == 0)
    {
        *ethmod = ATK_IDE01_ETHMOD_UDP_SERVER;
    }
    else if (rt_strcmp((char *)_ethmod, "TCP_CLIENT") == 0)
    {
        *ethmod = ATK_IDE01_ETHMOD_TCP_CLIENT;
    }
    else if (rt_strcmp((char *)_ethmod, "TCP_SERVER") == 0)
    {
        *ethmod = ATK_IDE01_ETHMOD_TCP_SERVER;
    }
    else if (rt_strcmp((char *)_ethmod, "TCP_CLOUD") == 0)
    {
        *ethmod = ATK_IDE01_ETHMOD_TCP_CLOUD;
    }
    else if (rt_strcmp((char *)_ethmod, "UDP_MULTICAST") == 0)
    {
        *ethmod = ATK_IDE01_ETHMOD_UDP_MULTICAST;
    }
    else
    {
        return ATK_IDE01_ERROR;
    }
    
    return ATK_IDE01_EOK;
}



/**
 * @brief       设置ATK-IDE01模块的网络模式
 * @param       ethmod: ATK_IDE01_ETHMOD_UDP_CLIENT   : UDP客户端模式
 *                      ATK_IDE01_ETHMOD_UDP_SERVER   : UDP服务器模式
 *                      ATK_IDE01_ETHMOD_TCP_CLIENT   : TCP客户端模式
 *                      ATK_IDE01_ETHMOD_TCP_SERVER   : TCP服务器模式
 *                      ATK_IDE01_ETHMOD_TCP_CLOUD    : TCP_CLOUD模式
 *                      ATK_IDE01_ETHMOD_UDP_MULTICAST: UDP组播模式
 * @retval      ATK_IDE01_EOK   : 设置ATK-IDE01模块的网络模式成功
 *              ATK_IDE01_ERROR : 设置ATK-IDE01模块的网络模式失败
 *              ATK_IDE01_EINVAL: 函数参数错误
 */
rt_uint8_t atk_ide01_set_ethmod(atk_ide01_ethmod_t ethmod)
{
    rt_uint8_t ret;
    char cmd[27];
    
    switch (ethmod)
    {
        case ATK_IDE01_ETHMOD_UDP_CLIENT:
        {
            rt_sprintf(cmd, "AT+ETH_MOD=\"%s\"", "UDP_CLIENT");
            break;
        }
        case ATK_IDE01_ETHMOD_UDP_SERVER:
        {
            rt_sprintf(cmd, "AT+ETH_MOD=\"%s\"", "UDP_SERVER");
            break;
        }
        case ATK_IDE01_ETHMOD_TCP_CLIENT:
        {
            rt_sprintf(cmd, "AT+ETH_MOD=\"%s\"", "TCP_CLIENT");
            break;
        }
        case ATK_IDE01_ETHMOD_TCP_SERVER:
        {
            rt_sprintf(cmd, "AT+ETH_MOD=\"%s\"", "TCP_SERVER");
            break;
        }
        case ATK_IDE01_ETHMOD_TCP_CLOUD:
        {
            rt_sprintf(cmd, "AT+ETH_MOD=\"%s\"", "TCP_CLOUD");
            break;
        }
        case ATK_IDE01_ETHMOD_UDP_MULTICAST:
        {
            rt_sprintf(cmd, "AT+ETH_MOD=\"%s\"", "UDP_MULTICAST");
            break;
        }
        default:
        {
            return ATK_IDE01_EINVAL;
        }
    }
    
    ret = atk_ide01_send_at_cmd(cmd, "OK", ATK_IDE01_AT_TIMEOUT);
    if (ret != ATK_IDE01_EOK)
    {
        return ATK_IDE01_ERROR;
    }
    
    return ATK_IDE01_EOK;
}

/**
 * @brief       获取ATK-IDE01模块的DNS服务器地址
 * @note        获取到的DNS服务器地址格式为“255.255.255.255\0”（16字节）
 * @param       dns: ATK-IDE01模块的DNS服务器地址（至少有16字节的空间）
 * @retval      ATK_IDE01_EOK   : 获取ATK-IDE01模块的DNS服务器地址成功
 *              ATK_IDE01_ERROR : 获取ATK-IDE01模块的DNS服务器地址失败
 *              ATK_IDE01_EINVAL: 函数参数错误
 */
rt_uint8_t atk_ide01_get_dns(char *dns)
{
    rt_uint8_t ret;
    char cmd[15];
    rt_uint8_t *buf;
    rt_uint16_t param_index;
    rt_uint16_t param_len;
    
    if (dns == NULL)
    {
        return ATK_IDE01_EINVAL;
    }
    
    rt_sprintf(cmd, "AT+DNS_SERVER?");
    ret = atk_ide01_send_at_cmd(cmd, "OK", ATK_IDE01_AT_TIMEOUT);
    if (ret != ATK_IDE01_EOK)
    {
        return ATK_IDE01_ERROR;
    }
    
    buf = atk_ide01_uart_rx_get_frame();
    ret = atk_ide01_get_parameter(buf, 1, &param_index, &param_len);
    if (ret != ATK_IDE01_EOK)
    {
        return ATK_IDE01_ERROR;
    }
    rt_memcpy(dns, &buf[param_index], param_len);
    dns[param_len] = '\0';
    
    return ATK_IDE01_EOK;
}

/**
 * @brief       设置ATK-IDE01模块的DNS服务器地址
 * @param       gateway: ATK-IDE01模块的DNS服务器地址
 * @retval      ATK_IDE01_EOK   : 设置ATK-IDE01模块的DNS服务器地址成功
 *              ATK_IDE01_ERROR : 设置ATK-IDE01模块的DNS服务器地址失败
 */
rt_uint8_t atk_ide01_set_dns(char *dns)
{
    rt_uint8_t ret;
    char cmd[32];
    
    rt_sprintf(cmd, "AT+DNS_SERVER=\"%s\"", dns);
    ret = atk_ide01_send_at_cmd(cmd, "OK", ATK_IDE01_AT_TIMEOUT);
    if (ret != ATK_IDE01_EOK)
    {
        return ATK_IDE01_ERROR;
    }
    
    return ATK_IDE01_EOK;
}

/**
 * @brief       获取ATK-IDE01模块的本地开放端口
 * @note        获取到的本地开放端口格式为“65535\0”（6字节）
 * @param       localport: ATK-IDE01模块的本地开放端口（至少有6字节的空间）
 * @retval      ATK_IDE01_EOK   : 获取ATK-IDE01模块的本地开放端口成功
 *              ATK_IDE01_ERROR : 获取ATK-IDE01模块的本地开放端口失败
 *              ATK_IDE01_EINVAL: 函数参数错误
 */
rt_uint8_t atk_ide01_get_localport(char *localport)
{
    rt_uint8_t ret;
    char cmd[15];
    rt_uint8_t *buf;
    rt_uint16_t param_index;
    rt_uint16_t param_len;
    
    if (localport == NULL)
    {
        return ATK_IDE01_EINVAL;
    }
    
    rt_sprintf(cmd, "AT+LOCAL_PORT?");
    ret = atk_ide01_send_at_cmd(cmd, "OK", ATK_IDE01_AT_TIMEOUT);
    if (ret != ATK_IDE01_EOK)
    {
        return ATK_IDE01_ERROR;
    }
    
    buf = atk_ide01_uart_rx_get_frame();
    ret = atk_ide01_get_parameter(buf, 1, &param_index, &param_len);
    if (ret != ATK_IDE01_EOK)
    {
        return ATK_IDE01_ERROR;
    }
    rt_memcpy(localport, &buf[param_index], param_len);
    localport[param_len] = '\0';
    
    return ATK_IDE01_EOK;
}

/**
 * @brief       设置ATK-IDE01模块的本地开放端口
 * @param       localport: ATK-IDE01模块的本地开放端口
 * @retval      ATK_IDE01_EOK   : 设置ATK-IDE01模块的本地开放端口成功
 *              ATK_IDE01_ERROR : 设置ATK-IDE01模块的本地开放端口失败
 */
rt_uint8_t atk_ide01_set_localport(char *localport)
{
    rt_uint8_t ret;
    char cmd[22];
    
    rt_sprintf(cmd, "AT+LOCAL_PORT=\"%s\"", localport);
    ret = atk_ide01_send_at_cmd(cmd, "OK", ATK_IDE01_AT_TIMEOUT);
    if (ret != ATK_IDE01_EOK)
    {
        return ATK_IDE01_ERROR;
    }
    
    return ATK_IDE01_EOK;
}

/**
 * @brief       获取ATK-IDE01模块要连接的远程端口
 * @note        获取到要连接的远程端口格式为“65535\0”（6字节）
 * @param       remoteport: ATK-IDE01模块要连接的远程端口（至少有6字节的空间）
 * @retval      ATK_IDE01_EOK   : 获取ATK-IDE01模块的要连接的远程端口成功
 *              ATK_IDE01_ERROR : 获取ATK-IDE01模块的要连接的远程端口失败
 *              ATK_IDE01_EINVAL: 函数参数错误
 */
rt_uint8_t atk_ide01_get_remoteport(char *remoteport)
{
    rt_uint8_t ret;
    char cmd[16];
    rt_uint8_t *buf;
    rt_uint16_t param_index;
    rt_uint16_t param_len;
    
    if (remoteport == NULL)
    {
        return ATK_IDE01_EINVAL;
    }
    
    rt_sprintf(cmd, "AT+REMOTE_PORT?");
    ret = atk_ide01_send_at_cmd(cmd, "OK", ATK_IDE01_AT_TIMEOUT);
    if (ret != ATK_IDE01_EOK)
    {
        return ATK_IDE01_ERROR;
    }
    
    buf = atk_ide01_uart_rx_get_frame();
    ret = atk_ide01_get_parameter(buf, 1, &param_index, &param_len);
    if (ret != ATK_IDE01_EOK)
    {
        return ATK_IDE01_ERROR;
    }
    rt_memcpy(remoteport, &buf[param_index], param_len);
    remoteport[param_len] = '\0';
    
    return ATK_IDE01_EOK;
}

/**
 * @brief       设置ATK-IDE01模块要连接的远程端口
 * @param       remoteport: ATK-IDE01模块要连接的远程端口
 * @retval      ATK_IDE01_EOK   : 设置ATK-IDE01模块要连接的远程端口成功
 *              ATK_IDE01_ERROR : 设置ATK-IDE01模块要连接的远程端口失败
 */
rt_uint8_t atk_ide01_set_remoteport(char *remoteport)
{
    rt_uint8_t ret;
    char cmd[23];
    
    rt_sprintf(cmd, "AT+REMOTE_PORT=\"%s\"", remoteport);
    ret = atk_ide01_send_at_cmd(cmd, "OK", ATK_IDE01_AT_TIMEOUT);
    if (ret != ATK_IDE01_EOK)
    {
        return ATK_IDE01_ERROR;
    }
    
    return ATK_IDE01_EOK;
}

/**
 * @brief       获取ATK-IDE01模块要连接的远程IP地址
 * @note        获取到要连接的远程IP地址格式为“255.255.255.255\0”（16字节）
 * @param       remoteport: ATK-IDE01模块要连接的远程IP地址（至少有16字节的空间）
 * @retval      ATK_IDE01_EOK   : 获取ATK-IDE01模块要连接的远程IP地址成功
 *              ATK_IDE01_ERROR : 获取ATK-IDE01模块要连接的远程IP地址失败
 *              ATK_IDE01_EINVAL: 函数参数错误
 */
rt_uint8_t atk_ide01_get_remoteip(char *remoteip)
{
    rt_uint8_t ret;
    char cmd[13];
    rt_uint8_t *buf;
    rt_uint16_t buf_len;
    rt_uint16_t buf_index = 0;
    rt_uint16_t param_index;
    rt_uint16_t param_len;
    
    if (remoteip == NULL)
    {
        return ATK_IDE01_EINVAL;
    }
    
    rt_sprintf(cmd, "AT+REMOTEIP?");
    ret = atk_ide01_send_at_cmd(cmd, "OK", ATK_IDE01_AT_TIMEOUT);
    if (ret != ATK_IDE01_EOK)
    {
        return ATK_IDE01_ERROR;
    }
    
    buf = atk_ide01_uart_rx_get_frame();
    buf_len = atk_ide01_uart_rx_get_frame_len();
    while ((buf[buf_index] != '=') && (buf_index < buf_len))
    {
        buf_index++;
    }
    if (buf[buf_index] == '\0')
    {
        return ATK_IDE01_ERROR;
    }
    param_index = buf_index + 2;
    while ((buf[buf_index] != 0x0D) && (buf[buf_index + 1] != 0x0A) && (buf_index < buf_len))
    {
        buf_index++;
    }
    if (buf[buf_index] == '\0')
    {
        return ATK_IDE01_ERROR;
    }
    param_len = buf_index - param_index;
    rt_memcpy(remoteip, &buf[param_index], param_len);
    remoteip[param_len] = '\0';
    
    return ATK_IDE01_EOK;
}

/**
 * @brief       设置ATK-IDE01模块要连接的远程IP地址
 * @param       remoteip: ATK-IDE01模块要连接的远程IP地址
 * @retval      ATK_IDE01_EOK   : 设置ATK-IDE01模块要连接的远程IP地址成功
 *              ATK_IDE01_ERROR : 设置ATK-IDE01模块要连接的远程IP地址失败
 */
rt_uint8_t atk_ide01_set_remoteip(char *remoteip)
{
    rt_uint8_t ret;
    char cmd[30];
    
    rt_sprintf(cmd, "AT+REMOTEIP=\"%s\"", remoteip);
    ret = atk_ide01_send_at_cmd(cmd, "OK", ATK_IDE01_AT_TIMEOUT);
    if (ret != ATK_IDE01_EOK)
    {
        return ATK_IDE01_ERROR;
    }
    
    return ATK_IDE01_EOK;
}

/**
 * @brief       获取ATK-IDE01模块的静态IP地址
 * @note        获取到的静态IP地址格式为“255.255.255.255\0”（16字节）
 * @param       staticip: ATK-IDE01模块的静态IP地址（至少有16字节的空间）
 * @retval      ATK_IDE01_EOK   : 获取ATK-IDE01模块的静态IP地址成功
 *              ATK_IDE01_ERROR : 获取ATK-IDE01模块的静态IP地址失败
 *              ATK_IDE01_EINVAL: 函数参数错误
 */
rt_uint8_t atk_ide01_get_staticip(char *staticip)
{
    rt_uint8_t ret;
    char cmd[13];
    rt_uint8_t *buf;
    rt_uint16_t param_index;
    rt_uint16_t param_len;
    
    if (staticip == NULL)
    {
        return ATK_IDE01_EINVAL;
    }
    
    rt_sprintf(cmd, "AT+STATICIP?");
    ret = atk_ide01_send_at_cmd(cmd, "OK", ATK_IDE01_AT_TIMEOUT);
    if (ret != ATK_IDE01_EOK)
    {
        return ATK_IDE01_ERROR;
    }
    
    buf = atk_ide01_uart_rx_get_frame();
    ret = atk_ide01_get_parameter(buf, 1, &param_index, &param_len);
    if (ret != ATK_IDE01_EOK)
    {
        return ATK_IDE01_ERROR;
    }
    rt_memcpy(staticip, &buf[param_index], param_len);
    staticip[param_len] = '\0';
    
    return ATK_IDE01_EOK;
}

/**
 * @brief       设置ATK-IDE01模块的静态IP地址
 * @param       staticip: ATK-IDE01模块的静态IP地址
 * @retval      ATK_IDE01_EOK   : 设置ATK-IDE01模块的静态IP地址成功
 *              ATK_IDE01_ERROR : 设置ATK-IDE01模块的静态IP地址失败
 */
rt_uint8_t atk_ide01_set_staticip(char *staticip)
{
    rt_uint8_t ret;
    char cmd[30];
    
    rt_sprintf(cmd, "AT+STATICIP=\"%s\"", staticip);
    ret = atk_ide01_send_at_cmd(cmd, "OK", ATK_IDE01_AT_TIMEOUT);
    if (ret != ATK_IDE01_EOK)
    {
        return ATK_IDE01_ERROR;
    }
    
    return ATK_IDE01_EOK;
}

/**
 * @brief       获取ATK-IDE01模块的指令回显状态
 * @param       echo: ATK_IDE01_ECHO_ON : 使能指令回显
 *                    ATK_IDE01_ECHO_OFF: 关闭指令回显
 * @retval      ATK_IDE01_EOK   : 获取ATK-IDE01模块的指令回显状态成功
 *              ATK_IDE01_ERROR : 获取ATK-IDE01模块的指令回显状态失败
 *              ATK_IDE01_EINVAL: 函数参数错误
 */
rt_uint8_t atk_ide01_get_echo(atk_ide01_echo_t *echo)
{
    rt_uint8_t ret;
    char cmd[9];
    rt_uint8_t *buf;
    rt_uint16_t param_index;
    rt_uint16_t param_len;
    rt_uint8_t _echo[4];
    
    if (echo == NULL)
    {
        return ATK_IDE01_EINVAL;
    }
    
    rt_sprintf(cmd, "AT+ECHO?");
    ret = atk_ide01_send_at_cmd(cmd, "OK", ATK_IDE01_AT_TIMEOUT);
    if (ret != ATK_IDE01_EOK)
    {
        return ATK_IDE01_ERROR;
    }
    
    buf = atk_ide01_uart_rx_get_frame();
    ret = atk_ide01_get_parameter(buf, 1, &param_index, &param_len);
    if (ret != ATK_IDE01_EOK)
    {
        return ATK_IDE01_ERROR;
    }
    rt_memcpy(_echo, &buf[param_index], param_len);
    _echo[param_len] = '\0';
    
    if (rt_strcmp((char *)_echo, "ON") == 0)
    {
        *echo = ATK_IDE01_ECHO_ON;
    }
    else if (rt_strcmp((char *)_echo, "OFF") == 0)
    {
        *echo = ATK_IDE01_ECHO_OFF;
    }
    else
    {
        return ATK_IDE01_ERROR;
    }
    
    return ATK_IDE01_EOK;
}

/**
 * @brief       设置ATK-IDE01模块的指令回显状态
 * @param       echo: ATK-IDE01模块的指令回显状态
 * @retval      ATK_IDE01_EOK   : 设置ATK-IDE01模块的指令回显状态成功
 *              ATK_IDE01_ERROR : 设置ATK-IDE01模块的指令回显状态失败
 *              ATK_IDE01_EINVAL: 函数参数错误
 */
rt_uint8_t atk_ide01_set_echo(atk_ide01_echo_t echo)
{
    rt_uint8_t ret;
    char cmd[14];
    
    switch (echo)
    {
        case ATK_IDE01_ECHO_ON:
        {
            rt_sprintf(cmd, "AT+ECHO=\"%s\"", "ON");
            break;
        }
        case ATK_IDE01_ECHO_OFF:
        {
            rt_sprintf(cmd, "AT+ECHO=\"%s\"", "OFF");
            break;
        }
        default:
        {
            return ATK_IDE01_EINVAL;
        }
    }
    
    ret = atk_ide01_send_at_cmd(cmd, "OK", ATK_IDE01_AT_TIMEOUT);
    if (ret != ATK_IDE01_EOK)
    {
        return ATK_IDE01_ERROR;
    }
    
    return ATK_IDE01_EOK;
}

/**
 * @brief       获取ATK-IDE01模块的DHCP状态
 * @param       dhcp: ATK_IDE01_DHCP_ON : 使能DHCP功能
 *                    ATK_IDE01_DHCP_OFF: 关闭DHCP功能
 * @retval      ATK_IDE01_EOK   : 获取ATK-IDE01模块的DHCP状态成功
 *              ATK_IDE01_ERROR : 获取ATK-IDE01模块的DHCP状态失败
 *              ATK_IDE01_EINVAL: 函数参数错误
 */
rt_uint8_t atk_ide01_get_dhcp(atk_ide01_dhcp_t *dhcp)
{
    rt_uint8_t ret;
    char cmd[9];
    rt_uint8_t *buf;
    rt_uint16_t param_index;
    rt_uint16_t param_len;
    rt_uint8_t _echo[4];
    
    if (dhcp == NULL)
    {
        return ATK_IDE01_EINVAL;
    }
    
    rt_sprintf(cmd, "AT+DHCP?");
    ret = atk_ide01_send_at_cmd(cmd, "OK", ATK_IDE01_AT_TIMEOUT);
    if (ret != ATK_IDE01_EOK)
    {
        return ATK_IDE01_ERROR;
    }
    
    buf = atk_ide01_uart_rx_get_frame();
    ret = atk_ide01_get_parameter(buf, 1, &param_index, &param_len);
    if (ret != ATK_IDE01_EOK)
    {
        return ATK_IDE01_ERROR;
    }
    rt_memcpy(_echo, &buf[param_index], param_len);
    _echo[param_len] = '\0';
    
    if (rt_strcmp((char *)_echo, "ON") == 0)
    {
        *dhcp = ATK_IDE01_DHCP_ON;
    }
    else if (rt_strcmp((char *)_echo, "OFF") == 0)
    {
        *dhcp = ATK_IDE01_DHCP_OFF;
    }
    else
    {
        return ATK_IDE01_ERROR;
    }
    
    return ATK_IDE01_EOK;
}

/**
 * @brief       设置ATK-IDE01模块的DHCP状态
 * @param       dhcp: ATK-IDE01模块的DHCP状态
 * @retval      ATK_IDE01_EOK   : 设置ATK-IDE01模块的DHCP状态成功
 *              ATK_IDE01_ERROR : 设置ATK-IDE01模块的DHCP状态失败
 *              ATK_IDE01_EINVAL: 函数参数错误
 */
rt_uint8_t atk_ide01_set_dhcp(atk_ide01_dhcp_t dhcp)
{
    rt_uint8_t ret;
    char cmd[14];
    
    switch (dhcp)
    {
        case ATK_IDE01_DHCP_ON:
        {
            rt_sprintf(cmd, "AT+DHCP=\"%s\"", "ON");
            break;
        }
        case ATK_IDE01_DHCP_OFF:
        {
            rt_sprintf(cmd, "AT+DHCP=\"%s\"", "OFF");
            break;
        }
        default:
        {
            return ATK_IDE01_EINVAL;
        }
    }
    
    ret = atk_ide01_send_at_cmd(cmd, "OK", ATK_IDE01_AT_TIMEOUT);
    if (ret != ATK_IDE01_EOK)
    {
        return ATK_IDE01_ERROR;
    }
    
    return ATK_IDE01_EOK;
}

/**
 * @brief       复位ATK-IDE01模块
 * @note        ATK-IDE01需要大约15秒的复位时间
 *              复位后需重新进入AT指令模式，才能进行AT指令交互
 * @param       无
 * @retval      ATK_IDE01_EOK   : 复位ATK-IDE01模块成功
 *              ATK_IDE01_ERROR : 复位ATK-IDE01模块失败
 */
rt_uint8_t atk_ide01_reset(void)
{
    rt_uint8_t ret;
    char cmd[9];
    
    rt_sprintf(cmd, "AT+RESET");
    ret = atk_ide01_send_at_cmd(cmd, "OK", ATK_IDE01_AT_TIMEOUT);
    if (ret != ATK_IDE01_EOK)
    {
        return ATK_IDE01_ERROR;
    }
    
    return ATK_IDE01_EOK;
}

/**
 * @brief       更新ATK-IDE01模块配置并保存
 * @note        保存后的配置，断电后不丢失
 * @param       无
 * @retval      ATK_IDE01_EOK   : 更新ATK-IDE01模块配置并保存成功
 *              ATK_IDE01_ERROR : 更新ATK-IDE01模块配置并保存失败
 */
rt_uint8_t atk_ide01_update(void)
{
    rt_uint8_t ret;
    char cmd[10];
    
    rt_sprintf(cmd, "AT+UPDATE");
    ret = atk_ide01_send_at_cmd(cmd, "OK", ATK_IDE01_AT_TIMEOUT);
    if (ret != ATK_IDE01_EOK)
    {
        return ATK_IDE01_ERROR;
    }
    
    return ATK_IDE01_EOK;
}

/**
 * @brief       获取ATK-IDE01模块的软硬件版本
 * @note        获取到的软硬件版本格式为“H_0000/S_0000\0”（14字节）
 * @param       version: ATK-IDE01模块的软硬件版本（至少有14字节的空间）
 * @retval      ATK_IDE01_EOK   : 获取ATK-IDE01模块的软硬件版本成功
 *              ATK_IDE01_ERROR : 获取ATK-IDE01模块的软硬件版本失败
 */
rt_uint8_t atk_ide01_get_version(char *version)
{
    rt_uint8_t ret;
    char cmd[12];
    rt_uint8_t *buf;
    rt_uint16_t param_index;
    rt_uint16_t param_len;
    
    rt_sprintf(cmd, "AT+VERSION?");
    ret = atk_ide01_send_at_cmd(cmd, "OK", ATK_IDE01_AT_TIMEOUT);
    if (ret != ATK_IDE01_EOK)
    {
        return ATK_IDE01_ERROR;
    }
    
    buf = atk_ide01_uart_rx_get_frame();
    ret = atk_ide01_get_parameter(buf, 1, &param_index, &param_len);
    if (ret != ATK_IDE01_EOK)
    {
        return ATK_IDE01_ERROR;
    }
    rt_memcpy(version, &buf[param_index], param_len);
    version[param_len] = '\0';
    
    return ATK_IDE01_EOK;
}

/**
 * @brief       获取ATK-IDE01模块名称的长度
 * @note        获取到的名称长度为模块名称的实际长度+'\0'占用的1字符
 * @param       len: ATK-IDE01模块名称的长度
 * @retval      ATK_IDE01_EOK   : 获取ATK-IDE01模块名称的长度成功
 *              ATK_IDE01_ERROR : 获取ATK-IDE01模块名称的长度失败
 *              ATK_IDE01_EINVAL: 函数参数错误
 */
rt_uint8_t atk_ide01_get_moduname_len(rt_uint16_t *len)
{
    rt_uint8_t ret;
    char cmd[13];
    rt_uint8_t *buf;
    rt_uint16_t buf_len;
    rt_uint16_t buf_index = 0;
    rt_uint16_t param_index;
    
    if (len == NULL)
    {
        return ATK_IDE01_EINVAL;
    }
    
    rt_sprintf(cmd, "AT+MODUNAME?");
    ret = atk_ide01_send_at_cmd(cmd, "OK", ATK_IDE01_AT_TIMEOUT);
    if (ret != ATK_IDE01_EOK)
    {
        return ATK_IDE01_ERROR;
    }
    
    buf = atk_ide01_uart_rx_get_frame();
    buf_len = atk_ide01_uart_rx_get_frame_len();
    while ((buf[buf_index] != '=') && (buf_index < buf_len))
    {
        buf_index++;
    }
    if (buf[buf_index] == '\0')
    {
        return ATK_IDE01_ERROR;
    }
    param_index = buf_index + 2;
    while ((buf[buf_index] != 0x0D) && (buf[buf_index + 1] != 0x0A) && (buf_index < buf_len))
    {
        buf_index++;
    }
    if (buf[buf_index] == '\0')
    {
        return ATK_IDE01_ERROR;
    }
    *len = buf_index - param_index + 1;
    
    return ATK_IDE01_EOK;
}

/**
 * @brief       获取ATK-IDE01模块的名称
 * @note        模块名称的长度由函数atk_ide01_get_moduname_len()获取
 * @param       moduname: ATK-IDE01模块的名称
 * @retval      ATK_IDE01_EOK   : 获取ATK-IDE01模块的名称成功
 *              ATK_IDE01_ERROR : 获取ATK-IDE01模块的名称失败
 *              ATK_IDE01_EINVAL: 函数参数错误
 */
rt_uint8_t atk_ide01_get_moduname(char *moduname)
{
    rt_uint8_t ret;
    char cmd[13];
    rt_uint8_t *buf;
    rt_uint16_t buf_len;
    rt_uint16_t buf_index = 0;
    rt_uint16_t param_index;
    rt_uint16_t param_len;
    
    if (moduname == NULL)
    {
        return ATK_IDE01_EINVAL;
    }
    
    rt_sprintf(cmd, "AT+MODUNAME?");
    ret = atk_ide01_send_at_cmd(cmd, "OK", ATK_IDE01_AT_TIMEOUT);
    if (ret != ATK_IDE01_EOK)
    {
        return ATK_IDE01_ERROR;
    }
    
    buf = atk_ide01_uart_rx_get_frame();
    buf_len = atk_ide01_uart_rx_get_frame_len();
    while ((buf[buf_index] != '=') && (buf_index < buf_len))
    {
        buf_index++;
    }
    if (buf[buf_index] == '\0')
    {
        return ATK_IDE01_ERROR;
    }
    param_index = buf_index + 2;
    while ((buf[buf_index] != 0x0D) && (buf[buf_index + 1] != 0x0A) && (buf_index < buf_len))
    {
        buf_index++;
    }
    if (buf[buf_index] == '\0')
    {
        return ATK_IDE01_ERROR;
    }
    param_len = buf_index - param_index;
    rt_memcpy(moduname, &buf[param_index], param_len);
    moduname[param_len] = '\0';
    
    return ATK_IDE01_EOK;
}

/**
 * @brief       设置ATK-IDE01模块的名称
 * @param       moduname: ATK-IDE01模块的名称
 * @retval      ATK_IDE01_EOK   : 设置ATK-IDE01模块的名称成功
 *              ATK_IDE01_ERROR : 设置ATK-IDE01模块的名称失败
 */
rt_uint8_t atk_ide01_set_moduname(char *moduname)
{
    rt_uint8_t ret;
    char cmd[64];
    
    rt_sprintf(cmd, "AT+MODUNAME=\"%s\"", moduname);
    ret = atk_ide01_send_at_cmd(cmd, "OK", ATK_IDE01_AT_TIMEOUT);
    if (ret != ATK_IDE01_EOK)
    {
        return ATK_IDE01_ERROR;
    }
    
    return ATK_IDE01_EOK;
}

/**
 * @brief       获取ATK-IDE01模块的上位机配置端口
 * @note        获取到的上位机配置端口格式为“65535\0”（6字节）
 * @param       xcomport: ATK-IDE01模块的本地开放端口（至少有6字节的空间）
 * @retval      ATK_IDE01_EOK   : 获取ATK-IDE01模块的上位机配置端口成功
 *              ATK_IDE01_ERROR : 获取ATK-IDE01模块的上位机配置端口失败
 *              ATK_IDE01_EINVAL: 函数参数错误
 */
rt_uint8_t atk_ide01_get_xcomport(char *xcomport)
{
    rt_uint8_t ret;
    char cmd[14];
    rt_uint8_t *buf;
    rt_uint16_t param_index;
    rt_uint16_t param_len;
    
    if (xcomport == NULL)
    {
        return ATK_IDE01_EINVAL;
    }
    
    rt_sprintf(cmd, "AT+XCOM_PORT?");
    ret = atk_ide01_send_at_cmd(cmd, "OK", ATK_IDE01_AT_TIMEOUT);
    if (ret != ATK_IDE01_EOK)
    {
        return ATK_IDE01_ERROR;
    }
    
    buf = atk_ide01_uart_rx_get_frame();
    ret = atk_ide01_get_parameter(buf, 1, &param_index, &param_len);
    if (ret != ATK_IDE01_EOK)
    {
        return ATK_IDE01_ERROR;
    }
    rt_memcpy(xcomport, &buf[param_index], param_len);
    xcomport[param_len] = '\0';
    
    return ATK_IDE01_EOK;
}

/**
 * @brief       设置ATK-IDE01模块的上位机配置端口
 * @param       xcomport: ATK-IDE01模块的上位机配置端口
 * @retval      ATK_IDE01_EOK   : 设置ATK-IDE01模块的上位机配置端口成功
 *              ATK_IDE01_ERROR : 设置ATK-IDE01模块的上位机配置端口失败
 */
rt_uint8_t atk_ide01_set_xcomport(char *xcomport)
{
    rt_uint8_t ret;
    char cmd[21];
    
    rt_sprintf(cmd, "AT+XCOM_PORT=\"%s\"", xcomport);
    ret = atk_ide01_send_at_cmd(cmd, "OK", ATK_IDE01_AT_TIMEOUT);
    if (ret != ATK_IDE01_EOK)
    {
        return ATK_IDE01_ERROR;
    }
    
    return ATK_IDE01_EOK;
}

/**
 * @brief       获取ATK-IDE01模块的串口配置参数
 * @param       baudrate: 波特率
 *              stop    : 数据位
 *              data    : 校验位
 *              parity  : 停止位
 * @retval      ATK_IDE01_EOK   : 获取ATK-IDE01模块的串口配置参数成功
 *              ATK_IDE01_ERROR : 获取ATK-IDE01模块的串口配置参数失败
 *              ATK_IDE01_EINVAL: 函数参数错误
 */
rt_uint8_t atk_ide01_get_uart(atk_ide01_uart_baudrate_t *baudrate, atk_ide01_uart_stop_t *stop, atk_ide01_uart_data_t *data, atk_ide01_uart_parity_t *parity)
{
    rt_uint8_t ret;
    char cmd[9];
    rt_uint8_t *buf;
    rt_uint8_t _baudrate[7];
    rt_uint8_t _stop[2];
    rt_uint8_t _data[2];
    rt_uint8_t _parity[5];
    rt_uint16_t param_index;
    rt_uint16_t param_len;
    
    if ((baudrate == NULL) && (stop == NULL) && (data == NULL) && (parity == NULL))
    {
        return ATK_IDE01_EINVAL;
    }
    
    rt_sprintf(cmd, "AT+UART?");
    ret = atk_ide01_send_at_cmd(cmd, "OK", ATK_IDE01_AT_TIMEOUT);
    if (ret != ATK_IDE01_EOK)
    {
        return ATK_IDE01_ERROR;
    }
    
    buf = atk_ide01_uart_rx_get_frame();
    if (baudrate != NULL)
    {
        ret = atk_ide01_get_parameter(buf, 1, &param_index, &param_len);
        if (ret != ATK_IDE01_EOK)
        {
            return ATK_IDE01_ERROR;
        }
        rt_memcpy(_baudrate, &buf[param_index], param_len);
        _baudrate[param_len] = '\0';
        if (rt_strcmp((char *)_baudrate, "2400") == 0)
        {
            *baudrate = ATK_IDE01_UART_BAUDRATE_2400;
        }
        else if (rt_strcmp((char *)_baudrate, "4800") == 0)
        {
            *baudrate = ATK_IDE01_UART_BAUDRATE_4800;
        }
        else if (rt_strcmp((char *)_baudrate, "9600") == 0)
        {
            *baudrate = ATK_IDE01_UART_BAUDRATE_9600;
        }
        else if (rt_strcmp((char *)_baudrate, "14400") == 0)
        {
            *baudrate = ATK_IDE01_UART_BAUDRATE_14400;
        }
        else if (rt_strcmp((char *)_baudrate, "38400") == 0)
        {
            *baudrate = ATK_IDE01_UART_BAUDRATE_38400;
        }
        else if (rt_strcmp((char *)_baudrate, "43000") == 0)
        {
            *baudrate = ATK_IDE01_UART_BAUDRATE_43000;
        }
        else if (rt_strcmp((char *)_baudrate, "57600") == 0)
        {
            *baudrate = ATK_IDE01_UART_BAUDRATE_57600;
        }
        else if (rt_strcmp((char *)_baudrate, "76800") == 0)
        {
            *baudrate = ATK_IDE01_UART_BAUDRATE_76800;
        }
        else if (rt_strcmp((char *)_baudrate, "115200") == 0)
        {
            *baudrate = ATK_IDE01_UART_BAUDRATE_115200;
        }
        else if (rt_strcmp((char *)_baudrate, "128000") == 0)
        {
            *baudrate = ATK_IDE01_UART_BAUDRATE_128000;
        }
        else if (rt_strcmp((char *)_baudrate, "230400") == 0)
        {
            *baudrate = ATK_IDE01_UART_BAUDRATE_230400;
        }
        else if (rt_strcmp((char *)_baudrate, "256000") == 0)
        {
            *baudrate = ATK_IDE01_UART_BAUDRATE_256000;
        }
        else if (rt_strcmp((char *)_baudrate, "460800") == 0)
        {
            *baudrate = ATK_IDE01_UART_BAUDRATE_460800;
        }
        else if (rt_strcmp((char *)_baudrate, "921600") == 0)
        {
            *baudrate = ATK_IDE01_UART_BAUDRATE_921600;
        }
        else
        {
            return ATK_IDE01_ERROR;
        }
    }
    if (stop != NULL)
    {
        ret = atk_ide01_get_parameter(buf, 2, &param_index, &param_len);
        if (ret != ATK_IDE01_EOK)
        {
            return ATK_IDE01_ERROR;
        }
        rt_memcpy(_stop, &buf[param_index], param_len);
        _stop[param_len] = '\0';
        if (rt_strcmp((char *)_stop, "1") == 0)
        {
            *stop = ATK_IDE01_UART_STOP_1;
        }
        else if (rt_strcmp((char *)_stop, "2") == 0)
        {
            *stop = ATK_IDE01_UART_STOP_2;
        }
        else
        {
            return ATK_IDE01_ERROR;
        }
    }
    if (data != NULL)
    {
        ret = atk_ide01_get_parameter(buf, 3, &param_index, &param_len);
        if (ret != ATK_IDE01_EOK)
        {
            return ATK_IDE01_ERROR;
        }
        rt_memcpy(_data, &buf[param_index], param_len);
        _data[param_len] = '\0';
        if (rt_strcmp((char *)_data, "8") == 0)
        {
            *data = ATK_IDE01_UART_DATA_8;
        }
        else
        {
            return ATK_IDE01_ERROR;
        }
    }
    if (parity != NULL)
    {
        ret = atk_ide01_get_parameter(buf, 4, &param_index, &param_len);
        if (ret != ATK_IDE01_EOK)
        {
            return ATK_IDE01_ERROR;
        }
        rt_memcpy(_parity, &buf[param_index], param_len);
        _parity[param_len] = '\0';
        if (rt_strcmp((char *)_parity, "NONE") == 0)
        {
            *parity = ATK_IDE01_UART_PARITY_NONE;
        }
        else if (rt_strcmp((char *)_parity, "EVEN") == 0)
        {
            *parity = ATK_IDE01_UART_PARITY_EVEN;
        }
        else if (rt_strcmp((char *)_parity, "ODD") == 0)
        {
            *parity = ATK_IDE01_UART_PARITY_ODD;
        }
        else
        {
            return ATK_IDE01_ERROR;
        }
    }
    
    return ATK_IDE01_EOK;
}

/**
 * @brief       设置ATK-IDE01模块的串口配置参数
 * @param       baudrate: 波特率
 *              stop    : 数据位
 *              data    : 校验位
 *              parity  : 停止位
 * @retval      ATK_IDE01_EOK   : 设置ATK-IDE01模块的串口配置参数成功
 *              ATK_IDE01_ERROR : 设置ATK-IDE01模块的串口配置参数失败
 */
rt_uint8_t atk_ide01_set_uart(atk_ide01_uart_baudrate_t baudrate, atk_ide01_uart_stop_t stop, atk_ide01_uart_data_t data, atk_ide01_uart_parity_t parity)
{
    rt_uint8_t ret;
    char cmd[32];
    char _baudrate[7];
    char _stop[2];
    char _data[2];
    char _parity[5];
    
    switch (baudrate)
    {
        case ATK_IDE01_UART_BAUDRATE_2400:
        {
            rt_sprintf(_baudrate, "2400");
            break;
        }
        case ATK_IDE01_UART_BAUDRATE_4800:
        {
            rt_sprintf(_baudrate, "4800");
            break;
        }
        case ATK_IDE01_UART_BAUDRATE_9600:
        {
            rt_sprintf(_baudrate, "9600");
            break;
        }
        case ATK_IDE01_UART_BAUDRATE_14400:
        {
            rt_sprintf(_baudrate, "14400");
            break;
        }
        case ATK_IDE01_UART_BAUDRATE_19200:
        {
            rt_sprintf(_baudrate, "19200");
            break;
        }
        case ATK_IDE01_UART_BAUDRATE_38400:
        {
            rt_sprintf(_baudrate, "38400");
            break;
        }
        case ATK_IDE01_UART_BAUDRATE_43000:
        {
            rt_sprintf(_baudrate, "43000");
            break;
        }
        case ATK_IDE01_UART_BAUDRATE_57600:
        {
            rt_sprintf(_baudrate, "57600");
            break;
        }
        case ATK_IDE01_UART_BAUDRATE_76800:
        {
            rt_sprintf(_baudrate, "76800");
            break;
        }
        case ATK_IDE01_UART_BAUDRATE_115200:
        {
            rt_sprintf(_baudrate, "115200");
            break;
        }
        case ATK_IDE01_UART_BAUDRATE_128000:
        {
            rt_sprintf(_baudrate, "120000");
            break;
        }
        case ATK_IDE01_UART_BAUDRATE_230400:
        {
            rt_sprintf(_baudrate, "230400");
            break;
        }
        case ATK_IDE01_UART_BAUDRATE_256000:
        {
            rt_sprintf(_baudrate, "256000");
            break;
        }
        case ATK_IDE01_UART_BAUDRATE_460800:
        {
            rt_sprintf(_baudrate, "460800");
            break;
        }
        case ATK_IDE01_UART_BAUDRATE_921600:
        {
            rt_sprintf(_baudrate, "921600");
            break;
        }
        default:
        {
            return ATK_IDE01_EINVAL;
        }
    }
    
    switch (stop)
    {
        case ATK_IDE01_UART_STOP_1:
        {
            rt_sprintf(_stop, "1");
            break;
        }
        case ATK_IDE01_UART_STOP_2:
        {
            rt_sprintf(_stop, "2");
            break;
        }
        default:
        {
            return ATK_IDE01_EINVAL;
        }
    }
    
    switch (data)
    {
        case ATK_IDE01_UART_DATA_8:
        {
            rt_sprintf(_data, "8");
            break;
        }
        default:
        {
            return ATK_IDE01_EINVAL;
        }
    }
    
    switch (parity)
    {
        case ATK_IDE01_UART_PARITY_NONE:
        {
            rt_sprintf(_parity, "NONE");
            break;
        }
        case ATK_IDE01_UART_PARITY_EVEN:
        {
            rt_sprintf(_parity, "EVEN");
            break;
        }
        case ATK_IDE01_UART_PARITY_ODD:
        {
            rt_sprintf(_parity, "ODD");
            break;
        }
        default:
        {
            return ATK_IDE01_EINVAL;
        }
    }
    
    rt_sprintf(cmd, "AT+UART=\"%s\",\"%s\",\"%s\",\"%s\"", _baudrate, _stop, _data, _parity);
    ret = atk_ide01_send_at_cmd(cmd, "OK", ATK_IDE01_AT_TIMEOUT);
    if (ret != ATK_IDE01_EOK)
    {
        return ATK_IDE01_ERROR;
    }
    
    return ATK_IDE01_EOK;
}

/**
 * @brief       获取ATK-IDE01模块的串口打包时间
 * @param       uartpacktime: ATK-IDE01模块的串口打包时间，单位：毫秒
 * @retval      ATK_IDE01_EOK   : 获取ATK-IDE01模块的串口打包时间成功
 *              ATK_IDE01_ERROR : 获取ATK-IDE01模块的串口打包时间失败
 *              ATK_IDE01_EINVAL: 函数参数错误
 */
rt_uint8_t atk_ide01_get_uartpacktime(rt_uint16_t *uartpacktime)
{
    rt_uint8_t ret;
    char cmd[18];
    rt_uint8_t *buf;
    char _uartpacktime[6];
    rt_uint16_t param_index;
    rt_uint16_t param_len;
    
    if (uartpacktime == NULL)
    {
        return ATK_IDE01_EINVAL;
    }
    
    rt_sprintf(cmd, "AT+UART_PACKTIME?");
    ret = atk_ide01_send_at_cmd(cmd, "OK", ATK_IDE01_AT_TIMEOUT);
    if (ret != ATK_IDE01_EOK)
    {
        return ATK_IDE01_ERROR;
    }
    
    buf = atk_ide01_uart_rx_get_frame();
    ret = atk_ide01_get_parameter(buf, 1, &param_index, &param_len);
    if (ret != ATK_IDE01_EOK)
    {
        return ATK_IDE01_ERROR;
    }
    rt_memcpy(_uartpacktime, &buf[param_index], param_len);
    _uartpacktime[param_len] = '\0';
    *uartpacktime = (rt_uint16_t)atoi(_uartpacktime);
    
    return ATK_IDE01_EOK;
}

/**
 * @brief       设置ATK-IDE01模块的串口打包时间
 * @param       uartpacktime: ATK-IDE01模块的串口打包时间
 * @retval      ATK_IDE01_EOK   : 设置ATK-IDE01模块的串口打包时间成功
 *              ATK_IDE01_ERROR : 设置ATK-IDE01模块的串口打包时间失败
 */
rt_uint8_t atk_ide01_set_uartpacktime(rt_uint16_t uartpacktime)
{
    rt_uint8_t ret;
    char cmd[25];
    
    rt_sprintf(cmd, "AT+UART_PACKTIME=\"%d\"", uartpacktime);
    ret = atk_ide01_send_at_cmd(cmd, "OK", ATK_IDE01_AT_TIMEOUT);
    if (ret != ATK_IDE01_EOK)
    {
        return ATK_IDE01_ERROR;
    }
    
    return ATK_IDE01_EOK;
}

/**
 * @brief       获取ATK-IDE01模块的串口打包长度
 * @param       uartpacklen: ATK-IDE01模块的串口打包长度，单位：字节，范围：1~1450
 * @retval      ATK_IDE01_EOK   : 获取ATK-IDE01模块的串口打包长度成功
 *              ATK_IDE01_ERROR : 获取ATK-IDE01模块的串口打包长度失败
 *              ATK_IDE01_EINVAL: 函数参数错误
 */
rt_uint8_t atk_ide01_get_uartpacklen(rt_uint16_t *uartpacklen)
{
    rt_uint8_t ret;
    char cmd[17];
    rt_uint8_t *buf;
    char _uartpacklen[6];
    rt_uint16_t param_index;
    rt_uint16_t param_len;
    
    if (uartpacklen == NULL)
    {
        return ATK_IDE01_EINVAL;
    }
    
    rt_sprintf(cmd, "AT+UART_PACKLEN?");
    ret = atk_ide01_send_at_cmd(cmd, "OK", ATK_IDE01_AT_TIMEOUT);
    if (ret != ATK_IDE01_EOK)
    {
        return ATK_IDE01_ERROR;
    }
    
    buf = atk_ide01_uart_rx_get_frame();
    ret = atk_ide01_get_parameter(buf, 1, &param_index, &param_len);
    if (ret != ATK_IDE01_EOK)
    {
        return ATK_IDE01_ERROR;
    }
    rt_memcpy(_uartpacklen, &buf[param_index], param_len);
    _uartpacklen[param_len] = '\0';
    *uartpacklen = (rt_uint16_t)atoi(_uartpacklen);
    
    return ATK_IDE01_EOK;
}

/**
 * @brief       设置ATK-IDE01模块的串口打包长度
 * @param       uartpacklen: ATK-IDE01模块的串口打包长度，单位：字节，范围：1~1450
 * @retval      ATK_IDE01_EOK   : 设置ATK-IDE01模块的串口打包时间成功
 *              ATK_IDE01_ERROR : 设置ATK-IDE01模块的串口打包时间失败
 *              ATK_IDE01_EINVAL: 函数参数错误
 */
rt_uint8_t atk_ide01_set_uartpacklen(rt_uint16_t uartpacklen)
{
    rt_uint8_t ret;
    char cmd[24];
    
    if ((uartpacklen < 1) || (uartpacklen > 1450))
    {
        return ATK_IDE01_EINVAL;
    }
    
    rt_sprintf(cmd, "AT+UART_PACKLEN=\"%d\"", uartpacklen);
    ret = atk_ide01_send_at_cmd(cmd, "OK", ATK_IDE01_AT_TIMEOUT);
    if (ret != ATK_IDE01_EOK)
    {
        return ATK_IDE01_ERROR;
    }
    
    return ATK_IDE01_EOK;
}

/**
 * @brief       获取ATK-IDE01模块的原子云启用状态
 * @param       cloudlink: ATK_IDE01_CLOUDLINK_ON : 使能原子云
 *                         ATK_IDE01_CLOUDLINK_OFF: 关闭原子云
 * @retval      ATK_IDE01_EOK   : 获取ATK-IDE01模块的原子云启用状态成功
 *              ATK_IDE01_ERROR : 获取ATK-IDE01模块的原子云启用状态失败
 *              ATK_IDE01_EINVAL: 函数参数错误
 */
rt_uint8_t atk_ide01_get_cloudlinken(atk_ide01_cloudlink_t *cloudlink)
{
    rt_uint8_t ret;
    char cmd[18];
    rt_uint8_t *buf;
    rt_uint16_t param_index;
    rt_uint16_t param_len;
    rt_uint8_t _cloudlink[4];
    
    if (cloudlink == NULL)
    {
        return ATK_IDE01_EINVAL;
    }
    
    rt_sprintf(cmd, "AT+CLOUD_LINK_EN?");
    ret = atk_ide01_send_at_cmd(cmd, "OK", ATK_IDE01_AT_TIMEOUT);
    if (ret != ATK_IDE01_EOK)
    {
        return ATK_IDE01_ERROR;
    }
    
    buf = atk_ide01_uart_rx_get_frame();
    ret = atk_ide01_get_parameter(buf, 1, &param_index, &param_len);
    if (ret != ATK_IDE01_EOK)
    {
        return ATK_IDE01_ERROR;
    }
    rt_memcpy(_cloudlink, &buf[param_index], param_len);
    _cloudlink[param_len] = '\0';
    
    if (rt_strcmp((char *)_cloudlink, "ON") == 0)
    {
        *cloudlink = ATK_IDE01_CLOUDLINK_ON;
    }
    else if (rt_strcmp((char *)_cloudlink, "OFF") == 0)
    {
        *cloudlink = ATK_IDE01_CLOUDLINK_OFF;
    }
    else
    {
        return ATK_IDE01_ERROR;
    }
    
    return ATK_IDE01_EOK;
}

/**
 * @brief       设置ATK-IDE01模块的原子云启用状态
 * @param       cloudlink: ATK_IDE01_CLOUDLINK_ON : 使能原子云
 *                         ATK_IDE01_CLOUDLINK_OFF: 关闭原子云
 * @retval      ATK_IDE01_EOK   : 设置ATK-IDE01模块的原子云启用状态成功
 *              ATK_IDE01_ERROR : 设置ATK-IDE01模块的原子云启用状态失败
 *              ATK_IDE01_EINVAL: 函数参数错误
 */
rt_uint8_t atk_ide01_set_cloudlinken(atk_ide01_cloudlink_t cloudlink)
{
    rt_uint8_t ret;
    char cmd[23];
    
    switch (cloudlink)
    {
        case ATK_IDE01_CLOUDLINK_ON:
        {
            rt_sprintf(cmd, "AT+CLOUD_LINK_EN=\"%s\"", "ON");
            break;
        }
        case ATK_IDE01_CLOUDLINK_OFF:
        {
            rt_sprintf(cmd, "AT+CLOUD_LINK_EN=\"%s\"", "OFF");
            break;
        }
        default:
        {
            return ATK_IDE01_EINVAL;
        }
    }
    
    ret = atk_ide01_send_at_cmd(cmd, "OK", ATK_IDE01_AT_TIMEOUT);
    if (ret != ATK_IDE01_EOK)
    {
        return ATK_IDE01_ERROR;
    }
    
    return ATK_IDE01_EOK;
}

/**
 * @brief       获取ATK-IDE01模块指定连接域名的连接端口
 * @note        获取到的连接端口格式为“65535\0”（6字节）
 * @param       cloudport: ATK-IDE01模块指定连接域名的连接端口（至少有6字节的空间）
 * @retval      ATK_IDE01_EOK   : 获取ATK-IDE01模块指定连接域名的连接端口成功
 *              ATK_IDE01_ERROR : 获取ATK-IDE01模块指定连接域名的连接端口失败
 *              ATK_IDE01_EINVAL: 函数参数错误
 */
rt_uint8_t atk_ide01_get_cloudport(char *cloudport)
{
    rt_uint8_t ret;
    char cmd[15];
    rt_uint8_t *buf;
    rt_uint16_t param_index;
    rt_uint16_t param_len;
    
    if (cloudport == NULL)
    {
        return ATK_IDE01_EINVAL;
    }
    
    rt_sprintf(cmd, "AT+CLOUD_PORT?");
    ret = atk_ide01_send_at_cmd(cmd, "OK", ATK_IDE01_AT_TIMEOUT);
    if (ret != ATK_IDE01_EOK)
    {
        return ATK_IDE01_ERROR;
    }
    
    buf = atk_ide01_uart_rx_get_frame();
    ret = atk_ide01_get_parameter(buf, 1, &param_index, &param_len);
    if (ret != ATK_IDE01_EOK)
    {
        return ATK_IDE01_ERROR;
    }
    rt_memcpy(cloudport, &buf[param_index], param_len);
    cloudport[param_len] = '\0';
    
    return ATK_IDE01_EOK;
}

/**
 * @brief       设置ATK-IDE01模块指定连接域名的连接端口
 * @param       cloudport: ATK-IDE01模块指定连接域名的连接端口
 * @retval      ATK_IDE01_EOK   : 设置ATK-IDE01模块指定连接域名的连接端口成功
 *              ATK_IDE01_ERROR : 设置ATK-IDE01模块指定连接域名的连接端口失败
 */
rt_uint8_t atk_ide01_set_cloudport(char *cloudport)
{
    rt_uint8_t ret;
    char cmd[22];
    
    rt_sprintf(cmd, "AT+CLOUD_PORT=\"%s\"", cloudport);
    ret = atk_ide01_send_at_cmd(cmd, "OK", ATK_IDE01_AT_TIMEOUT);
    if (ret != ATK_IDE01_EOK)
    {
        return ATK_IDE01_ERROR;
    }
    
    return ATK_IDE01_EOK;
}

/**
 * @brief       获取ATK-IDE01模块指定连接云的域名或IP地址的长度
 * @note        获取到的指定连接云的域名或IP地址长度为模块指定连接云的域名或IP地址的实际长度+'\0'占用的1字符
 * @param       len: ATK-IDE01模块指定连接云的域名或IP地址的长度
 * @retval      ATK_IDE01_EOK   : 获取ATK-IDE01模块指定连接云的域名或IP地址的长度成功
 *              ATK_IDE01_ERROR : 获取ATK-IDE01模块指定连接云的域名或IP地址的长度失败
 *              ATK_IDE01_EINVAL: 函数参数错误
 */
rt_uint8_t atk_ide01_get_clouddomain_len(rt_uint16_t *len)
{
    rt_uint8_t ret;
    char cmd[17];
    rt_uint8_t *buf;
    rt_uint16_t param_index;
    rt_uint16_t param_len;
    
    if (len == NULL)
    {
        return ATK_IDE01_EINVAL;
    }
    
    rt_sprintf(cmd, "AT+COLUD_DOMAIN?");
    ret = atk_ide01_send_at_cmd(cmd, "OK", ATK_IDE01_AT_TIMEOUT);
    if (ret != ATK_IDE01_EOK)
    {
        return ATK_IDE01_ERROR;
    }
    
    buf = atk_ide01_uart_rx_get_frame();
    ret = atk_ide01_get_parameter(buf, 1, &param_index, &param_len);
    if (ret != ATK_IDE01_EOK)
    {
        return ATK_IDE01_ERROR;
    }
    
    *len = param_len + 1;
    
    return ATK_IDE01_EOK;
}

/**
 * @brief       获取ATK-IDE01模块指定连接云的域名或IP地址
 * @note        模块指定连接云的域名或IP地址的长度由函数atk_ide01_get_clouddomain_len()获取
 * @param       clouddomain: ATK-IDE01模块指定连接云的域名或IP地址
 * @retval      ATK_IDE01_EOK   : 获取ATK-IDE01模块指定连接云的域名或IP地址成功
 *              ATK_IDE01_ERROR : 获取ATK-IDE01模块指定连接云的域名或IP地址失败
 *              ATK_IDE01_EINVAL: 函数参数错误
 */
rt_uint8_t atk_ide01_get_clouddomain(char *clouddomain)
{
    rt_uint8_t ret;
    char cmd[17];
    rt_uint8_t *buf;
    rt_uint16_t param_index;
    rt_uint16_t param_len;
    
    if (clouddomain == NULL)
    {
        return ATK_IDE01_EINVAL;
    }
    
    rt_sprintf(cmd, "AT+COLUD_DOMAIN?");
    ret = atk_ide01_send_at_cmd(cmd, "OK", ATK_IDE01_AT_TIMEOUT);
    if (ret != ATK_IDE01_EOK)
    {
        return ATK_IDE01_ERROR;
    }
    
    buf = atk_ide01_uart_rx_get_frame();
    ret = atk_ide01_get_parameter(buf, 1, &param_index, &param_len);
    if (ret != ATK_IDE01_EOK)
    {
        return ATK_IDE01_ERROR;
    }
    rt_memcpy(clouddomain, &buf[param_index], param_len);
    clouddomain[param_len] = '\0';
    
    return ATK_IDE01_EOK;
}

/**
 * @brief       设置ATK-IDE01模块指定连接云的域名或IP地址
 * @param       clouddomain: ATK-IDE01模块指定连接云的域名或IP地址
 * @retval      ATK_IDE01_EOK   : 设置ATK-IDE01模块指定连接云的域名或IP地址成功
 *              ATK_IDE01_ERROR : 设置ATK-IDE01模块指定连接云的域名或IP地址失败
 */
rt_uint8_t atk_ide01_set_clouddomain(char *clouddomain)
{
    rt_uint8_t ret;
    char cmd[64];
    
    rt_sprintf(cmd, "AT+COLUD_DOMAIN=\"%s\"", clouddomain);
    ret = atk_ide01_send_at_cmd(cmd, "OK", ATK_IDE01_AT_TIMEOUT);
    if (ret != ATK_IDE01_EOK)
    {
        return ATK_IDE01_ERROR;
    }
    
    return ATK_IDE01_EOK;
}

/**
 * @brief       获取ATK-IDE01模块原子云透传设备密码
 * @note        获取到的原子云透传设备密码格式为“12345678\0”（9字节）
 * @param       atklinkpwd: ATK-IDE01模块原子云透传设备密码（至少有9字节的空间）
 * @retval      ATK_IDE01_EOK   : 获取ATK-IDE01模块原子云透传设备密码成功
 *              ATK_IDE01_ERROR : 获取ATK-IDE01模块原子云透传设备密码失败
 *              ATK_IDE01_EINVAL: 函数参数错误
 */
rt_uint8_t atk_ide01_get_atklinkpwd(char *atklinkpwd)
{
    rt_uint8_t ret;
    char cmd[15];
    rt_uint8_t *buf;
    rt_uint16_t param_index;
    rt_uint16_t param_len;
    
    if (atklinkpwd == NULL)
    {
        return ATK_IDE01_EINVAL;
    }
    
    rt_sprintf(cmd, "AT+ATKLINKPWD?");
    ret = atk_ide01_send_at_cmd(cmd, "OK", ATK_IDE01_AT_TIMEOUT);
    if (ret != ATK_IDE01_EOK)
    {
        return ATK_IDE01_ERROR;
    }
    
    buf = atk_ide01_uart_rx_get_frame();
    ret = atk_ide01_get_parameter(buf, 1, &param_index, &param_len);
    if (ret != ATK_IDE01_EOK)
    {
        return ATK_IDE01_ERROR;
    }
    rt_memcpy(atklinkpwd, &buf[param_index], param_len);
    atklinkpwd[param_len] = '\0';
    
    return ATK_IDE01_EOK;
}

/**
 * @brief       设置ATK-IDE01模块原子云透传设备密码
 * @param       atklinkpwd: ATK-IDE01模块原子云透传设备密码
 * @retval      ATK_IDE01_EOK   : 设置ATK-IDE01模块原子云透传设备密码成功
 *              ATK_IDE01_ERROR : 设置ATK-IDE01模块原子云透传设备密码失败
 */
rt_uint8_t atk_ide01_set_atklinkpwd(char *atklinkpwd)
{
    rt_uint8_t ret;
    char cmd[25];
    
    rt_sprintf(cmd, "AT+ATKLINKPWD=\"%s\"", atklinkpwd);
    ret = atk_ide01_send_at_cmd(cmd, "OK", ATK_IDE01_AT_TIMEOUT);
    if (ret != ATK_IDE01_EOK)
    {
        return ATK_IDE01_ERROR;
    }
    
    return ATK_IDE01_EOK;
}

/**
 * @brief       获取ATK-IDE01模块原子云透传设备ID
 * @note        获取到的原子云透传设备ID格式为“01234567890123456789\0”（21字节）
 * @param       atkyundevid: ATK-IDE01模块原子云透传设备ID（至少有21字节的空间）
 * @retval      ATK_IDE01_EOK   : 获取ATK-IDE01模块原子云透传设备ID成功
 *              ATK_IDE01_ERROR : 获取ATK-IDE01模块原子云透传设备ID失败
 *              ATK_IDE01_EINVAL: 函数参数错误
 */
rt_uint8_t atk_ide01_get_atkyundevid(char *atkyundevid)
{
    rt_uint8_t ret;
    char cmd[16];
    rt_uint8_t *buf;
    rt_uint16_t param_index;
    rt_uint16_t param_len;
    
    if (atkyundevid == NULL)
    {
        return ATK_IDE01_EINVAL;
    }
    
    rt_sprintf(cmd, "AT+ATKYUNDEVID?");
    ret = atk_ide01_send_at_cmd(cmd, "OK", ATK_IDE01_AT_TIMEOUT);
    if (ret != ATK_IDE01_EOK)
    {
        return ATK_IDE01_ERROR;
    }
    
    buf = atk_ide01_uart_rx_get_frame();
    ret = atk_ide01_get_parameter(buf, 1, &param_index, &param_len);
    if (ret != ATK_IDE01_EOK)
    {
        return ATK_IDE01_ERROR;
    }
    rt_memcpy(atkyundevid, &buf[param_index], param_len);
    atkyundevid[param_len] = '\0';
    
    return ATK_IDE01_EOK;
}

/**
 * @brief       设置ATK-IDE01模块原子云透传设备ID
 * @param       atkyundevid: ATK-IDE01模块原子云透传设备ID
 * @retval      ATK_IDE01_EOK   : 设置ATK-IDE01模块原子云透传设备ID成功
 *              ATK_IDE01_ERROR : 设置ATK-IDE01模块原子云透传设备ID失败
 *              ATK_IDE01_EINVAL: 函数参数错误
 */
rt_uint8_t atk_ide01_set_atkyundevid(char *atkyundevid)
{
    rt_uint8_t ret;
    char cmd[38];
    
    if (rt_strlen((char *)atkyundevid) != 20)
    {
        return ATK_IDE01_EINVAL;
    }
    
    rt_sprintf(cmd, "AT+ATKYUNDEVID=\"%s\"", atkyundevid);
    ret = atk_ide01_send_at_cmd(cmd, "OK", ATK_IDE01_AT_TIMEOUT);
    if (ret != ATK_IDE01_EOK)
    {
        return ATK_IDE01_ERROR;
    }
    
    return ATK_IDE01_EOK;
}

/**
 * @brief       获取ATK-IDE01模块当前网络模式的连接状态
 * @param       netlksta: ATK_IDE01_NETLKSTA_ON : 当前网络模式的网络已连接成功
 *                        ATK_IDE01_NETLKSTA_OFF: 当前网络模式的网络未连接成功
 * @retval      ATK_IDE01_EOK   : 获取ATK-IDE01模块当前网络模式的连接状态成功
 *              ATK_IDE01_ERROR : 获取ATK-IDE01模块当前网络模式的连接状态失败
 *              ATK_IDE01_EINVAL: 函数参数错误
 */
rt_uint8_t atk_ide01_get_netlksta(atk_ide01_netlksta_t *netlksta)
{
    rt_uint8_t ret;
    char cmd[13];
    rt_uint8_t *buf;
    rt_uint16_t param_index;
    rt_uint16_t param_len;
    rt_uint8_t _netlksta[4];
    
    if (netlksta == NULL)
    {
        return ATK_IDE01_EINVAL;
    }
    
    rt_sprintf(cmd, "AT+NETLK_ST?");
    ret = atk_ide01_send_at_cmd(cmd, "OK", ATK_IDE01_AT_TIMEOUT);
    if (ret != ATK_IDE01_EOK)
    {
        return ATK_IDE01_ERROR;
    }
    
    buf = atk_ide01_uart_rx_get_frame();
    ret = atk_ide01_get_parameter(buf, 1, &param_index, &param_len);
    if (ret != ATK_IDE01_EOK)
    {
        return ATK_IDE01_ERROR;
    }
    rt_memcpy(_netlksta, &buf[param_index], param_len);
    _netlksta[param_len] = '\0';
    
    if (rt_strcmp((char *)_netlksta, "ON") == 0)
    {
        *netlksta = ATK_IDE01_NETLKSTA_ON;
    }
    else if (rt_strcmp((char *)_netlksta, "OFF") == 0)
    {
        *netlksta = ATK_IDE01_NETLKSTA_OFF;
    }
    else
    {
        return ATK_IDE01_ERROR;
    }
    
    return ATK_IDE01_EOK;
}

/**
 * @brief       获取ATK-IDE01模块与指定的域名或IP是否连通
 * @param       domain: 指定的域名或IP
 * @retval      ATK_IDE01_EOK   : ATK-IDE01模块与指定的域名或IP连通
 *              ATK_IDE01_ERROR : ATK-IDE01模块与指定的域名或IP不连通
 *              ATK_IDE01_EINVAL: 函数参数错误
 */
rt_uint8_t atk_ide01_ping(char *domain)
{
    rt_uint8_t ret;
    char cmd[64];
    
    if (domain == NULL)
    {
        return ATK_IDE01_EINVAL;
    }
    
    rt_sprintf(cmd, "AT+PING=\"%s\"", domain);
    ret = atk_ide01_send_at_cmd(cmd, "OK", ATK_IDE01_AT_TIMEOUT);
    if (ret != ATK_IDE01_EOK)
    {
        return ATK_IDE01_ERROR;
    }
    
    return ATK_IDE01_EOK;
}

/**
 * @brief       获取ATK-IDE01模块已建立的连接数
 * @param       linknum: ATK-IDE01模块已建立的连接数
 * @retval      ATK_IDE01_EOK   : 获取ATK-IDE01模块已建立的连接数成功
 *              ATK_IDE01_ERROR : 获取ATK-IDE01模块已建立的连接数失败
 *              ATK_IDE01_EINVAL: 函数参数错误
 */
rt_uint8_t atk_ide01_get_linknum(rt_uint16_t *linknum)
{
    rt_uint8_t ret;
    char cmd[12];
    rt_uint8_t *buf;
    char _linknum[6];
    rt_uint16_t param_index;
    rt_uint16_t param_len;
    
    if (linknum == NULL)
    {
        return ATK_IDE01_EINVAL;
    }
    
    rt_sprintf(cmd, "AT+LINKNUM?");
    ret = atk_ide01_send_at_cmd(cmd, "OK", ATK_IDE01_AT_TIMEOUT);
    if (ret != ATK_IDE01_EOK)
    {
        return ATK_IDE01_ERROR;
    }
    
    buf = atk_ide01_uart_rx_get_frame();
    ret = atk_ide01_get_parameter(buf, 1, &param_index, &param_len);
    if (ret != ATK_IDE01_EOK)
    {
        return ATK_IDE01_ERROR;
    }
    rt_memcpy(_linknum, &buf[param_index], param_len);
    _linknum[param_len] = '\0';
    *linknum = (rt_uint16_t)atoi(_linknum);
    
    return ATK_IDE01_EOK;
}

/**
 * @brief       获取ATK-IDE01模块连接的远程IP和端口
 * @note        获取到连接的远程IP地址格式为“255.255.255.255\0”（16字节）
 *              获取到连接的远程端口地址格式为“65535\0”（6字节）
 * @param       num       : ATK-IDE01模块内部连接的顺序
 *              remoteip  : ATK-IDE01模块连接的远程IP地址（至少有16字节的空间）
 *              remoteport: ATK-IDE01模块连接的远程端口（至少有6字节的空间）
 * @retval      ATK_IDE01_EOK   : 获取ATK-IDE01模块连接的远程IP和端口成功
 *              ATK_IDE01_ERROR : 获取ATK-IDE01模块连接的远程IP和端口失败
 *              ATK_IDE01_EINVAL: 函数参数错误
 */
rt_uint8_t atk_ide01_get_remotelinkinfo(rt_uint8_t num, char *remoteip, char *remoteport)
{
    rt_uint8_t ret;
    char cmd[20];
    rt_uint8_t *buf;
    rt_uint16_t param_index;
    rt_uint16_t param_len;
    
    if ((remoteip == NULL) && (remoteport == NULL))
    {
        return ATK_IDE01_EINVAL;
    }
    
    rt_sprintf(cmd, "AT+LINKINF_RM=\"%d\"", num);
    ret = atk_ide01_send_at_cmd(cmd, "OK", ATK_IDE01_AT_TIMEOUT);
    if (ret != ATK_IDE01_EOK)
    {
        return ATK_IDE01_ERROR;
    }
    
    buf = atk_ide01_uart_rx_get_frame();
    if (remoteip != NULL)
    {
        ret = atk_ide01_get_parameter(buf, 1, &param_index, &param_len);
        if (ret != ATK_IDE01_EOK)
        {
            return ATK_IDE01_ERROR;
        }
        rt_memcpy(remoteip, &buf[param_index], param_len);
        remoteip[param_len] = '\0';
    }
    if (remoteport != NULL)
    {
        ret = atk_ide01_get_parameter(buf, 2, &param_index, &param_len);
        if (ret != ATK_IDE01_EOK)
        {
            return ATK_IDE01_ERROR;
        }
        rt_memcpy(remoteport, &buf[param_index], param_len);
        remoteport[param_len] = '\0';
    }
    
    return ATK_IDE01_EOK;
}

/**
 * @brief       获取ATK-IDE01模块连接的本地IP和端口
 * @note        获取到连接的本地IP地址格式为“255.255.255.255\0”（16字节）
 *              获取到连接的本地端口地址格式为“65535\0”（6字节）
 * @param       num       : ATK-IDE01模块内部连接的顺序
 *              remoteip  : ATK-IDE01模块连接的本地IP地址（至少有16字节的空间）
 *              remoteport: ATK-IDE01模块连接的本地端口（至少有6字节的空间）
 * @retval      ATK_IDE01_EOK   : 获取ATK-IDE01模块连接的本地IP和端口成功
 *              ATK_IDE01_ERROR : 获取ATK-IDE01模块连接的本地IP和端口失败
 *              ATK_IDE01_EINVAL: 函数参数错误
 */
rt_uint8_t atk_ide01_get_locallinkinfo(rt_uint8_t num, char *localip, char *localport)
{
    rt_uint8_t ret;
    char cmd[20];
    rt_uint8_t *buf;
    rt_uint16_t param_index;
    rt_uint16_t param_len;
    
    if ((localip == NULL) && (localport == NULL))
    {
        return ATK_IDE01_EINVAL;
    }
    
    rt_sprintf(cmd, "AT+LINKINF_LC=\"%d\"", num);
    ret = atk_ide01_send_at_cmd(cmd, "OK", ATK_IDE01_AT_TIMEOUT);
    if (ret != ATK_IDE01_EOK)
    {
        return ATK_IDE01_ERROR;
    }
    
    buf = atk_ide01_uart_rx_get_frame();
    if (localip != NULL)
    {
        ret = atk_ide01_get_parameter(buf, 1, &param_index, &param_len);
        if (ret != ATK_IDE01_EOK)
        {
            return ATK_IDE01_ERROR;
        }
        rt_memcpy(localip, &buf[param_index], param_len);
        localip[param_len] = '\0';
    }
    if (localport != NULL)
    {
        ret = atk_ide01_get_parameter(buf, 2, &param_index, &param_len);
        if (ret != ATK_IDE01_EOK)
        {
            return ATK_IDE01_ERROR;
        }
        rt_memcpy(localport, &buf[param_index], param_len);
        localport[param_len] = '\0';
    }
    
    return ATK_IDE01_EOK;
}

/**
 * @brief       获取ATK-IDE01模块的UDP组播IP地址
 * @note        获取到的UDP组播IP地址格式为“255.255.255.255\0”（16字节）
 * @param       multcastip: ATK-IDE01模块的UDP组播IP地址（至少有16字节的空间）
 * @retval      ATK_IDE01_EOK   : 获取ATK-IDE01模块的UDP组播IP地址成功
 *              ATK_IDE01_ERROR : 获取ATK-IDE01模块的UDP组播IP地址失败
 *              ATK_IDE01_EINVAL: 函数参数错误
 */
rt_uint8_t atk_ide01_get_multcastip(char *multcastip)
{
    rt_uint8_t ret;
    char cmd[16];
    rt_uint8_t *buf;
    rt_uint16_t param_index;
    rt_uint16_t param_len;
    
    if (multcastip == NULL)
    {
        return ATK_IDE01_EINVAL;
    }
    
    rt_sprintf(cmd, "AT+MULTCAST_IP?");
    ret = atk_ide01_send_at_cmd(cmd, "OK", ATK_IDE01_AT_TIMEOUT);
    if (ret != ATK_IDE01_EOK)
    {
        return ATK_IDE01_ERROR;
    }
    
    buf = atk_ide01_uart_rx_get_frame();
    ret = atk_ide01_get_parameter(buf, 1, &param_index, &param_len);
    if (ret != ATK_IDE01_EOK)
    {
        return ATK_IDE01_ERROR;
    }
    rt_memcpy(multcastip, &buf[param_index], param_len);
    multcastip[param_len] = '\0';
    
    return ATK_IDE01_EOK;
}

/**
 * @brief       设置ATK-IDE01模块的UDP组播IP地址
 * @param       multcastip: ATK-IDE01模块的UDP组播IP地址
 * @retval      ATK_IDE01_EOK   : 设置ATK-IDE01模块的UDP组播IP地址成功
 *              ATK_IDE01_ERROR : 设置ATK-IDE01模块的UDP组播IP地址失败
 */
rt_uint8_t atk_ide01_set_multcastip(char *multcastip)
{
    rt_uint8_t ret;
    char cmd[33];
    
    rt_sprintf(cmd, "AT+MULTCAST_IP=\"%s\"", multcastip);
    ret = atk_ide01_send_at_cmd(cmd, "OK", ATK_IDE01_AT_TIMEOUT);
    if (ret != ATK_IDE01_EOK)
    {
        return ATK_IDE01_ERROR;
    }
    
    return ATK_IDE01_EOK;
}

/**
 * @brief       获取ATK-IDE01模块的UDP组播端口
 * @note        获取到的UDP组播端口格式为“65535\0”（6字节）
 * @param       multcastport: ATK-IDE01模块的UDP组播端口（至少有6字节的空间）
 * @retval      ATK_IDE01_EOK   : 获取ATK-IDE01模块的UDP组播端口成功
 *              ATK_IDE01_ERROR : 获取ATK-IDE01模块的UDP组播端口失败
 *              ATK_IDE01_EINVAL: 函数参数错误
 */
rt_uint8_t atk_ide01_get_multcastport(char *multcastport)
{
    rt_uint8_t ret;
    char cmd[18];
    rt_uint8_t *buf;
    rt_uint16_t param_index;
    rt_uint16_t param_len;
    
    if (multcastport == NULL)
    {
        return ATK_IDE01_EINVAL;
    }
    
    rt_sprintf(cmd, "AT+MULTCAST_PORT?");
    ret = atk_ide01_send_at_cmd(cmd, "OK", ATK_IDE01_AT_TIMEOUT);
    if (ret != ATK_IDE01_EOK)
    {
        return ATK_IDE01_ERROR;
    }
    
    buf = atk_ide01_uart_rx_get_frame();
    ret = atk_ide01_get_parameter(buf, 1, &param_index, &param_len);
    if (ret != ATK_IDE01_EOK)
    {
        return ATK_IDE01_ERROR;
    }
    rt_memcpy(multcastport, &buf[param_index], param_len);
    multcastport[param_len] = '\0';
    
    return ATK_IDE01_EOK;
}

/**
 * @brief       设置ATK-IDE01模块的UDP组播端口
 * @param       multcastport: ATK-IDE01模块的UDP组播端口
 * @retval      ATK_IDE01_EOK   : 设置ATK-IDE01模块的UDP组播端口成功
 *              ATK_IDE01_ERROR : 设置ATK-IDE01模块的UDP组播端口失败
 */
rt_uint8_t atk_ide01_set_multcastport(char *multcastport)
{
    rt_uint8_t ret;
    char cmd[35];
    
    rt_sprintf(cmd, "AT+MULTCAST_PORT=\"%s\"", multcastport);
    ret = atk_ide01_send_at_cmd(cmd, "OK", ATK_IDE01_AT_TIMEOUT);
    if (ret != ATK_IDE01_EOK)
    {
        return ATK_IDE01_ERROR;
    }
    
    return ATK_IDE01_EOK;
}
