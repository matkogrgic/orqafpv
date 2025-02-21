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
#include "core_cm4.h"
#include "i2c_lcd.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
#define NUM_SENSORS 4
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
I2C_HandleTypeDef hi2c1;

TIM_HandleTypeDef htim2;
TIM_HandleTypeDef htim3;

UART_HandleTypeDef huart3;

PCD_HandleTypeDef hpcd_USB_OTG_FS;

/* USER CODE BEGIN PV */

I2C_LCD_HandleTypeDef lcd1;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART3_UART_Init(void);
static void MX_USB_OTG_FS_PCD_Init(void);
static void MX_TIM3_Init(void);
static void MX_I2C1_Init(void);
static void MX_TIM2_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* ----- MULTI CHANNEL ECHO VARIABLES ----- */

volatile uint32_t echo_start[NUM_SENSORS] = {0};
volatile uint32_t echo_end[NUM_SENSORS] = {0};
volatile uint8_t  echo_captured[NUM_SENSORS] = {0};

/* ---------------------------------------- */
/* ----- BOOL/STATE VARIABLES ----- */

volatile uint8_t isRunning = 0;
volatile uint8_t previousState = 0;
volatile uint8_t start_stop_stay = 0;
		 uint8_t ledId = 0;
		 int second = 1000;

/* -------------------------------- */
/* ----- TIMER/LONG VARIABLES ----- */

volatile uint32_t stopwatch_count = 0;
volatile uint32_t currentMillis, previousMillis = 0;
		 uint32_t ignoreStateUntil = 0;
		 uint32_t distances[NUM_SENSORS] = {0};
		 uint32_t stopwatchMillis = 0;

/* -------------------------------- */
/* ----- USED FUNCTIONS ----- */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim);
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin);
void HAL_TIM_IC_CaptureCallback(TIM_HandleTypeDef *htim);
void start_stopwatch();
void stop_stopwatch();
void delay_ms(uint32_t ms);
void delay_us(uint32_t us);
void send_time_to_outputs();
void toggleLED(uint8_t index, uint8_t v);
void resetLEDs();
void readSensor();
float getDistanceInCm(uint8_t ch_index);
void checkForDrone();

/* -------------------------- */
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
  MX_USART3_UART_Init();
  MX_USB_OTG_FS_PCD_Init();
  MX_TIM3_Init();
  MX_I2C1_Init();
  MX_TIM2_Init();
  /* USER CODE BEGIN 2 */

  // LCD initialization
	  lcd1.hi2c = &hi2c1;
	  lcd1.address = 0x4E;
	  lcd_init(&lcd1);
  // CONTENT ON LCD
	  lcd_clear(&lcd1);

	  HAL_TIM_Base_Start_IT(&htim3);
	  HAL_TIM_IC_Start_IT(&htim2, TIM_CHANNEL_1);
	  HAL_TIM_IC_Start_IT(&htim2, TIM_CHANNEL_2);
	  HAL_TIM_IC_Start_IT(&htim2, TIM_CHANNEL_3);
	  HAL_TIM_IC_Start_IT(&htim2, TIM_CHANNEL_4);
	  send_time_to_outputs();

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
	  checkForDrone();
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
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 4;
  RCC_OscInitStruct.PLL.PLLN = 168;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 7;
  RCC_OscInitStruct.PLL.PLLR = 2;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
  {
    Error_Handler();
  }
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
  hi2c1.Init.ClockSpeed = 400000;
  hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
  hi2c1.Init.OwnAddress1 = 0;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C1_Init 2 */

  /* USER CODE END I2C1_Init 2 */

}

