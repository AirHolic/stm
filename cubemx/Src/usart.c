/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    usart.c
  * @brief   This file provides code for the configuration
  *          of the USART instances.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2023 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "usart.h"

/* USER CODE BEGIN 0 */
#if USART3_TCPNET


static struct
{
  rt_uint8_t buf[ATK_IDE01_UART_RX_BUF_SIZE]; /* 帧接收缓冲 */
  struct
  {
    rt_uint16_t len : 15;                                    /* 帧接收长度，sta[14:0] */
    rt_uint16_t finsh : 1;                                   /* 帧接收完成标志，sta[15] */
  } sta;                                                  /* 帧状态信息 */
} g_uart_rx_frame = {0};                                  /* ATK-IDE01 UART接收帧缓冲信息结构体 */
static rt_uint8_t g_uart_tx_buf[ATK_IDE01_UART_TX_BUF_SIZE]; /* ATK-IDE01 UART发送缓冲 */

#endif
/* USER CODE END 0 */

UART_HandleTypeDef huart3;

/* USART3 init function */

void MX_USART3_UART_Init(void)
{

  /* USER CODE BEGIN USART3_Init 0 */

  /* USER CODE END USART3_Init 0 */

  /* USER CODE BEGIN USART3_Init 1 */

  /* USER CODE END USART3_Init 1 */
  huart3.Instance = USART3;
  huart3.Init.BaudRate = 115200;
  huart3.Init.WordLength = UART_WORDLENGTH_8B;
  huart3.Init.StopBits = UART_STOPBITS_1;
  huart3.Init.Parity = UART_PARITY_NONE;
  huart3.Init.Mode = UART_MODE_TX_RX;
  huart3.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart3.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart3) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART3_Init 2 */

  /* USER CODE END USART3_Init 2 */

}

