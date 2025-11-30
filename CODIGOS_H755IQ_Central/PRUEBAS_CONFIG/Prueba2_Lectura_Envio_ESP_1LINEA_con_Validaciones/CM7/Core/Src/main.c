/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
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
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */


typedef struct {
    int nodo_id;
    float x, y, z;
    float temperatura;
    float humedad;
    int aqi;
    int tvoc;
    float eco2;
    int received;
} NodoData;

NodoData nodos[3] = {0};

uint32_t lastResetTime = 0;   // para timeout
const uint32_t timeoutMs = 1500; // 1.5 seg límite para promedios
char alertaNodo[3][50] = {"", "", ""}; // Guarda la última alerta detectada para cada nodo

void parseNodo(char *msg);
void procesarPromedio(void);
void verificarAlertasComunes(void);
void extractAlertType(char *input, char *output, size_t outputSize);

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* DUAL_CORE_BOOT_SYNC_SEQUENCE: Define for dual core boot synchronization    */
/*                             demonstration code based on hardware semaphore */
/* This define is present in both CM7/CM4 projects                            */
/* To comment when developping/debugging on a single core                     */
//#define DUAL_CORE_BOOT_SYNC_SEQUENCE

#if defined(DUAL_CORE_BOOT_SYNC_SEQUENCE)
#ifndef HSEM_ID_0
#define HSEM_ID_0 (0U) /* HW semaphore 0*/
#endif
#endif /* DUAL_CORE_BOOT_SYNC_SEQUENCE */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

UART_HandleTypeDef huart2;
UART_HandleTypeDef huart3;

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_USART3_UART_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */
/* USER CODE BEGIN Boot_Mode_Sequence_0 */
#if defined(DUAL_CORE_BOOT_SYNC_SEQUENCE)
  int32_t timeout;
#endif /* DUAL_CORE_BOOT_SYNC_SEQUENCE */
/* USER CODE END Boot_Mode_Sequence_0 */

/* USER CODE BEGIN Boot_Mode_Sequence_1 */
#if defined(DUAL_CORE_BOOT_SYNC_SEQUENCE)
  /* Wait until CPU2 boots and enters in stop mode or timeout*/
  timeout = 0xFFFF;
  while((__HAL_RCC_GET_FLAG(RCC_FLAG_D2CKRDY) != RESET) && (timeout-- > 0));
  if ( timeout < 0 )
  {
  Error_Handler();
  }
#endif /* DUAL_CORE_BOOT_SYNC_SEQUENCE */
/* USER CODE END Boot_Mode_Sequence_1 */
  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();
