/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
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
#include "stm32f1xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

void HAL_TIM_MspPostInit(TIM_HandleTypeDef *htim);

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define FAN_POWER_Pin GPIO_PIN_0
#define FAN_POWER_GPIO_Port GPIOA
#define LCD_D7_Pin GPIO_PIN_1
#define LCD_D7_GPIO_Port GPIOA
#define LCD_D6_Pin GPIO_PIN_2
#define LCD_D6_GPIO_Port GPIOA
#define LCD_D5_Pin GPIO_PIN_3
#define LCD_D5_GPIO_Port GPIOA
#define LCD_D4_Pin GPIO_PIN_4
#define LCD_D4_GPIO_Port GPIOA
#define LCD_EN_Pin GPIO_PIN_5
#define LCD_EN_GPIO_Port GPIOA
#define LCD_RS_Pin GPIO_PIN_6
#define LCD_RS_GPIO_Port GPIOA
#define REED_SW_Pin GPIO_PIN_1
#define REED_SW_GPIO_Port GPIOB
#define BL_Pin GPIO_PIN_10
#define BL_GPIO_Port GPIOB
#define MAX_CS_Pin GPIO_PIN_12
#define MAX_CS_GPIO_Port GPIOB
#define MAX_SCK_Pin GPIO_PIN_13
#define MAX_SCK_GPIO_Port GPIOB
#define MAX_MISO_Pin GPIO_PIN_14
#define MAX_MISO_GPIO_Port GPIOB
#define AC_ZERO_Pin GPIO_PIN_8
#define AC_ZERO_GPIO_Port GPIOA
#define GUN_POWER_Pin GPIO_PIN_9
#define GUN_POWER_GPIO_Port GPIOA
#define AC_RELAY_Pin GPIO_PIN_11
#define AC_RELAY_GPIO_Port GPIOA
#define ENC_L_Pin GPIO_PIN_4
#define ENC_L_GPIO_Port GPIOB
#define ENC_R_Pin GPIO_PIN_5
#define ENC_R_GPIO_Port GPIOB
#define ENC_B_Pin GPIO_PIN_6
#define ENC_B_GPIO_Port GPIOB
#define BUZZER_Pin GPIO_PIN_7
#define BUZZER_GPIO_Port GPIOB
#define SCL_Pin GPIO_PIN_8
#define SCL_GPIO_Port GPIOB
#define SDA_Pin GPIO_PIN_9
#define SDA_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */
#define FW_VERSION	("1.00")
/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