void HAL_UART_MspInit(UART_HandleTypeDef* uartHandle)
{

  GPIO_InitTypeDef GPIO_InitStruct = {0};
  if(uartHandle->Instance==USART3)
  {
  /* USER CODE BEGIN USART3_MspInit 0 */

  /* USER CODE END USART3_MspInit 0 */
    /* USART3 clock enable */
    __HAL_RCC_USART3_CLK_ENABLE();

    __HAL_RCC_GPIOB_CLK_ENABLE();
    /**USART3 GPIO Configuration
    PB10     ------> USART3_TX
    PB11     ------> USART3_RX
    */
    GPIO_InitStruct.Pin = USART3_TX_Pin|USART3_RX_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF7_USART3;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    /* USART3 interrupt Init */
    HAL_NVIC_SetPriority(USART3_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(USART3_IRQn);
  /* USER CODE BEGIN USART3_MspInit 1 */
    __HAL_UART_ENABLE_IT(uartHandle, UART_IT_RXNE);
    __HAL_UART_ENABLE_IT(uartHandle, UART_IT_IDLE);
  /* USER CODE END USART3_MspInit 1 */
  }
}

void HAL_UART_MspDeInit(UART_HandleTypeDef* uartHandle)
{

  if(uartHandle->Instance==USART3)
  {
  /* USER CODE BEGIN USART3_MspDeInit 0 */

  /* USER CODE END USART3_MspDeInit 0 */
    /* Peripheral clock disable */
    __HAL_RCC_USART3_CLK_DISABLE();

    /**USART3 GPIO Configuration
    PB10     ------> USART3_TX
    PB11     ------> USART3_RX
    */
    HAL_GPIO_DeInit(GPIOB, USART3_TX_Pin|USART3_RX_Pin);

    /* USART3 interrupt Deinit */
    HAL_NVIC_DisableIRQ(USART3_IRQn);
  /* USER CODE BEGIN USART3_MspDeInit 1 */
    __HAL_UART_DISABLE_IT(uartHandle, UART_IT_RXNE);
    //__HAL_UART_DISENABLE_IT(uartHandle, UART_IT_IDLE);
  /* USER CODE END USART3_MspDeInit 1 */
  }
}

/* USER CODE BEGIN 1 */

/**
 * @brief       ATK-IDE01 UART中断回调函数
 * @param       无
 * @retval      无
 */
void USART3_IRQHandler(void)
{
  rt_uint8_t tmp;
    
    if (__HAL_UART_GET_FLAG(&huart3, UART_FLAG_ORE) != RESET)        /* UART接收过载错误中断 */
    {
        __HAL_UART_CLEAR_OREFLAG(&huart3);                           /* 清除接收过载错误中断标志 */
        (void)huart3.Instance->SR;                                   /* 先读SR寄存器，再读DR寄存器 */
        (void)huart3.Instance->DR;
    }
    
    if (__HAL_UART_GET_FLAG(&huart3, UART_FLAG_RXNE) != RESET)       /* UART接收中断 */
    {
        HAL_UART_Receive(&huart3, &tmp, 1, HAL_MAX_DELAY);           /* UART接收数据 */
        
        if (g_uart_rx_frame.sta.len < (ATK_IDE01_UART_RX_BUF_SIZE - 1))     /* 判断UART接收缓冲是否溢出
                                                                             * 留出一位给结束符'\0'
                                                                             */
        {
            g_uart_rx_frame.buf[g_uart_rx_frame.sta.len] = tmp;             /* 将接收到的数据写入缓缓冲 */
            g_uart_rx_frame.sta.len++;                                      /* 更新接收到的数据长度 */
        }
        else                                                                /* UART接收缓冲溢出 */
        {
            g_uart_rx_frame.sta.len = 0;                                    /* 覆盖之前收到的数据 */
            g_uart_rx_frame.buf[g_uart_rx_frame.sta.len] = tmp;             /* 将接收到的数据写入缓缓冲 */
            g_uart_rx_frame.sta.len++;                                      /* 更新接收到的数据长度 */
        }
    }
    
    if (__HAL_UART_GET_FLAG(&huart3, UART_FLAG_IDLE) != RESET)       /* UART总线空闲中断 */
    {
        g_uart_rx_frame.sta.finsh = 1;                                      /* 标记帧接收完成 */
        
        __HAL_UART_CLEAR_IDLEFLAG(&huart3);                          /* 清除UART总线空闲中断 */
    }
}

/**
 * @brief       ATK-IDE01 UART printf
 * @param       fmt: 待打印的数据
 * @retval      无
 */
void atk_ide01_uart_printf(char *fmt, ...)
{
    va_list ap;
    rt_uint16_t len;
    
    va_start(ap, fmt);
    rt_vsprintf((char *)g_uart_tx_buf, fmt, ap);
    va_end(ap);
    
    len = strlen((const char *)g_uart_tx_buf);
    HAL_UART_Transmit(&huart3, g_uart_tx_buf, len, HAL_MAX_DELAY);
}

/**
 * @brief       ATK-IDE01 UART重新开始接收数据
 * @param       无
 * @retval      无
 */
void atk_ide01_uart_rx_restart(void)
{
    g_uart_rx_frame.sta.len     = 0;
    g_uart_rx_frame.sta.finsh   = 0;
}


/**
 * @brief       获取ATK-IDE01 UART接收到的一帧数据
 * @param       无
 * @retval      NULL: 未接收到一帧数据
 *              其他: 接收到的一帧数据
 */
rt_uint8_t *atk_ide01_uart_rx_get_frame(void)
{
    if (g_uart_rx_frame.sta.finsh == 1)
    {
        g_uart_rx_frame.buf[g_uart_rx_frame.sta.len] = '\0';
        return g_uart_rx_frame.buf;
    }
    else
    {
        return NULL;
    }
}

/**
 * @brief       获取ATK-IDE01 UART接收到的一帧数据的长度
 * @param       无
 * @retval      0   : 未接收到一帧数据
 *              其他: 接收到的一帧数据的长度
 */
rt_uint16_t atk_ide01_uart_rx_get_frame_len(void)
{
    if (g_uart_rx_frame.sta.finsh == 1)
    {
        return g_uart_rx_frame.sta.len;
    }
    else
    {
        return 0;
    }
}
/* USER CODE END 1 */
