/* USER CODE BEGIN Header */
/**
  * ******************************************************************************
  * @file           : main.c
  * @brief          : Standalone Dual-Timer Core (TIM3-Left / TIM2-Right Override)
  * ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define SENSOR_ADDR (0x29 << 1)
#define CMD_BIT     0x80
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
I2C_HandleTypeDef hi2c1;

TIM_HandleTypeDef htim2;
TIM_HandleTypeDef htim3;

UART_HandleTypeDef huart2;

/* USER CODE BEGIN PV */
volatile uint16_t Red_Color = 0;
volatile uint16_t Green_Color = 0;
volatile uint16_t Blue_Color = 0;
volatile uint16_t Clear_Color = 0;

volatile HAL_StatusTypeDef scan_result = HAL_ERROR;
volatile uint8_t found_address = 0;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_TIM2_Init(void);
static void MX_TIM3_Init(void);
static void MX_I2C1_Init(void);
static void MX_USART2_UART_Init(void);
/* USER CODE BEGIN PFP */
void Read_Color_Values(void);
void Color_Sensor_Init(void);
void Set_Motor_Speed(uint32_t left_speed, uint32_t right_speed);
void Debug_Print(const char* format, ...);
void Robot_Forward(void);
void Robot_Stop(void);
void Robot_Turn_Left(void);
void Robot_Turn_Right(void);
void Robot_UTurn(void);
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
  HAL_Init();

  /* =========================================================================
     🧹 EMERGENCY HARDWARE PIN BIT-FLUSH (UNBRICK ROUTINE)
     ========================================================================= */
  __HAL_RCC_GPIOB_CLK_ENABLE();
  GPIO_InitTypeDef GPIO_ResetStruct = {0};
  GPIO_ResetStruct.Pin = GPIO_PIN_8 | GPIO_PIN_9;
  GPIO_ResetStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_ResetStruct.Pull = GPIO_NOPULL;
  GPIO_ResetStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_ResetStruct);

  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_8 | GPIO_PIN_9, GPIO_PIN_SET);
  HAL_Delay(10);
  for(int i = 0; i < 9; i++) {
      HAL_GPIO_WritePin(GPIOB, GPIO_PIN_8, GPIO_PIN_RESET);
      HAL_Delay(5);
      HAL_GPIO_WritePin(GPIOB, GPIO_PIN_8, GPIO_PIN_SET);
      HAL_Delay(5);
  }
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_8 | GPIO_PIN_9, GPIO_PIN_SET);
  HAL_Delay(10);
  /* ========================================================================= */

  SystemClock_Config();

  MX_GPIO_Init();
  MX_TIM2_Init();
  MX_TIM3_Init();
  MX_I2C1_Init();
  MX_USART2_UART_Init();

  /* USER CODE BEGIN 2 */
  /* 🏎️ Fire up the Dual-Timer Engine channels */
  HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_2); // Left Speed via D9
  HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_3); // Right Speed via D11 Override

  HAL_Delay(200);

  Debug_Print("\r\n🚀 DUAL-TIMER FIRMWARE LOADED: Right channel active on D11\r\n");

  scan_result = HAL_I2C_IsDeviceReady(&hi2c1, SENSOR_ADDR, 3, 100);
  if (scan_result == HAL_OK) {
      found_address = 0x29;
      Debug_Print("✅ Color Sensor Detected at address: 0x29\r\n");
  } else {
      Debug_Print("⚠️ WARNING: Color Sensor missed on I2C1 bus!\r\n");
  }

  Color_Sensor_Init();
  Robot_Forward();
  /* USER CODE END 2 */

  /* Infinite loop */
    /* USER CODE BEGIN WHILE */
    while (1)
    {
      uint8_t ir_read = HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_8);

      if (ir_read == 0)
      {
          // 🛡️ HARDWARE DEBOUNCE
          HAL_Delay(40);
          if (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_8) != 0) {
              continue;
          }

          /* 🛑 STEP 1 & 2: IMMEDIATE STOP & 2-SECOND SETTLING WINDOW */
          Robot_Stop();
          Debug_Print("ℹ️ Obstacle flagged. Vehicle locked. Settling for 2 seconds...\r\n");
          HAL_Delay(2000); // 2-second hold to eliminate mechanical shake and stabilize I2C lines

          // 🗳️ Vote Trackers
          int red_votes = 0;
          int green_votes = 0;
          int blue_votes = 0;
          int white_votes = 0;

          Debug_Print("🔄 Commencing 5-sample data acquisition...\r\n");

          /* 🔄 STEP 4: CAPTURE SAMPLES UNDER LOW-LUX CONDITIONS */
          for (int sample = 1; sample <= 5; sample++)
          {
              Read_Color_Values();

              uint32_t r_scaled = 0;
              uint32_t g_scaled = 0;
              uint32_t b_scaled = 0;

              if (Clear_Color > 0)
              {
                  r_scaled = ((uint32_t)Red_Color * 255) / Clear_Color;
                  g_scaled = ((uint32_t)Green_Color * 255) / Clear_Color;
                  b_scaled = ((uint32_t)Blue_Color * 255) / Clear_Color;

                  // Clamp values to max 8-bit scale
                  if (r_scaled > 255) r_scaled = 255;
                  if (g_scaled > 255) g_scaled = 255;
                  if (b_scaled > 255) b_scaled = 255;
              }

              Debug_Print("  [Sample %d] -> Raw Clear: %d | R:%d G:%d B:%d\r\n", sample, Clear_Color, (int)r_scaled, (int)g_scaled, (int)b_scaled);

              /* 🎨 LOW-LUX COMPENSATED INTERPRETER ENGINE */
              if (Clear_Color > 200 && r_scaled > 210 && g_scaled > 180)
              {
                  /* White target confirmation based on high raw balance metrics */
                  white_votes++;
              }
              else if (Clear_Color > 15) // Lowered safety floor to securely process low-lux readings
              {
                  /* Green target isolation: G is strictly dominant over R and B */
                  if (g_scaled > r_scaled && g_scaled > b_scaled)
                  {
                      green_votes++;
                  }
                  /* Red target isolation: R is strictly dominant over G and B */
                  else if (r_scaled > g_scaled && r_scaled > b_scaled)
                  {
                      red_votes++;
                  }
                  /* Blue target isolation */
                  else if (b_scaled > g_scaled && b_scaled > 140)
                  {
                      blue_votes++;
                  }
                  else
                  {
                      // Clean mathematical fallback rule if signals flatten out evenly
                      if (b_scaled > r_scaled && b_scaled > g_scaled) blue_votes++;
                  }
              }

              HAL_Delay(110);
          }

          /* ⏱️ STEP 5: 3-SECOND POST-PROCESSING DATA WINDOW */
          Debug_Print("🗳️ REPORT CARD -> RED:%d | GREEN:%d | BLUE:%d | WHITE:%d\r\n", red_votes, green_votes, blue_votes, white_votes);
          Debug_Print("ℹ️ Holding for 3 seconds to process decision...\r\n");
          HAL_Delay(3000); // 3-second diagnostic delay to safely verify results on the terminal

          /* 🚀 STEP 6: EXECUTE ROUTE ACTION */
          uint8_t executed_turn = 0;

          if (white_votes >= 3)
          {
              Debug_Print("↳ Route Action: WHITE -> U-Turn\r\n");
              Robot_UTurn();
              executed_turn = 1;
          }
          else if (red_votes >= 3)
          {
              Debug_Print("↳ Route Action: RED -> 5-Second Stop\r\n");
              Robot_Stop();
              HAL_Delay(5000);
              executed_turn = 1;
          }
          else if (green_votes >= 3)
          {
              Debug_Print("↳ Route Action: GREEN -> Turn Right\r\n");
              Robot_Turn_Right();
              executed_turn = 1;
          }
          else if (blue_votes >= 3)
          {
              Debug_Print("↳ Route Action: BLUE -> Turn Left\r\n");
              Robot_Turn_Left();
              executed_turn = 1;
          }
          else
          {
              Debug_Print("↳ Route Action: SPLIT VOTE -> Holding Brake\r\n");
              Robot_Stop();
              HAL_Delay(500);
          }

          // Kick back into low-speed cruise mode
          Robot_Forward();

          if (executed_turn == 1) {
              HAL_Delay(1000);
          }
      }
      else
      {
          Robot_Forward();
      }

      HAL_Delay(20);
    }
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
    /* USER CODE END 3 */

}

