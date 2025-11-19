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
#include "stdio.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
#define BUFFER_MAX_USART 100

//---------------------ACELEROMETRO-----------------------------------------
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


//----------------------------CALIDAD DE AIRE--------------------------------
#define AHT21_ADDR    (0x38 << 1)

#define ENS160_ADDR_52   (0x52 << 1)
#define ENS160_ADDR_53   (0x53 << 1)

/* ENS160 registros útiles */
#define ENS160_PART_ID       0x00
#define ENS160_OPMODE        0x10
#define ENS160_CONFIG        0x11
#define ENS160_COMMAND       0x12
#define ENS160_TEMP_IN       0x13
#define ENS160_RH_IN         0x15
#define ENS160_DEVICE_STATUS 0x20
#define ENS160_DATA_AQI      0x21
#define ENS160_DATA_TVOC     0x22
#define ENS160_DATA_ECO2     0x24



typedef struct{
	bool ens160_found;
	uint8_t ID_LSB;
	uint8_t ID_MSB;
	uint16_t ID1;
	bool deviceReady1;
	bool dataReady1;
	uint8_t config_value;
	uint8_t command_value;
	uint8_t opmode_value;
	uint8_t status;
	uint8_t aqi;
	uint16_t tvoc;
	uint16_t eco2;
} ENS160_Config;

typedef struct{
	bool aht21_found;
	uint8_t status;
	bool deviceReady1;
	float temperature;
	float humidity;
} AHT21_Config;


ENS160_Config ens160 = {};
AHT21_Config aht21 = {};