/* USER CODE BEGIN Boot_Mode_Sequence_2 */
#if defined(DUAL_CORE_BOOT_SYNC_SEQUENCE)
/* When system initialization is finished, Cortex-M7 will release Cortex-M4 by means of
HSEM notification */
/*HW semaphore Clock enable*/
__HAL_RCC_HSEM_CLK_ENABLE();
/*Take HSEM */
HAL_HSEM_FastTake(HSEM_ID_0);
/*Release HSEM in order to notify the CPU2(CM4)*/
HAL_HSEM_Release(HSEM_ID_0,0);
/* wait until CPU2 wakes up from stop mode */
timeout = 0xFFFF;
while((__HAL_RCC_GET_FLAG(RCC_FLAG_D2CKRDY) == RESET) && (timeout-- > 0));
if ( timeout < 0 )
{
Error_Handler();
}
#endif /* DUAL_CORE_BOOT_SYNC_SEQUENCE */
/* USER CODE END Boot_Mode_Sequence_2 */

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_USART2_UART_Init();
  MX_USART3_UART_Init();
  /* USER CODE BEGIN 2 */

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
	  static uint8_t buffer[300];
	  static uint16_t idx = 0;
	  uint8_t byte;

	  if (HAL_UART_Receive(&huart2, &byte, 1, 100) == HAL_OK)
	  {
		  buffer[idx++] = byte;

		  if (byte == '\n')
		  {
			  buffer[idx] = '\0'; // convertir en string

			  // 1️⃣ Mostrar lo recibido
//			  HAL_UART_Transmit(&huart3, buffer, idx, 1000);
//			  HAL_UART_Transmit(&huart3, (uint8_t*)"\r\n", 2, 100);

			  // 2️⃣ Procesar el nodo
			  parseNodo((char*)buffer);

			  // reiniciar buffer
			  idx = 0;
			  memset(buffer, 0, sizeof(buffer));

			  // 3️⃣ Intentar sacar promedios
			  procesarPromedio();
		  }
	  }
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Supply configuration update enable
  */
  HAL_PWREx_ConfigSupply(PWR_DIRECT_SMPS_SUPPLY);

  /** Configure the main internal regulator output voltage
  */
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE3);

  while(!__HAL_PWR_GET_FLAG(PWR_FLAG_VOSRDY)) {}

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_DIV1;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2
                              |RCC_CLOCKTYPE_D3PCLK1|RCC_CLOCKTYPE_D1PCLK1;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
  RCC_ClkInitStruct.SYSCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB3CLKDivider = RCC_APB3_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_APB1_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_APB2_DIV1;
  RCC_ClkInitStruct.APB4CLKDivider = RCC_APB4_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief USART2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART2_UART_Init(void)
{

  /* USER CODE BEGIN USART2_Init 0 */

  /* USER CODE END USART2_Init 0 */

  /* USER CODE BEGIN USART2_Init 1 */

  /* USER CODE END USART2_Init 1 */
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 115200;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  huart2.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart2.Init.ClockPrescaler = UART_PRESCALER_DIV1;
  huart2.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_SetTxFifoThreshold(&huart2, UART_TXFIFO_THRESHOLD_1_8) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_SetRxFifoThreshold(&huart2, UART_RXFIFO_THRESHOLD_1_8) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_DisableFifoMode(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART2_Init 2 */

  /* USER CODE END USART2_Init 2 */

}

/**
  * @brief USART3 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART3_UART_Init(void)
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
  huart3.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart3.Init.ClockPrescaler = UART_PRESCALER_DIV1;
  huart3.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart3) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_SetTxFifoThreshold(&huart3, UART_TXFIFO_THRESHOLD_1_8) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_SetRxFifoThreshold(&huart3, UART_RXFIFO_THRESHOLD_1_8) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_DisableFifoMode(&huart3) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART3_Init 2 */

  /* USER CODE END USART3_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOG_CLK_ENABLE();

  /*Configure GPIO pins : PC1 PC4 PC5 */
  GPIO_InitStruct.Pin = GPIO_PIN_1|GPIO_PIN_4|GPIO_PIN_5;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.Alternate = GPIO_AF11_ETH;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pins : PA1 PA2 PA7 */
  GPIO_InitStruct.Pin = GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_7;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.Alternate = GPIO_AF11_ETH;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pin : PB13 */
  GPIO_InitStruct.Pin = GPIO_PIN_13;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.Alternate = GPIO_AF11_ETH;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pins : PA8 PA11 PA12 */
  GPIO_InitStruct.Pin = GPIO_PIN_8|GPIO_PIN_11|GPIO_PIN_12;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.Alternate = GPIO_AF10_OTG1_FS;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pins : PG11 PG13 */
  GPIO_InitStruct.Pin = GPIO_PIN_11|GPIO_PIN_13;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.Alternate = GPIO_AF11_ETH;
  HAL_GPIO_Init(GPIOG, &GPIO_InitStruct);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

void procesarPromedio(void) {

    uint32_t ahora = HAL_GetTick();

    int countXY = 0;
    int countEco2 = 0;

    float sumXY = 0;
    float sumEco2 = 0;

    for (int i = 0; i < 3; i++) {
        if (nodos[i].received) {
            float mag = sqrtf(nodos[i].x * nodos[i].x + nodos[i].y * nodos[i].y);
            sumXY += mag;
            sumEco2 += nodos[i].eco2;

            countXY++;
            countEco2++;
        }
    }

    // si no se recibió nada
    if (countXY == 0) {
//     	HAL_UART_Transmit(&huart3, (uint8_t*)"⚠ No se recibieron nodos\n", 28, 100);
        return;
    }

    // si ya pasaron 1500 ms → enviar promedios aunque falten nodos
    if (ahora - lastResetTime > timeoutMs) {

        float avgXY = sumXY / countXY;
        float avgEco2 = sumEco2 / countEco2;

        // Imprimir cada nodo recibido con sus valores reales
        for (int i = 0; i < 3; i++) {
            if (nodos[i].received) {
                char out[300];
                sprintf(out,
                        "[ESP_NODO_%d]X:%.2f;Y:%.2f;Z:%.2f;Temp:%.2f;Hum:%.2f;AQI:%d;TVOC:%d;eCO2:%.2f;PromXY:%.2f;PromECO2:%.2f\n\r",
                        nodos[i].nodo_id,
                        nodos[i].x,
                        nodos[i].y,
                        nodos[i].z,
                        nodos[i].temperatura,
                        nodos[i].humedad,
                        nodos[i].aqi,
                        nodos[i].tvoc,
                        nodos[i].eco2,
                        avgXY,
                        avgEco2);

                HAL_UART_Transmit(&huart3, (uint8_t*)out, strlen(out), 100);
                HAL_UART_Transmit(&huart2, (uint8_t*)out, strlen(out), 100);
            }
        }

        // reiniciar
        for (int i = 0; i < 3; i++) nodos[i].received = 0;
        lastResetTime = ahora;
    }
}

void parseNodo(char *msg) {

    int nodoID = -1;
    char alertMsgBuffer[200]; // Buffer para la alerta

    if (strstr(msg, "[GENERAL]") != NULL) { //Alerta general

            char *pAlert = strstr(msg, "ALERT:");
            if (pAlert != NULL) {

                char *startMsg = pAlert + strlen("ALERT:");

                sprintf(alertMsgBuffer, "ALERTA_GENERAL:%s\n\r", startMsg);

                HAL_UART_Transmit(&huart3, (uint8_t*)alertMsgBuffer, strlen(alertMsgBuffer), 100);
                HAL_UART_Transmit(&huart2, (uint8_t*)alertMsgBuffer, strlen(alertMsgBuffer), 100);
            }

            return; // NO PROCESAR COMO NODO
    }

    if (strstr(msg, "[ESP_NODO_1]")) nodoID = 0;
    else if (strstr(msg, "[ESP_NODO_2]")) nodoID = 1;
    else if (strstr(msg, "[ESP_NODO_3]")) nodoID = 2;
    else return; // no coincide con ningún nodo

    char *pAlert = strstr(msg, "ALERT:");
	if (pAlert != NULL) {
		// Encontró el patrón ALERT:


		// Buscar el inicio del mensaje de alerta (después de "ALERT:")
		char *startMsg = pAlert + strlen("ALERT:");

		snprintf(alertaNodo[nodoID], 50, "%s", startMsg);// Guardar la alerta por nodo

		sprintf(alertMsgBuffer, "ALERTA NODO %d: %s\n\r", nodoID + 1, startMsg);

		// Enviar la alerta a la Consola (huart3)
		HAL_UART_Transmit(&huart3, (uint8_t*)alertMsgBuffer, strlen(alertMsgBuffer), 100);
		HAL_UART_Transmit(&huart2, (uint8_t*)alertMsgBuffer, strlen(alertMsgBuffer), 100);

		verificarAlertasComunes();
		return;
	}

    nodos[nodoID].nodo_id = nodoID + 1; // guardar número de nodo (1, 2 o 3)

    char *pX = strstr(msg, "X:");
    if (pX) nodos[nodoID].x = atof(pX + 2);

    char *pY = strstr(msg, "Y:");
    if (pY) nodos[nodoID].y = atof(pY + 2);

    char *pZ = strstr(msg, "Z:");
    if (pZ) nodos[nodoID].z = atof(pZ + 2);

    char *pT = strstr(msg, "Temp:");
    if (pT) nodos[nodoID].temperatura = atof(pT + 5);

    char *pH = strstr(msg, "Hum:");
    if (pH) nodos[nodoID].humedad = atof(pH + 4);

    char *pA = strstr(msg, "AQI:");
    if (pA) nodos[nodoID].aqi = atoi(pA + 4);

    char *pTV = strstr(msg, "TVOC:");
    if (pTV) nodos[nodoID].tvoc = atoi(pTV + 5);

    char *pE = strstr(msg, "eCO2:");
    if (pE) nodos[nodoID].eco2 = atof(pE + 5);

    nodos[nodoID].received = 1;
}

void extractAlertType(char *input, char *output, size_t outputSize) {
    char *parenPos = strchr(input, '(');

    if (parenPos != NULL) {
        // Hay paréntesis → copiar solo hasta antes del paréntesis
        size_t len = parenPos - input;

        // Limitar al tamaño del buffer de salida
        if (len >= outputSize) len = outputSize - 1;

        strncpy(output, input, len);
        output[len] = '\0';

        // Eliminar espacios al final
        while (len > 0 && output[len-1] == ' ') {
            output[--len] = '\0';
        }
    } else {
        // No hay paréntesis → copiar todo
        strncpy(output, input, outputSize - 1);
        output[outputSize - 1] = '\0';
    }
}


void verificarAlertasComunes(void) {

    // Validar si las 3 contienen algo
    if (strlen(alertaNodo[0]) == 0 ||
        strlen(alertaNodo[1]) == 0 ||
        strlen(alertaNodo[2]) == 0)
        return; // falta una alerta

    // 🔥 NUEVO: Extraer TIPOS de alerta (sin valores numéricos)
    char tipo1[50], tipo2[50], tipo3[50];
    extractAlertType(alertaNodo[0], tipo1, sizeof(tipo1));
    extractAlertType(alertaNodo[1], tipo2, sizeof(tipo2));
    extractAlertType(alertaNodo[2], tipo3, sizeof(tipo3));

    // Si los 3 TIPOS son iguales → alerta común
    if (strcmp(tipo1, tipo2) == 0 && strcmp(tipo2, tipo3) == 0)
    {
        char buffer[200];
        snprintf(buffer, sizeof(buffer),
                 "ALERTA COMÚN: %s\n\r",
                 tipo1); // Usar el tipo sin valores

        HAL_UART_Transmit(&huart3, (uint8_t*)buffer, strlen(buffer), 100);
        HAL_UART_Transmit(&huart2, (uint8_t*)buffer, strlen(buffer), 100);

        // limpiar alertas para evitar repetición
        alertaNodo[0][0] = '\0';
        alertaNodo[1][0] = '\0';
        alertaNodo[2][0] = '\0';
    }
}

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