void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE3);

  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  HAL_RCC_OscConfig(&RCC_OscInitStruct);

  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK|RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
  HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0);
}

static void MX_I2C1_Init(void)
{
  hi2c1.Instance = I2C1;
  hi2c1.Init.ClockSpeed = 100000;
  hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
  hi2c1.Init.OwnAddress1 = 0;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  HAL_I2C_Init(&hi2c1);
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
  HAL_TIM_PWM_Init(&htim2);

  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig);

  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 0;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  HAL_TIM_PWM_ConfigChannel(&htim2, &sConfigOC, TIM_CHANNEL_3);

  HAL_TIM_MspPostInit(&htim2);
}

static void MX_TIM3_Init(void)
{
  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};

  htim3.Instance = TIM3;
  htim3.Init.Prescaler = 83;
  htim3.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim3.Init.Period = 999;
  htim3.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim3.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  HAL_TIM_Base_Init(&htim3);

  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  HAL_TIM_ConfigClockSource(&htim3, &sClockSourceConfig);
  HAL_TIM_PWM_Init(&htim3);

  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  HAL_TIMEx_MasterConfigSynchronization(&htim3, &sMasterConfig);

  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 0;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  HAL_TIM_PWM_ConfigChannel(&htim3, &sConfigOC, TIM_CHANNEL_2);

  HAL_TIM_MspPostInit(&htim3);
}