/**
  * @brief TIM2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM2_Init(void)
{

  /* USER CODE BEGIN TIM2_Init 0 */

  /* USER CODE END TIM2_Init 0 */

  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_IC_InitTypeDef sConfigIC = {0};

  /* USER CODE BEGIN TIM2_Init 1 */

  /* USER CODE END TIM2_Init 1 */
  htim2.Instance = TIM2;
  htim2.Init.Prescaler = 83;
  htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim2.Init.Period = 4294967295;
  htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_IC_Init(&htim2) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigIC.ICPolarity = TIM_INPUTCHANNELPOLARITY_BOTHEDGE;
  sConfigIC.ICSelection = TIM_ICSELECTION_DIRECTTI;
  sConfigIC.ICPrescaler = TIM_ICPSC_DIV1;
  sConfigIC.ICFilter = 0;
  if (HAL_TIM_IC_ConfigChannel(&htim2, &sConfigIC, TIM_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_IC_ConfigChannel(&htim2, &sConfigIC, TIM_CHANNEL_2) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_IC_ConfigChannel(&htim2, &sConfigIC, TIM_CHANNEL_3) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_IC_ConfigChannel(&htim2, &sConfigIC, TIM_CHANNEL_4) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM2_Init 2 */
  HAL_NVIC_SetPriority(TIM2_IRQn, 2, 0);
  HAL_NVIC_EnableIRQ(TIM2_IRQn);
  /* USER CODE END TIM2_Init 2 */

}

/**
  * @brief TIM3 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM3_Init(void)
{

  /* USER CODE BEGIN TIM3_Init 0 */

  /* USER CODE END TIM3_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM3_Init 1 */

  /* USER CODE END TIM3_Init 1 */
  htim3.Instance = TIM3;
  htim3.Init.Prescaler = 83;
  htim3.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim3.Init.Period = 7518;
  htim3.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim3.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim3) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim3, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim3, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM3_Init 2 */
  HAL_NVIC_SetPriority(TIM3_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(TIM3_IRQn);
  /* USER CODE END TIM3_Init 2 */

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
  if (HAL_UART_Init(&huart3) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART3_Init 2 */

  /* USER CODE END USART3_Init 2 */

}

/**
  * @brief USB_OTG_FS Initialization Function
  * @param None
  * @retval None
  */
static void MX_USB_OTG_FS_PCD_Init(void)
{

  /* USER CODE BEGIN USB_OTG_FS_Init 0 */

  /* USER CODE END USB_OTG_FS_Init 0 */

  /* USER CODE BEGIN USB_OTG_FS_Init 1 */

  /* USER CODE END USB_OTG_FS_Init 1 */
  hpcd_USB_OTG_FS.Instance = USB_OTG_FS;
  hpcd_USB_OTG_FS.Init.dev_endpoints = 6;
  hpcd_USB_OTG_FS.Init.speed = PCD_SPEED_FULL;
  hpcd_USB_OTG_FS.Init.dma_enable = DISABLE;
  hpcd_USB_OTG_FS.Init.phy_itface = PCD_PHY_EMBEDDED;
  hpcd_USB_OTG_FS.Init.Sof_enable = ENABLE;
  hpcd_USB_OTG_FS.Init.low_power_enable = DISABLE;
  hpcd_USB_OTG_FS.Init.lpm_enable = DISABLE;
  hpcd_USB_OTG_FS.Init.vbus_sensing_enable = ENABLE;
  hpcd_USB_OTG_FS.Init.use_dedicated_ep1 = DISABLE;
  if (HAL_PCD_Init(&hpcd_USB_OTG_FS) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USB_OTG_FS_Init 2 */

  /* USER CODE END USB_OTG_FS_Init 2 */

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
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOG_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_6, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, LD1_Pin|LD3_Pin|LD2_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOG, GPIO_PIN_0|GPIO_PIN_1|USB_PowerSwitchOn_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_8, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOD, GPIO_PIN_5, GPIO_PIN_RESET);

  /*Configure GPIO pin : USER_Btn_Pin */
  GPIO_InitStruct.Pin = USER_Btn_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(USER_Btn_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : PA6 */
  GPIO_InitStruct.Pin = GPIO_PIN_6;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pins : LD1_Pin LD3_Pin LD2_Pin */
  GPIO_InitStruct.Pin = LD1_Pin|LD3_Pin|LD2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pins : PG0 PG1 USB_PowerSwitchOn_Pin */
  GPIO_InitStruct.Pin = GPIO_PIN_0|GPIO_PIN_1|USB_PowerSwitchOn_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOG, &GPIO_InitStruct);

  /*Configure GPIO pin : USB_OverCurrent_Pin */
  GPIO_InitStruct.Pin = USB_OverCurrent_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(USB_OverCurrent_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : PC8 */
  GPIO_InitStruct.Pin = GPIO_PIN_8;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pin : PD0 */
  GPIO_InitStruct.Pin = GPIO_PIN_0;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

  /*Configure GPIO pin : PD5 */
  GPIO_InitStruct.Pin = GPIO_PIN_5;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

  /* EXTI interrupt init*/
  HAL_NVIC_SetPriority(EXTI0_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI0_IRQn);

/* USER CODE BEGIN MX_GPIO_Init_2 */
/* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

//Real-time stopwatch (100Hz)
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) {
    if (htim->Instance == TIM3 && isRunning) {
    	__HAL_TIM_CLEAR_IT(htim, TIM_IT_UPDATE);
           stopwatch_count++;
           send_time_to_outputs();
        }
}

//Button interrupt for manual start/stop
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {

	currentMillis = HAL_GetTick();
    if (GPIO_Pin == GPIO_PIN_0 && (currentMillis - previousMillis > 200)) {

        if(isRunning){	stop_stopwatch();  start_stop_stay = 2; send_time_to_outputs(); }
        else		 {	 lcd_clear(&lcd1); start_stopwatch(); start_stop_stay = 1; send_time_to_outputs();}

        previousMillis = currentMillis;
    }
}

//ECHO capture for all channels
void HAL_TIM_IC_CaptureCallback(TIM_HandleTypeDef *htim) {
	if (htim->Instance == TIM2){
		uint8_t ch_index = 0;
		uint32_t ch_value = 0;
		// Determine which channel triggered the interrupt
		switch (htim->Channel) {
			case HAL_TIM_ACTIVE_CHANNEL_1: ch_index = 0; ch_value = TIM_CHANNEL_1; break;
			case HAL_TIM_ACTIVE_CHANNEL_2: ch_index = 1; ch_value = TIM_CHANNEL_2; break;
			case HAL_TIM_ACTIVE_CHANNEL_3: ch_index = 2; ch_value = TIM_CHANNEL_3; break;
			case HAL_TIM_ACTIVE_CHANNEL_4: ch_index = 3; ch_value = TIM_CHANNEL_4; break;
			default: return;
		}

		 uint32_t captured_value = HAL_TIM_ReadCapturedValue(htim, ch_value);

		// Capture start or end of pulse
		if (echo_captured[ch_index] == 0) {
			echo_start[ch_index] = captured_value;
			echo_captured[ch_index] = 1;
		} else if (echo_captured[ch_index] == 1) {
			echo_end[ch_index] = captured_value;
			echo_captured[ch_index] = 2;  // Capture complete
		}
	}
}

//Start/Reset stopwatch
void start_stopwatch() {
	isRunning = 1;
    stopwatch_count = 0;

}

//Stop and show stopwatch
void stop_stopwatch() {
    isRunning = 0;
    send_time_to_outputs();
}

//Non-blocking delay in ms
void delay_ms(uint32_t ms) {
    uint32_t start = HAL_GetTick();
    while (HAL_GetTick() - start < ms) {

    }
}

//Non-blocking delay in us
void delay_us(uint32_t us) {
    // Enable the DWT cycle counter
    if (!(DWT->CTRL & DWT_CTRL_CYCCNTENA_Msk)) {
        DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
    }

    uint32_t start = DWT->CYCCNT;  // Read current cycle count
    uint32_t target = us * (SystemCoreClock / 1000000);  // Convert microseconds to cycles

    while ((DWT->CYCCNT - start) < target);  // Wait until the required cycles pass
}

//Sending stopwatch data to LCD and UART (app)
void send_time_to_outputs() {

    char buffer[10];  // Enough space for "999.99\0"

    lcd_gotoxy(&lcd1, 0, 0);
    lcd_puts(&lcd1, "Time: ");
    snprintf(buffer, sizeof(buffer), "%lu.%02lu", stopwatch_count / 100, stopwatch_count % 100);
    lcd_puts(&lcd1, buffer);
    snprintf(buffer, sizeof(buffer), "%lu.%02lu\n", stopwatch_count / 100, stopwatch_count % 100);
    HAL_UART_Transmit(&huart3, (uint8_t*)buffer, strlen(buffer), HAL_MAX_DELAY);


}


//Toggle state for indexed LED
void toggleLED(uint8_t index, uint8_t v){
	if(index < 0 || index > NUM_SENSORS) return;
	else{
		switch(index){
		    	case 0:
		    		HAL_GPIO_WritePin(GPIOC, GPIO_PIN_8, v);
		    		break;
		    	case 1:
		    		HAL_GPIO_WritePin(GPIOD, GPIO_PIN_5, v);
		    		break;
		    	case 2:
		    		HAL_GPIO_WritePin(GPIOG, GPIO_PIN_1, v);
		    		break;
		       	case 3:
					HAL_GPIO_WritePin(GPIOG, GPIO_PIN_0, v);
					break;
		    	default: return;
		}
	}
}

//Turn Off all LEDs
void resetLEDs(){
	HAL_GPIO_WritePin(GPIOC, GPIO_PIN_8, 0);
	HAL_GPIO_WritePin(GPIOD, GPIO_PIN_5, 0);
	HAL_GPIO_WritePin(GPIOG, GPIO_PIN_1, 0);
	HAL_GPIO_WritePin(GPIOG, GPIO_PIN_0, 0);
}

//Send trigger impuls to each sensor
void readSensor() {

		for (int i = 0; i < NUM_SENSORS; i++){
		    	echo_captured[i] = 0;
		    	echo_end[i] = 0;
		    	echo_start[i] = 0;
		 }

		HAL_GPIO_WritePin(GPIOA, GPIO_PIN_6, 1);
		delay_us(10);								// Delay 10us, as per datasheet
		HAL_GPIO_WritePin(GPIOA, GPIO_PIN_6, 0);

	    __HAL_TIM_SET_COUNTER(&htim3, 0);

}

//Convert pulse_width to distance(cm)
float getDistanceInCm(uint8_t ch_index) {
    if (echo_captured[ch_index] == 2) {
        uint32_t pulse_width;

        if (echo_end[ch_index] >= echo_start[ch_index]) {
            pulse_width = echo_end[ch_index] - echo_start[ch_index];
        } else {
            // Timer overflow case -> ARR+1 = 2^32+1
            pulse_width = 0xFFFFFFFF - echo_start[ch_index] + echo_end[ch_index];
        }
        distances[ch_index] = pulse_width / 58.0;
        return distances[ch_index]; // Convert to cm
    }
    return -1;
}

//Check each sensor for value < 30cm - drone detection
void checkForDrone() {
    readSensor();
    delay_ms(20);
    float distance[NUM_SENSORS] = {0};
    uint8_t currentState = 0;
    currentMillis = HAL_GetTick();

    for (int i = NUM_SENSORS - 1; i >= 0; i--) {
        distance[i] = getDistanceInCm(i);
        if (distance[i] > 0 && distance[i] < 30) {
            currentState = 1;
            ledId = i;
            break;  // Stop checking as soon as a drone is detected
        }
    }

    // If we are within the ignore period, exit early without updating state
    if(currentMillis - previousMillis < 2*second){

    	return;
    }

    resetLEDs();

    if(currentState != previousState){
		if (previousState == 1 && currentState == 0 && start_stop_stay == 0) {
			if (!isRunning ) {
				lcd_clear(&lcd1);
				start_stopwatch();
				start_stop_stay++;
				previousMillis = currentMillis;
				toggleLED(ledId, 1);

			}
		}

		else if (previousState == 0 && currentState == 1 && start_stop_stay == 1) {
			if (isRunning ) {
				stop_stopwatch();
				start_stop_stay++;
				send_time_to_outputs();				//Updating LCD output now, then delaying readings
				previousMillis = currentMillis;
				toggleLED(ledId, 1);
			}
		}

		else if(previousState == 1 && currentState == 0 && start_stop_stay == 2){
			send_time_to_outputs();
			start_stop_stay++;
			}

		else if(previousState == 0 && currentState == 1 && start_stop_stay == 3){
				send_time_to_outputs();
				start_stop_stay=0;

				}
    }
    previousState = currentState;

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

#ifdef  USE_FULL_ASSERT
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
