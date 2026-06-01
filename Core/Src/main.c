/* USER CODE BEGIN Header */
/**
  * ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body with Straight Drive and 90-Degree Turns
  * ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private variables ---------------------------------------------------------*/
TIM_HandleTypeDef htim2;
TIM_HandleTypeDef htim3;

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_TIM2_Init(void);
static void MX_TIM3_Init(void);

/* USER CODE BEGIN PFP */
void Set_Motor_Speed(uint16_t left_speed, uint16_t right_speed);
void Robot_Stop(void);
void Robot_Drive_Smooth_Timed(uint16_t speed, uint32_t total_duration_ms);
void Robot_Turn_Right_90(uint16_t turn_speed, uint32_t duration_ms);
void Robot_Turn_Left_90(uint16_t turn_speed, uint32_t duration_ms);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
uint16_t Base_Speed = 430;         // Baseline cruise speed

/* Master Speed Control Function */
void Set_Motor_Speed(uint16_t left_speed, uint16_t right_speed) {
    __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_3, left_speed);  // ENA (D6)
    __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_2, right_speed); // ENB (D9)
}

/* PRECISION COAST STOP: Smooth asymmetric drop to kill the left stop-jerk */
void Robot_Stop(void) {
    // 1. Drop right wheel speed to 0 first to kill its forward coasting momentum
    __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_2, 0); // ENB (Right) = 0

    // 2. Leave left wheel on for a microsecond flash so it holds the line
    HAL_Delay(40); // 40ms pause to let the right side naturally slow down

    // 3. Complete absolute power cutoff for both sides
    Set_Motor_Speed(0, 0);
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_10, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_3,  GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_5,  GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_4,  GPIO_PIN_RESET);
}

/* 5-SECOND DRIVING ENGINE (Lower Speed Control) */
void Robot_Drive_Smooth_Timed(uint16_t speed, uint32_t total_duration_ms) {
    // 1. TAMED KICKOFF
    Set_Motor_Speed(950, 700);

    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_10, GPIO_PIN_SET);   // IN1
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_3,  GPIO_PIN_RESET); // IN2
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_5,  GPIO_PIN_SET);   // IN3
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_4,  GPIO_PIN_RESET); // IN4
    HAL_Delay(100);

    // 2. CRUISE PHASE
    uint32_t elapsed_time = 100;
    uint16_t current_offset = 65; // Reset to a healthy baseline offset

    while (elapsed_time < total_duration_ms) {

        // Late stage minor adjustment if it drifts after 3 seconds
        if (elapsed_time > 3000) {
            current_offset = 75;
        }

        uint16_t corrected_right_speed = speed;
        if (speed > current_offset) {
            corrected_right_speed = speed - current_offset;
        } else {
            corrected_right_speed = 0;
        }

        Set_Motor_Speed(speed, corrected_right_speed);

        HAL_Delay(100);
        elapsed_time += 100;
    }
}

/* PRECISION IN-PLACE RIGHT 90-DEGREE TURN */
/* PRECISION IN-PLACE RIGHT 90-DEGREE TURN (Clockwise into Quadrant I / IV) */
void Robot_Turn_Right_90(uint16_t turn_speed, uint32_t duration_ms) {
    // FORCE CLOCKWISE: Left wheel moves FORWARD, Right wheel moves BACKWARD
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_10, GPIO_PIN_SET);   // IN1 = SET
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_3,  GPIO_PIN_RESET); // IN2 = RESET
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_5,  GPIO_PIN_RESET); // IN3 = RESET
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_4,  GPIO_PIN_SET);   // IN4 = SET

    Set_Motor_Speed(turn_speed, turn_speed);
    HAL_Delay(duration_ms);

    Robot_Stop();
}

/* PRECISION IN-PLACE LEFT 90-DEGREE TURN (Counter-Clockwise into Quadrant II) */
void Robot_Turn_Left_90(uint16_t turn_speed, uint32_t duration_ms) {
    // FORCE COUNTER-CLOCKWISE: Left wheel moves BACKWARD, Right wheel moves FORWARD
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_10, GPIO_PIN_RESET); // IN1 = RESET
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_3,  GPIO_PIN_SET);   // IN2 = SET
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_5,  GPIO_PIN_SET);   // IN3 = SET
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_4,  GPIO_PIN_RESET); // IN4 = RESET

    Set_Motor_Speed(turn_speed, turn_speed);
    HAL_Delay(duration_ms);

    Robot_Stop();
}
/* USER CODE END 0 */

int main(void)
{
  HAL_Init();
  SystemClock_Config();

  MX_GPIO_Init();
  MX_TIM2_Init();
  MX_TIM3_Init();

  /* USER CODE BEGIN 2 */
  HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_3);
  HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_2);

  Robot_Stop();
  HAL_Delay(2000); // 2 seconds to safely place it on the floor
  /* USER CODE END 2 */

  /* Infinite loop */
    while (1)
    {
      // === STEP 1: DRIVE STRAIGHT ===
      Robot_Drive_Smooth_Timed(Base_Speed, 2000);
      Robot_Stop();
      HAL_Delay(800); // Let it settle completely

      // === STEP 2: ISOLATED RIGHT TURN TUNING ===
      // If 300 was close but not perfect, try one of the two tuning options below!
      Robot_Turn_Right_90(550, 320);
      Robot_Stop();

      // === STEP 3: LONG PARK (Gives you time to check the angle) ===
      HAL_Delay(5000);
    }
}

/* --- Auto-generated STM32 Peripherals Configurations --- */

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
    Error_Handler();
  }

  if (HAL_PWREx_EnableOverDrive() != HAL_OK)
  {
    Error_Handler();
  }

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
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 0;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  if (HAL_TIM_PWM_ConfigChannel(&htim2, &sConfigOC, TIM_CHANNEL_3) != HAL_OK)
  {
    Error_Handler();
  }
  HAL_TIM_MspPostInit(&htim2);
}

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
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim3, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 0;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  if (HAL_TIM_PWM_ConfigChannel(&htim3, &sConfigOC, TIM_CHANNEL_2) != HAL_OK)
  {
    Error_Handler();
  }
  HAL_TIM_MspPostInit(&htim3);
}

static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();

  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_10, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_3|GPIO_PIN_4|GPIO_PIN_5, GPIO_PIN_RESET);

  GPIO_InitStruct.Pin = GPIO_PIN_10;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  GPIO_InitStruct.Pin = GPIO_PIN_3|GPIO_PIN_4|GPIO_PIN_5;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
}

void Error_Handler(void)
{
  __disable_irq();
  while (1)
  {
  }
}
