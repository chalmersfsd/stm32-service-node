/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  ** This notice applies to any and all portions of this file
  * that are not between comment pairs USER CODE BEGIN and
  * USER CODE END. Other portions of this file, whether 
  * inserted by the user or by software development tools
  * are owned by their respective copyright owners.
  *
  * COPYRIGHT(c) 2018 STMicroelectronics
  *
  * Redistribution and use in source and binary forms, with or without modification,
  * are permitted provided that the following conditions are met:
  *   1. Redistributions of source code must retain the above copyright notice,
  *      this list of conditions and the following disclaimer.
  *   2. Redistributions in binary form must reproduce the above copyright notice,
  *      this list of conditions and the following disclaimer in the documentation
  *      and/or other materials provided with the distribution.
  *   3. Neither the name of STMicroelectronics nor the names of its contributors
  *      may be used to endorse or promote products derived from this software
  *      without specific prior written permission.
  *
  * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
  * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
  * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
  * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
  * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
  * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
  * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
  * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
  *
  ******************************************************************************
  */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H__
#define __MAIN_H__

/* Includes ------------------------------------------------------------------*/

/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Private define ------------------------------------------------------------*/

#define PWMBrake_Pin GPIO_PIN_5
#define PWMBrake_GPIO_Port GPIOE
#define PWMAssiGreen_Pin GPIO_PIN_5
#define PWMAssiGreen_GPIO_Port GPIOA
#define PWMAssiRed_Pin GPIO_PIN_6
#define PWMAssiRed_GPIO_Port GPIOA
#define PWMSteer_Pin GPIO_PIN_9
#define PWMSteer_GPIO_Port GPIOE
#define PWMAssiBlue_Pin GPIO_PIN_12
#define PWMAssiBlue_GPIO_Port GPIOD
#define ServiceTank_Pin GPIO_PIN_10
#define ServiceTank_GPIO_Port GPIOA
#define EbsLine_Pin GPIO_PIN_11
#define EbsLine_GPIO_Port GPIOA
#define EbsActuator_Pin GPIO_PIN_12
#define EbsActuator_GPIO_Port GPIOA
#define SteerPosition_Pin GPIO_PIN_13
#define SteerPosition_GPIO_Port GPIOA
#define RackPostition_Pin GPIO_PIN_14
#define RackPostition_GPIO_Port GPIOA
#define PressureReg_Pin GPIO_PIN_15
#define PressureReg_GPIO_Port GPIOA
#define HeartBeat_Pin GPIO_PIN_0
#define HeartBeat_GPIO_Port GPIOD
#define EbsSpeaker_Pin GPIO_PIN_1
#define EbsSpeaker_GPIO_Port GPIOD
#define Compressor_Pin GPIO_PIN_2
#define Compressor_GPIO_Port GPIOD
#define SteeringRight_Pin GPIO_PIN_3
#define SteeringRight_GPIO_Port GPIOD
#define EbsRelief_Pin GPIO_PIN_4
#define EbsRelief_GPIO_Port GPIOD
#define Clamp_Pin GPIO_PIN_5
#define Clamp_GPIO_Port GPIOD
#define Finished_Pin GPIO_PIN_6
#define Finished_GPIO_Port GPIOD
#define Shutdown_Pin GPIO_PIN_7
#define Shutdown_GPIO_Port GPIOD
#define ServiceBreak_Pin GPIO_PIN_3
#define ServiceBreak_GPIO_Port GPIOB
#define ASMS_Pin GPIO_PIN_4
#define ASMS_GPIO_Port GPIOB
#define EbsOk_Pin GPIO_PIN_5
#define EbsOk_GPIO_Port GPIOB
#define ClampedSensor_Pin GPIO_PIN_6
#define ClampedSensor_GPIO_Port GPIOB

/* ########################## Assert Selection ############################## */
/**
  * @brief Uncomment the line below to expanse the "assert_param" macro in the 
  *        HAL drivers code
  */
/* #define USE_FULL_ASSERT    1U */

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
 extern "C" {
#endif
void _Error_Handler(char *, int);

#define Error_Handler() _Error_Handler(__FILE__, __LINE__)
#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H__ */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
