#include "main.h"

/* Private variables ---------------------------------------------------------*/
TIM_HandleTypeDef htim2;
TIM_HandleTypeDef htim3;

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_TIM2_Init(void);
static void MX_TIM3_Init(void);
void HAL_TIM_MspPostInit(TIM_HandleTypeDef *htim);

/* Master Speed Control Function (0 = Stopped, 999 = Max Speed) -------------*/
void Set_Motor_Speed(uint16_t left_speed, uint16_t right_speed) {
    __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_3, left_speed);  // Controls ENA (D6)
    __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_2, right_speed); // Controls ENB (D9)
}

/* Robot Movement Functions --------------------------------------------------*/

// 1. FORWARD
void Robot_Forward(uint16_t speed) {
    Set_Motor_Speed(speed, speed);
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_10, GPIO_PIN_SET);   // D2 (IN1) = 1
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_3,  GPIO_PIN_RESET); // D3 (IN2) = 0
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_5,  GPIO_PIN_SET);   // D4 (IN3) = 1
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_4,  GPIO_PIN_RESET); // D5 (IN4) = 0
}

// 2. STOP
void Robot_Stop(void) {
    // 1. Force the PWM timer counters to 0 immediately to cut all power
    __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_3, 0); // ENA = 0
    __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_2, 0); // ENB = 0

    // 2. Clear all direction pins to ground out the motor windings
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_10, GPIO_PIN_RESET); // D2 = 0
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_3,  GPIO_PIN_RESET); // D3 = 0
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_5,  GPIO_PIN_RESET); // D4 = 0
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_4,  GPIO_PIN_RESET); // D5 = 0

    // 3. Tiny delay to allow the hardware registers to latch the stop state
    HAL_Delay(10);
}

// 3. TURN RIGHT
void Robot_TurnRight(uint16_t speed) {
    Set_Motor_Speed(speed, speed);
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_10, GPIO_PIN_SET);   // Left Forward
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_3,  GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_5,  GPIO_PIN_RESET); // Right Stop
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_4,  GPIO_PIN_RESET);
}

// 4. TURN LEFT
void Robot_TurnLeft(uint16_t speed) {
    Set_Motor_Speed(speed, speed);
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_10, GPIO_PIN_RESET); // Left Stop
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_3,  GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_5,  GPIO_PIN_SET);   // Right Forward
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_4,  GPIO_PIN_RESET);
}

// 5. U-TURN
void Robot_UTurn(uint16_t speed) {
    Set_Motor_Speed(speed, speed);
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_10, GPIO_PIN_SET);   // Left Forward
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_3,  GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_5,  GPIO_PIN_RESET); // Right Backward
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_4,  GPIO_PIN_SET);
}

/**
  * @brief  The application entry point.
  */
int main(void)
{
  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* Configure the system clock */
  SystemClock_Config();

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_TIM2_Init();
  MX_TIM3_Init();

  /* USER CODE BEGIN 2 */
  // START THE HARDWARE PWM GENERATION
  HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_3); // Starts ENA Speed Pin (D6)
  HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_2); // Starts ENB Speed Pin (D9)
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  /* Infinite loop */
    /* USER CODE BEGIN WHILE */
    while (1)
    {
        // ==========================================
        // TEST 1: FORWARD (Both wheels spin forward)
        // ==========================================
        Robot_Forward(700);
        HAL_Delay(3000);      // Run for 3 seconds

        Robot_Stop();
        HAL_Delay(2000);      // Stop completely for 2 seconds

        // ==========================================
        // TEST 2: TURN RIGHT (Left forward, Right stops)
        // ==========================================
        Robot_TurnRight(600);
        HAL_Delay(2000);      // Run for 2 seconds

        Robot_Stop();
        HAL_Delay(2000);      // Stop completely for 2 seconds

        // ==========================================
        // TEST 3: TURN LEFT (Right forward, Left stops)
        // ==========================================
        Robot_TurnLeft(600);
        HAL_Delay(2000);      // Run for 2 seconds

        Robot_Stop();
        HAL_Delay(2000);      // Stop completely for 2 seconds

        // ==========================================
        // TEST 4: U-TURN (Left forward, Right backward)
        // ==========================================
        Robot_UTurn(600);
        HAL_Delay(2000);      // Run for 2 seconds

        Robot_Stop();
        HAL_Delay(5000);      // Long 5-second pause before repeating everything

      /* USER CODE END WHILE */

      /* USER CODE BEGIN 3 */
    }
    /* USER CODE END 3 */
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

  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 8;
  RCC_OscInitStruct.PLL.PLLN = 180;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 2;
  RCC_OscInitStruct.PLL.PLLR = 2;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    while(1);
  }

  if (HAL_PWREx_EnableOverDrive() != HAL_OK)
  {
    while(1);
  }

  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
  {
    while(1);
  }
}

/**
  * @brief TIM2 Initialization Function
  */
static void MX_TIM2_Init(void)
{
  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};

  htim2.Instance = TIM2;
  htim2.Init.Prescaler = 83;
  htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim2.Init.Period = 999;
  htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_PWM_Init(&htim2) != HAL_OK)
  {
    while(1);
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig) != HAL_OK)
  {
    while(1);
  }
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 0;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  if (HAL_TIM_PWM_ConfigChannel(&htim2, &sConfigOC, TIM_CHANNEL_3) != HAL_OK)
  {
    while(1);
  }
  HAL_TIM_MspPostInit(&htim2);
}

/**
  * @brief TIM3 Initialization Function
  */
static void MX_TIM3_Init(void)
{
  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};

  htim3.Instance = TIM3;
  htim3.Init.Prescaler = 83;
  htim3.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim3.Init.Period = 999;
  htim3.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim3.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_PWM_Init(&htim3) != HAL_OK)
  {
    while(1);
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim3, &sMasterConfig) != HAL_OK)
  {
    while(1);
  }
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 0;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  if (HAL_TIM_PWM_ConfigChannel(&htim3, &sConfigOC, TIM_CHANNEL_2) != HAL_OK)
  {
    while(1);
  }
  HAL_TIM_MspPostInit(&htim3);
}

/**
  * @brief GPIO Initialization Function
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_10, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_3|GPIO_PIN_4|GPIO_PIN_5, GPIO_PIN_RESET);

  /*Configure GPIO pin : PA10 */
  GPIO_InitStruct.Pin = GPIO_PIN_10;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pins : PB3 PB4 PB5 */
  GPIO_InitStruct.Pin = GPIO_PIN_3|GPIO_PIN_4|GPIO_PIN_5;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
}
