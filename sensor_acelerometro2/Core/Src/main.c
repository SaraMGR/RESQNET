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
#include "stdbool.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
#define BUFFER_MAX_USART 100

#define ADXL345_ADDR       0x53<<1  //Dirección I2C
#define REG_DEVID          0x00
#define REG_POWER_CTL      0x2D
#define REG_DATA_FORMAT    0x31
#define REG_BW_RATE        0x2C
#define REG_DATAX0         0x32
#define REG_DATAX1         0x33
#define REG_DATAY0         0x34
#define REG_DATAY1         0x35
#define REG_DATAZ0         0x36
#define REG_DATAZ1         0x37

#define REG_THRESH_ACT     0x24
#define REG_INT_ENABLE     0x2E
#define REG_INT_SOURCE     0x30


typedef struct{
	uint8_t bytes [BUFFER_MAX_USART];
	uint16_t longitud;
}Transfer_Config;

typedef struct{
	volatile float x_axis;
	volatile float y_axis;
	volatile float z_axis;
	volatile float acelX_filtrada;
	volatile float acelY_filtrada;
	volatile float acelZ_filtrada;
	volatile float deltaX;
	volatile float deltaY;
	volatile float deltaZ;
	volatile float prevX;
	volatile float prevY;

	bool startMeasure;
} Device_Config;

typedef struct{
	uint8_t ID;
	bool deviceReady;
	uint8_t rawData[8];
	int16_t rawX;
	int16_t rawY;
	int16_t rawZ;
} ADXL_Config;


typedef struct{
	uint8_t data_format;
	uint8_t power_ctl;
	uint8_t bw_rate;
} Values_ADXL;

Device_Config datos={};
ADXL_Config acelerometro={};
Values_ADXL values={};
Transfer_Config tx={};

void ADXL_Init(void);
void Value_Conversion(void);
void Send_USART(void);
float filtroSuavizado(float nuevo, float anterior);

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

I2C_HandleTypeDef hi2c1;

UART_HandleTypeDef huart2;
UART_HandleTypeDef huart3;

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_I2C1_Init(void);
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

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_I2C1_Init();
  MX_USART2_UART_Init();
  MX_USART3_UART_Init();
  /* USER CODE BEGIN 2 */

  datos.startMeasure=false;
  datos.x_axis=0;
  datos.y_axis=0;
  datos.z_axis=0;
  datos.acelX_filtrada=0;
  datos.acelY_filtrada=0;
  datos.acelZ_filtrada=0;
  datos.deltaX=0;
  datos.deltaY=0;
  datos.deltaZ=0;
  datos.prevX=0;
  datos.prevY=0;


  //confirmacion de que existe acelerometro
  HAL_I2C_Mem_Read(&hi2c1, ADXL345_ADDR, REG_DEVID, 1, &acelerometro.ID, 1, 100);
  if (acelerometro.ID==0xE5) {
	  acelerometro.deviceReady=true;
  } else{
	  acelerometro.deviceReady=false;
  }

  //configuracion de acelerometro
  if(acelerometro.deviceReady){
	  ADXL_Init();
  }

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
	  //obtencion de datos crudos
	    uint8_t buffer[2];

	    // Leer X
	    HAL_I2C_Mem_Read(&hi2c1, ADXL345_ADDR, REG_DATAX0, 1, buffer, 2, 100);
	    acelerometro.rawX = (buffer[1] << 8 | buffer[0]);

	    // Leer Y
	    HAL_I2C_Mem_Read(&hi2c1, ADXL345_ADDR, REG_DATAY0, 1, buffer, 2, 100);
	    acelerometro.rawY =(buffer[1] << 8 | buffer[0]);

	    //Leer Z
	    HAL_I2C_Mem_Read(&hi2c1, ADXL345_ADDR, REG_DATAZ0, 1, buffer, 2, 100);
	    acelerometro.rawZ = (buffer[1] << 8 | buffer[0]);

	    Value_Conversion();
	    Send_USART();

	    datos.acelX_filtrada = filtroSuavizado(datos.x_axis, datos.acelX_filtrada);
	    datos.acelY_filtrada = filtroSuavizado(datos.y_axis, datos.acelY_filtrada);
	    datos.acelZ_filtrada = filtroSuavizado(datos.z_axis, datos.acelZ_filtrada);


	    // Calcular deltas
	    datos.deltaX = datos.acelX_filtrada - datos.prevX;
	    datos.deltaY = datos.acelY_filtrada - datos.prevY;

	    datos.prevX = datos.acelX_filtrada;
	    datos.prevY = datos.acelY_filtrada;

	    // Magnitud lateral (evitar gravedad)
	    float magnitudLateral = sqrt(datos.deltaX*datos.deltaX + datos.deltaY*datos.deltaY);

	    // Detección
	    if (magnitudLateral > 0.01 && magnitudLateral < 0.03) {
	        snprintf((char*)tx.bytes, sizeof(tx.bytes), "Temblor leve detectado\r\n");
	        HAL_UART_Transmit(&huart3, tx.bytes, strlen((char*)tx.bytes), 1000);
	    }

	    else if (magnitudLateral >= 0.03 && magnitudLateral < 0.05) {
	    	        snprintf((char*)tx.bytes, sizeof(tx.bytes), "Temblor fuerte detectado\r\n");
	    	        HAL_UART_Transmit(&huart3, tx.bytes, strlen((char*)tx.bytes), 1000);
	    	    }

	    else if (magnitudLateral >= 0.5) {
	        snprintf((char*)tx.bytes, sizeof(tx.bytes), "Alerta\r\n");
	        HAL_UART_Transmit(&huart3, tx.bytes, strlen((char*)tx.bytes), 1000);
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

  /** Configure the main internal regulator output voltage
  */
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE3);

  while(!__HAL_PWR_GET_FLAG(PWR_FLAG_VOSRDY)) {}

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSIDiv = RCC_HSI_DIV1;
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
                              |RCC_CLOCKTYPE_PCLK3;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB3CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_3) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure the programming delay
  */
  __HAL_FLASH_SET_PROGRAM_DELAY(FLASH_PROGRAMMING_DELAY_1);
}