static void MX_USART2_UART_Init(void)
{
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 115200;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  HAL_UART_Init(&huart2);
}

static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();

  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_10, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_3|GPIO_PIN_4|GPIO_PIN_5, GPIO_PIN_RESET);

  /* IR Line Input Sensor */
  GPIO_InitStruct.Pin = GPIO_PIN_8;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /* IN1 Motor Rail Switch (PA10) */
  GPIO_InitStruct.Pin = GPIO_PIN_10;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /* IN2, IN3, IN4 Directional Bridges (PB3, PB4, PB5) */
  GPIO_InitStruct.Pin = GPIO_PIN_3|GPIO_PIN_4|GPIO_PIN_5;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /* USER CODE BEGIN MX_GPIO_Init_2 */
  /* 🏎️ HARD OVERRIDE OVERLAY: Route D11 (PB10) natively to TIM2_CH3 */
  GPIO_InitStruct.Pin = GPIO_PIN_10;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF1_TIM2;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /* Route D9 (PC7) natively to TIM3_CH2 */
  GPIO_InitStruct.Pin = GPIO_PIN_7;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF2_TIM3;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);
  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */
void Debug_Print(const char* format, ...) {
    char wifi_buffer[128];
    va_list args;
    va_start(args, format);
    vsnprintf(wifi_buffer, sizeof(wifi_buffer), format, args);
    va_end(args);
    HAL_UART_Transmit(&huart2, (uint8_t*)wifi_buffer, strlen(wifi_buffer), 50);
}

/* =========================================================================
    🏎️ SPLIT-TIMER MOTOR CONTROLLER INTERFACE
   ========================================================================= */
void Set_Motor_Speed(uint32_t left_speed, uint32_t right_speed) {
    __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_2, left_speed);   // Left Channel -> D9
    __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_3, right_speed);  // Right Channel -> D11
}

void Robot_Forward(void) {
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_10, GPIO_PIN_SET);   // IN1
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_3,  GPIO_PIN_RESET); // IN2
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_5,  GPIO_PIN_SET);   // IN3
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_4,  GPIO_PIN_RESET); // IN4

    Set_Motor_Speed(950, 950);
    HAL_Delay(25);

    Set_Motor_Speed(450, 450);
}