void ENS160_Init(void);
void ENS160_ConfigReg(void);
void AHT21_Init(void);
bool AHT21_ReadData(float *temperature, float *humidity);
void ENS160_SetTempHum(float *temperature, float *humidity);
void Read_ENS160_Data(void);
void Send_USART(void);
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

  //------------ACELEROMETRO--------------
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

  HAL_I2C_Mem_Read(&hi2c1, ADXL345_ADDR, REG_DEVID, 1, &acelerometro.ID, 1, 100);
  if (acelerometro.ID==0xE5) {
	  acelerometro.deviceReady=true;
  } else{
	  acelerometro.deviceReady=false;
  }

  if(acelerometro.deviceReady){
	  ADXL_Init(); //configuracion de acelerometro
  }

  //----------------CALIDAD AIRE----------------
  ENS160_Init();
  if (ens160.deviceReady1){
	ENS160_ConfigReg();
  }

  AHT21_Init();


  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
	  //------------ACEELEROMETRO----------------
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


	  //------------CALIDAD DE AIRE--------------
	  if(AHT21_ReadData(&aht21.temperature, &aht21.humidity)){
	  		  ENS160_SetTempHum(&aht21.temperature, &aht21.humidity); // ENS160 --> compensación ambiental
	  	  }

	  	  HAL_I2C_Mem_Read(&hi2c1, ENS160_ADDR_52, ENS160_DEVICE_STATUS, 1, &ens160.status, 1, 100);
	  	  bool opmode_running = ens160.status & (1 << 7);
	  	  bool error_detected = ens160.status & (1 << 6);
	  	  uint8_t validity_flag = (ens160.status >> 2) & 0x03;
	  	  bool new_data = ens160.status & (1 << 1);

	  	  // Acepta datos si NO hay error y NO es output inválido (flag 3)
	  	  if(opmode_running && !error_detected && validity_flag != 3 && new_data){
	  		  ens160.dataReady1 = true;

	  		  // Opcional: indicar que los datos aún no son 100% precisos
	  //		  ens160.isCalibrated = (validity_flag == 0);
	  	  } else {
	  		  ens160.dataReady1 = false;
	  	  }

	  	  if(ens160.dataReady1){
	  		  Read_ENS160_Data();
	  		  Send_USART(); // Enviar datos por UART
	  		  ens160.dataReady1 = false; // Opcional: limpiar bandera para esperar próxima lectura

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

	//-------------------ACELEROMETRO----------------
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


	float filtroSuavizado(float nuevo, float anterior) {
		return 0.8 * anterior + 0.2 * nuevo;  // Filtro paso bajo
	}

	//-------------------CALIDAD AIRE----------------
	void ENS160_Init(void){
		HAL_I2C_Mem_Read(&hi2c1, ENS160_ADDR_52, ENS160_PART_ID, 1, &ens160.ID_MSB, 1, 100);
		HAL_I2C_Mem_Read(&hi2c1, ENS160_ADDR_52, ENS160_PART_ID + 1, 1, &ens160.ID_LSB, 1, 100);

		ens160.ID1 = (ens160.ID_MSB << 8) | ens160.ID_LSB;

		if (ens160.ID1 == 0x6001){
			ens160.deviceReady1 = true;
		} else {
			ens160.deviceReady1 = false;
		}
	}

	void ENS160_ConfigReg(void){
		ens160.config_value = 0x00;		// CONFIG: interrupción desactivada (bit 0 = 0)
		ens160.command_value = 0xCC; 	// Limpiar registros GPR antes de iniciar
		ens160.opmode_value = 0x02;  	// 0x02 = Active gas sensing operating mode

		HAL_I2C_Mem_Write(&hi2c1, ENS160_ADDR_52, ENS160_CONFIG, 1, &ens160.config_value, 1, 100);
		HAL_I2C_Mem_Write(&hi2c1, ENS160_ADDR_52, ENS160_COMMAND, 1, &ens160.command_value, 1, 100);
		HAL_I2C_Mem_Write(&hi2c1, ENS160_ADDR_52, ENS160_OPMODE, 1, &ens160.opmode_value, 1, 100);
	}

	void AHT21_Init(void) {
		HAL_Delay(100);  // Esperar ≥100 ms tras encendido
		uint8_t command = 0x71; // Comando para leer status word

		HAL_I2C_Master_Transmit(&hi2c1, AHT21_ADDR, &command, 1, 100);
		HAL_I2C_Master_Receive(&hi2c1, AHT21_ADDR, &aht21.status, 1, 100);

		if (aht21.status == 0x18) {
			aht21.deviceReady1 = true;
		} else {
			aht21.deviceReady1 = false; // no calibrado o mal inicializado
		}
	}

	bool AHT21_ReadData(float *temperature, float *humidity) {
		uint8_t command_trigger[3] = {0xAC, 0x33, 0x00};
		uint8_t command_status = 0x71;
		uint8_t status;
		uint8_t data[6];


		// Iniciar medición
		HAL_I2C_Master_Transmit(&hi2c1, AHT21_ADDR, command_trigger, 3, 100);

		HAL_Delay(80);  // Esperar que termine la medición (~80 ms)

		// Leer estado (0x71)
		HAL_I2C_Master_Transmit(&hi2c1, AHT21_ADDR, &command_status, 1, 100);
		HAL_I2C_Master_Receive(&hi2c1, AHT21_ADDR, &status, 1, 100);

		if (status & 0x80) return false;  // sigue ocupado

		// Leer 6 bytes de datos
		HAL_I2C_Master_Receive(&hi2c1, AHT21_ADDR, data, 6, 100);

		// Convertir los datos
		uint32_t raw_humidity = ((uint32_t)data[1] << 12) | ((uint32_t)data[2] << 4) | (data[3] >> 4);
		uint32_t raw_temp     = (((uint32_t)data[3] & 0x0F) << 16) | ((uint32_t)data[4] << 8) | data[5];

		*humidity = ((float)raw_humidity / 1048576.0f) * 100.0f; // 2^20 = 1048576
		*temperature = ((float)raw_temp / 1048576.0f) * 200.0f - 50.0f;

		return true;
	}

	void ENS160_SetTempHum(float *temperature, float *humidity){
		uint16_t temp_kelvin = (uint16_t)((*temperature + 273.15f) * 64); //  DATA_T storage format
		uint16_t rh_scaled   = (uint16_t)(*humidity * 512); // DATA_RH storage format

		uint8_t temp_lsb = temp_kelvin & 0xFF;
		uint8_t temp_msb = temp_kelvin >> 8;
		uint8_t rh_lsb   = rh_scaled & 0xFF;
		uint8_t rh_msb   = rh_scaled >> 8;

		// Enviar al ENS160
		HAL_I2C_Mem_Write(&hi2c1, ENS160_ADDR_52, ENS160_TEMP_IN, 1, &temp_lsb, 1, 100);
		HAL_I2C_Mem_Write(&hi2c1, ENS160_ADDR_52, ENS160_TEMP_IN + 1, 1, &temp_msb, 1, 100);

		HAL_I2C_Mem_Write(&hi2c1, ENS160_ADDR_52, ENS160_RH_IN, 1, &rh_lsb, 1, 100);
		HAL_I2C_Mem_Write(&hi2c1, ENS160_ADDR_52, ENS160_RH_IN + 1, 1, &rh_msb, 1, 100);
	}

	void Read_ENS160_Data(void){
		uint8_t data_aqi_temp;
		uint8_t data_tvoc_lsb;
		uint8_t data_tvoc_msb;
		uint8_t data_eco2_lsb;
		uint8_t data_eco2_msb;

		// Leer AQI
		HAL_I2C_Mem_Read(&hi2c1, ENS160_ADDR_52, ENS160_DATA_AQI, 1, &data_aqi_temp, 1, 100);

		// Leer TVOC (2 bytes)
		HAL_I2C_Mem_Read(&hi2c1, ENS160_ADDR_52, ENS160_DATA_TVOC, 1, &data_tvoc_lsb, 1, 100);
		HAL_I2C_Mem_Read(&hi2c1, ENS160_ADDR_52, ENS160_DATA_TVOC + 1, 1, &data_tvoc_msb, 1, 100);

		// Leer eCO2 (2 bytes)
		HAL_I2C_Mem_Read(&hi2c1, ENS160_ADDR_52, ENS160_DATA_ECO2, 1, &data_eco2_lsb, 1, 100);
		HAL_I2C_Mem_Read(&hi2c1, ENS160_ADDR_52, ENS160_DATA_ECO2 + 1, 1, &data_eco2_msb, 1, 100);

		// Guardar los datos en la estructura
		ens160.aqi  = data_aqi_temp & 0x07;  // bits 0–2 son el AQI (1–5)
		ens160.tvoc = ((uint16_t)data_tvoc_msb << 8) | data_tvoc_lsb; // TVOC (ppb)
		ens160.eco2 = ((uint16_t)data_eco2_msb << 8) | data_eco2_lsb; // eCO2 (ppm)
	}


	void Send_USART() { //para acelerometro y calidad de aire
	    tx.longitud = (uint16_t)snprintf(
	        (char*)tx.bytes,
	        sizeof(tx.bytes),
	        "X: %.2f  Y: %.2f  Z: %.2f\r\n"
	        "Temp: %.2f C, Hum: %.2f %%\r\n"
	        "AQI: %d, TVOC: %d ppb, eCO2: %d ppm\r\n",
	        datos.x_axis,
	        datos.y_axis,
	        datos.z_axis,
	        aht21.temperature,
	        aht21.humidity,
	        ens160.aqi,
	        ens160.tvoc,
	        ens160.eco2
	    );

	    HAL_UART_Transmit(&huart3, tx.bytes, tx.longitud, 1000);

	    tx.longitud = (uint16_t)snprintf(
			(char*)tx.bytes,
			sizeof(tx.bytes),
			"X: %.2f  Y: %.2f  Z: %.2f\r\n"
			"Temp: %.2f C, Hum: %.2f %%\r\n"
			"AQI: %d, TVOC: %d ppb, eCO2: %d ppm\r\n",
			datos.x_axis,
			datos.y_axis,
			datos.z_axis,
			aht21.temperature,
			aht21.humidity,
			ens160.aqi,
			ens160.tvoc,
			ens160.eco2
		);

		HAL_UART_Transmit(&huart2, tx.bytes, tx.longitud, 1000);
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