/**
  * @brief I2C1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2C1_Init(void)
{

  /* USER CODE BEGIN I2C1_Init 0 */

  /* USER CODE END I2C1_Init 0 */

  /* USER CODE BEGIN I2C1_Init 1 */

  /* USER CODE END I2C1_Init 1 */
  hi2c1.Instance = I2C1;
  hi2c1.Init.Timing = 0x10707DBC;
  hi2c1.Init.OwnAddress1 = 0;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.OwnAddress2Masks = I2C_OA2_NOMASK;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Analogue filter
  */
  if (HAL_I2CEx_ConfigAnalogFilter(&hi2c1, I2C_ANALOGFILTER_ENABLE) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Digital filter
  */
  if (HAL_I2CEx_ConfigDigitalFilter(&hi2c1, 0) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C1_Init 2 */

  /* USER CODE END I2C1_Init 2 */

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

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(LED_VERDE_GPIO_Port, LED_VERDE_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : BTN_Pin */
  GPIO_InitStruct.Pin = BTN_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(BTN_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : LED_VERDE_Pin */
  GPIO_InitStruct.Pin = LED_VERDE_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(LED_VERDE_GPIO_Port, &GPIO_InitStruct);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

//FUNCIONEEEEEEEEEESSS
void ADXL_Init(void){
	  values.bw_rate=0x0A; //100MHZ
	  values.data_format=0x00; //Rango +2g
	  values.power_ctl=0x08; //medicion activado

	  HAL_I2C_Mem_Write(&hi2c1, ADXL345_ADDR, REG_POWER_CTL, 1, &values.power_ctl, 1, 100);
	  HAL_I2C_Mem_Write(&hi2c1, ADXL345_ADDR, REG_DATA_FORMAT, 1, &values.data_format, 1, 100);
	  HAL_I2C_Mem_Write(&hi2c1, ADXL345_ADDR, REG_BW_RATE, 1, &values.bw_rate, 1, 100);
}

void Value_Conversion(void){
	datos.x_axis = acelerometro.rawX / 256.0;
	datos.y_axis = acelerometro.rawY / 256.0;
	datos.z_axis = acelerometro.rawZ / 256.0;
}

void Send_USART() {
    tx.longitud = (uint16_t)snprintf(
        (char*)tx.bytes,               // destino
        sizeof(tx.bytes),              // tamaño del buffer
        "X: %.2f  Y: %.2f  Z: %.2f\r\n", // formato con dos decimales
        (double)datos.x_axis,
        (double)datos.y_axis,
        (double)datos.z_axis
    );

    HAL_UART_Transmit(&huart3, tx.bytes, tx.longitud, 1000);
    HAL_Delay(150);
}

float filtroSuavizado(float nuevo, float anterior) {
    return 0.8 * anterior + 0.2 * nuevo;  // Filtro paso bajo
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
