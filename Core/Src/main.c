/* USER CODE BEGIN Header */
/**
  * ******************************************************************************
  * @file           : main.c
  * @brief          : Smooth 2-Second Straight Line Loop with Instant Raw IR Scan
  * ******************************************************************************
  */
/* USER CODE END Header */

#include "main.h"

TIM_HandleTypeDef htim2;
TIM_HandleTypeDef htim3;

uint16_t Base_Speed = 430; // Cruise baseline speed

/* Function Prototypes */
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_TIM2_Init(void);
static void MX_TIM3_Init(void);
void Set_Motor_Speed(uint16_t left_speed, uint16_t right_speed);
void Robot_Stop(void);
uint8_t Robot_Drive_And_Scan(uint16_t speed, uint32_t total_duration_ms);

/* Helper function to update PWM registers safely */
void Set_Motor_Speed(uint16_t left_speed, uint16_t right_speed) {
    __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_3, left_speed);  // ENA (Left Motor)
    __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_2, right_speed); // ENB (Right Motor)
}

/**
 * @brief  PRECISION ANTI-JERK STOP
 * Cuts power to the slightly stronger left side first, waits 70ms to absorb
 * its physical forward momentum, then snaps the right side completely shut.
 */
void Robot_Stop(void) {
    // 1. Kill Left Motor instantly
    __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_3, 0);
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_10, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_3,  GPIO_PIN_RESET);

    // ⏱️ 70ms MOMENTUM BALANCER (Matches your nice left jerk fix!)
    HAL_Delay(70);

    // 2. Now brake the Right Motor dead straight
    __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_2, 0);
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_5,  GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_4,  GPIO_PIN_RESET);
}

/**
 * @brief  Drives forward by splitting 2 seconds into 500ms micro-blocks.
 * Fires a fresh kickstart pulse every 500ms to maintain power and prevent left U-turns!
 */
/**
 * @brief  Drives forward straight. Injects a targeted high-power kickstart pulse
 * at exactly the 1-second mark to eliminate battery sag and fix the left turn.
 */
uint8_t Robot_Drive_And_Scan(uint16_t speed, uint32_t total_duration_ms) {

    // Your calibrated straight-line cruise speeds
    uint16_t balanced_left_speed = 380;
    uint16_t balanced_right_speed = 320;

    // Set directions forward immediately
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_10, GPIO_PIN_SET);   // IN1 = HIGH
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_3,  GPIO_PIN_RESET); // IN2 = LOW
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_5,  GPIO_PIN_SET);   // IN3 = HIGH
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_4,  GPIO_PIN_RESET); // IN4 = LOW

    // 🚀 INITIAL LAUNCH: Quick 25ms kickstart to get moving from a dead stop
    Set_Motor_Speed(650, 600);
    HAL_Delay(25);

    // Drop into steady cruise power for the first half
    Set_Motor_Speed(balanced_left_speed, balanced_right_speed);

    uint32_t current_tick = 25;
    uint8_t mid_kick_fired = 0;

    // 🏎️ CONTINUOUS STRAIGHT RUN LOOP
    while (current_tick < total_duration_ms) {

        // 🔨 THE MID-RUN HAMMER: Exactly at 1 second (1000ms), blast through the battery drop!
        if (current_tick >= 1000 && mid_kick_fired == 0)
        {
            // Give an intense 30ms punch to synchronize both wheels instantly
            Set_Motor_Speed(800, 800);
            HAL_Delay(30);

            // Log the 30ms spent in the delay so the 2-second total clock stays perfect
            current_tick += 30;

            // Re-lock straight into cruise power for the remaining time
            Set_Motor_Speed(balanced_left_speed, balanced_right_speed);
            mid_kick_fired = 1; // Lock out so it only fires once
        }

        // Read IR Sensor Pin directly (PA8 / D7)
        if (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_8) == GPIO_PIN_RESET)
        {
            HAL_Delay(5); // Noise filter
            if (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_8) == GPIO_PIN_RESET)
            {
                Robot_Stop();
                return 1; // Obstacle hit!
            }
            current_tick += 5;
        }

        HAL_Delay(1);
        current_tick++;
    }

    // Only stops here if the full 2 seconds completed safely without any obstacle!
    Robot_Stop();
    return 0;
}
int main(void)
{
  HAL_Init();
  SystemClock_Config();
  MX_GPIO_Init();
  MX_TIM2_Init();
  MX_TIM3_Init();

  HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_3);
  HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_2);

  Robot_Stop();
  HAL_Delay(1000); // Startup break

  while (1)
    {
      // Launch a continuous 2-second forward run
      uint8_t obstacle_tripped = Robot_Drive_And_Scan(Base_Speed, 2000);

      if (obstacle_tripped == 1) {
          // 🚨 OBSTACLE ENCOUNTERED: Freeze completely for 5 seconds as requested!
          Robot_Stop();
          HAL_Delay(5000);
      }
      else {
          // ROAD IS CLEAR: Microscopic 50ms transition gap just to clear timer registers,
          // then immediately fires into the next smooth 2-second continuous run!
          HAL_Delay(50);
      }
    }
}

/* --- Hardware Initializations --- */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  __HAL_RCC_GPIOB_CLK_ENABLE();
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

  /* IR SENSOR INPUT PIN CONFIGURATION (PA8 / D7) */
  GPIO_InitStruct.Pin = GPIO_PIN_8;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP; // Enforce high internal state
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
}

void SystemClock_Config(void) {
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
  HAL_RCC_OscConfig(&RCC_OscInitStruct);
  HAL_PWREx_EnableOverDrive();
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK|RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;
  HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5);
}

static void MX_TIM2_Init(void) {
  TIM_OC_InitTypeDef sConfigOC = {0};
  htim2.Instance = TIM2;
  htim2.Init.Prescaler = 83;
  htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim2.Init.Period = 999;
  HAL_TIM_PWM_Init(&htim2);
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 0;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  HAL_TIM_PWM_ConfigChannel(&htim2, &sConfigOC, TIM_CHANNEL_3);
  HAL_TIM_MspPostInit(&htim2);
}

static void MX_TIM3_Init(void) {
  TIM_OC_InitTypeDef sConfigOC = {0};
  htim3.Instance = TIM3;
  htim3.Init.Prescaler = 83;
  htim3.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim3.Init.Period = 999;
  HAL_TIM_PWM_Init(&htim3);
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 0;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  HAL_TIM_PWM_ConfigChannel(&htim3, &sConfigOC, TIM_CHANNEL_2);
  HAL_TIM_MspPostInit(&htim3);
}

void Error_Handler(void) { while (1) {} }
