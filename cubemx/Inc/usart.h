/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    usart.h
  * @brief   This file contains all the function prototypes for
  *          the usart.c file
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
/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __USART_H__
#define __USART_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

extern UART_HandleTypeDef huart3;

/* USER CODE BEGIN Private defines */
#define USART3_TCPNET 1

#define ATK_IDE01_UART_RX_BUF_SIZE          64
#define ATK_IDE01_UART_TX_BUF_SIZE          64
/* USER CODE END Private defines */

void MX_USART3_UART_Init(void);

/* USER CODE BEGIN Prototypes */
void atk_ide01_uart_printf(char *fmt, ...);     /* ATK-IDE01 UART printf */
void atk_ide01_uart_rx_restart(void);           /* ATK-IDE01 UART重新开始接收数据 */
rt_uint8_t *atk_ide01_uart_rx_get_frame(void);     /* 获取ATK-IDE01 UART接收到的一帧数据 */
rt_uint16_t atk_ide01_uart_rx_get_frame_len(void); /* 获取ATK-IDE01 UART接收到的一帧数据的长度 */
/* USER CODE END Prototypes */

#ifdef __cplusplus
}
#endif

#endif /* __USART_H__ */

