/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
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
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <rtthread.h>
#include "usart.h"
#include "dma.h"
#include "spi.h"
#include "iwdg.h"
#include "./json_deal/jsmn.h"
#include "./json_deal/jsmn_util.h"
/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */
  /* 邮箱控制块 */
  struct rt_mailbox led_mb;
  /* 用于放邮件的内存池 */
  char led_mb_pool[64];

  struct msg
  {
    char *storey;     // 楼层
    char *parameters; // 控制参数
  };

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define ATK_NET_TR_Pin GPIO_PIN_6
#define ATK_NET_TR_GPIO_Port GPIOF
#define LED0_Pin GPIO_PIN_9
#define LED0_GPIO_Port GPIOF
#define LED1_Pin GPIO_PIN_10
#define LED1_GPIO_Port GPIOF
#define ATK_NET_DF_Pin GPIO_PIN_0
#define ATK_NET_DF_GPIO_Port GPIOC
#define USART3_TX_Pin GPIO_PIN_10
#define USART3_TX_GPIO_Port GPIOB
#define USART3_RX_Pin GPIO_PIN_11
#define USART3_RX_GPIO_Port GPIOB
#define F_CS_Pin GPIO_PIN_14
#define F_CS_GPIO_Port GPIOB
#define SPI1_SCK_Pin GPIO_PIN_3
#define SPI1_SCK_GPIO_Port GPIOB
#define SPI1_MISO_Pin GPIO_PIN_4
#define SPI1_MISO_GPIO_Port GPIOB
#define SPI1_MOSI_Pin GPIO_PIN_5
#define SPI1_MOSI_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */
#define SPI_FLASH 1
#define ATK_NET 1
/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