void Robot_Stop(void) {
    /* ⚡ ACTIVE REVERSE BRAKE: Force H-Bridge into reverse layout */
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_10, GPIO_PIN_RESET); // IN1
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_3,  GPIO_PIN_SET);   // IN2 (Reverse Left)
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_4,  GPIO_PIN_SET);   // IN4 (Reverse Right)
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_5,  GPIO_PIN_RESET); // IN3

    /* ⚖️ DRIFT CORRECTION: Pulse Left wheel (TIM3) slightly higher to counter the left drift */
    __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_2, 850);  // Left Speed (Slightly stronger reverse snap)
    __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_3, 750);  // Right Speed
    HAL_Delay(40); // Increased slightly from 35ms to ensure a complete dead-stop

    /* Cut all power completely */
    __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_2, 0);
    __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_3, 0);
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_10, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_3,  GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_5,  GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_4,  GPIO_PIN_RESET);
}
void Robot_Turn_Left(void) {
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_10, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_3,  GPIO_PIN_SET);
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_5,  GPIO_PIN_SET);
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_4,  GPIO_PIN_RESET);

    Set_Motor_Speed(850, 850); // Quick kickstart
    HAL_Delay(20);

    Set_Motor_Speed(160, 160);
    HAL_Delay(290); /* 📉 DECREASED from 350ms to fix the OVER-turn */
    Robot_Stop();
}

void Robot_Turn_Right(void) {
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_10, GPIO_PIN_SET);
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_3,  GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_5,  GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_4,  GPIO_PIN_SET);

    Set_Motor_Speed(850, 850); // Quick kickstart
    HAL_Delay(20);

    Set_Motor_Speed(160, 160);
    HAL_Delay(410); /* 📈 INCREASED from 350ms to fix the UNDER-turn */
    Robot_Stop();
}
void Robot_UTurn(void) {
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_10, GPIO_PIN_SET);
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_3,  GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_5,  GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_4,  GPIO_PIN_SET);

    Set_Motor_Speed(980, 980);
    HAL_Delay(30);

    Set_Motor_Speed(460, 460);
    HAL_Delay(750);
    Robot_Stop();
}

void Color_Sensor_Init(void) {
    uint8_t tx_data[2];
    tx_data[0] = CMD_BIT | 0x01; tx_data[1] = 0xD5;
    HAL_I2C_Master_Transmit(&hi2c1, SENSOR_ADDR, tx_data, 2, 100);
    tx_data[0] = CMD_BIT | 0x0F; tx_data[1] = 0x00;
    HAL_I2C_Master_Transmit(&hi2c1, SENSOR_ADDR, tx_data, 2, 100);
    tx_data[0] = CMD_BIT | 0x00; tx_data[1] = 0x03;
    HAL_I2C_Master_Transmit(&hi2c1, SENSOR_ADDR, tx_data, 2, 100);
}

void Read_Color_Values(void) {
    uint8_t rx_buff[2];
    HAL_I2C_Mem_Read(&hi2c1, SENSOR_ADDR, CMD_BIT | 0x14, I2C_MEMADD_SIZE_8BIT, rx_buff, 2, 100);
    Clear_Color = (uint16_t)((rx_buff[1] << 8) | rx_buff[0]);
    HAL_I2C_Mem_Read(&hi2c1, SENSOR_ADDR, CMD_BIT | 0x16, I2C_MEMADD_SIZE_8BIT, rx_buff, 2, 100);
    Red_Color = (uint16_t)((rx_buff[1] << 8) | rx_buff[0]);
    HAL_I2C_Mem_Read(&hi2c1, SENSOR_ADDR, CMD_BIT | 0x18, I2C_MEMADD_SIZE_8BIT, rx_buff, 2, 100);
    Green_Color = (uint16_t)((rx_buff[1] << 8) | rx_buff[0]);
    HAL_I2C_Mem_Read(&hi2c1, SENSOR_ADDR, CMD_BIT | 0x1A, I2C_MEMADD_SIZE_8BIT, rx_buff, 2, 100);
    Blue_Color = (uint16_t)((rx_buff[1] << 8) | rx_buff[0]);
}
/* USER CODE END 4 */

void Error_Handler(void)
{
  __disable_irq();
  while (1) {}
}
