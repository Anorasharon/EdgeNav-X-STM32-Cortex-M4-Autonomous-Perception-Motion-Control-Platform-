/* USER CODE BEGIN Header */
/**
  * ******************************************************************************
  * @file           : main.c
  * @brief          : Standalone Robot Code with Synchronized Majority Voting
  * ******************************************************************************
  */
/* USER CODE END Header */

#include "main.h"
#include <stdio.h>
#include <string.h>

/* Private variables ---------------------------------------------------------*/
I2C_HandleTypeDef hi2c1;
TIM_HandleTypeDef htim2;  // Controls PWM speed channels
UART_HandleTypeDef huart2;

#define SENSOR_ADDR (0x29 << 1)
#define CMD_BIT     0x80

/* Global Live Expressions Monitoring Buffers */
volatile uint16_t Red_Color = 0;
volatile uint16_t Green_Color = 0;
volatile uint16_t Blue_Color = 0;
volatile uint16_t Clear_Color = 0;

/* Diagnostic Registers */
volatile HAL_StatusTypeDef scan_result = HAL_ERROR;
volatile uint8_t found_address = 0;

/* Function Prototypes -------------------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_I2C1_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_TIM2_Init(void);

void Read_Color_Values(void);
void Color_Sensor_Init(void);
void Set_Motor_Speed(uint32_t left_speed, uint32_t right_speed);
void Robot_Forward(void);
void Robot_Stop(void);
void Robot_Turn_Left(void);
void Robot_Turn_Right(void);
void Robot_UTurn(void);

/* Application Entry Point ---------------------------------------------------*/
int main(void)
{
  /* 1. Initialize low-level abstraction layers */
  HAL_Init();

  /* =========================================================================
     🧹 EMERGENCY HARDWARE PIN BIT-FLUSH (UNBRICK ROUTINE)
     ========================================================================= */
  __HAL_RCC_GPIOB_CLK_ENABLE();
  GPIO_InitTypeDef GPIO_ResetStruct = {0};

  // Force SCL (PB8) and SDA (PB9) into general purpose output mode
  GPIO_ResetStruct.Pin = GPIO_PIN_8 | GPIO_PIN_9;
  GPIO_ResetStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_ResetStruct.Pull = GPIO_NOPULL;
  GPIO_ResetStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_ResetStruct);

  // Drive both rails high initially
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_8 | GPIO_PIN_9, GPIO_PIN_SET);
  HAL_Delay(10);

  // Pulse the Clock rail 9 times to push stuck bits out of the shift registers
  for(int i = 0; i < 9; i++) {
      HAL_GPIO_WritePin(GPIOB, GPIO_PIN_8, GPIO_PIN_RESET);
      HAL_Delay(5);
      HAL_GPIO_WritePin(GPIOB, GPIO_PIN_8, GPIO_PIN_SET);
      HAL_Delay(5);
  }

  // Leave lines clean and release physical rails
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_8 | GPIO_PIN_9, GPIO_PIN_SET);
  HAL_Delay(10);
  /* ========================================================================= */

  /* 2. Configure System Core Clock Engine */
  SystemClock_Config();

  /* 3. Re-initialize peripherals back into their intended native modes */
  MX_GPIO_Init();        // This remaps PB8/PB9 into native AF Open-Drain I2C mode
  MX_I2C1_Init();
  MX_USART2_UART_Init();
  MX_TIM2_Init();

  // Start your PWM timers for motor actuation
  HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_3); // Left speed control channel
  HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_4); // Right speed control channel

  HAL_Delay(200); // Settling delay for stable hardware transitions

  /* 🔎 Active Diagnostic Bus Scan */
  scan_result = HAL_I2C_IsDeviceReady(&hi2c1, SENSOR_ADDR, 3, 100);
  if (scan_result == HAL_OK) {
      found_address = 0x29;
  }

  /* 🎨 Initialize Sensor Registers with your working test profiles */
  Color_Sensor_Init();

  /* =====================================================
     MAIN RUNTIME PROCESSING LOOP
     ===================================================== */
  while (1)
  {
    // Pin Check: PA8 reading your physical IR distance sensor
    uint8_t first_read = HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_8);

    if (first_read == 0)
    {
        // 🛡️ HARDWARE DEBOUNCE
        HAL_Delay(40);
        if (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_8) != 0) {
            continue;
        }

        /* ⏱️ SHORT SETTLING DELAY */
        // Stopped chassis vibrations before sampling
        Robot_Stop();
        HAL_UART_Transmit(&huart2, (uint8_t*)"ℹ️ Obstacle detected. Voting running...\r\n", 41, 100);
        HAL_Delay(500);

        // 🗳️ Vote Counters
        int red_votes = 0;
        int green_votes = 0;
        int blue_votes = 0;
        int white_votes = 0;

        /* 🔄 TAKE 5 DISTINCT SAMPLES FOR THE MAJORITY VOTE */
        for (int sample = 1; sample <= 5; sample++)
        {
            Read_Color_Values();

            /* 🧮 Scaled Calculation Engine from working test script */
            uint32_t r_scaled = 0;
            uint32_t g_scaled = 0;
            uint32_t b_scaled = 0;

            if (Clear_Color > 0)
            {
                r_scaled = ((uint32_t)Red_Color * 255) / Clear_Color;
                g_scaled = ((uint32_t)Green_Color * 255) / Clear_Color;
                b_scaled = ((uint32_t)Blue_Color * 255) / Clear_Color;

                if (r_scaled > 255) r_scaled = 255;
                if (g_scaled > 255) g_scaled = 255;
                if (b_scaled > 255) b_scaled = 255;
            }

            /* 📺 Print each sample track to PuTTY console */
            char sample_msg[64];
            sprintf(sample_msg, "  Sample %d -> R:%d G:%d B:%d\r\n", sample, (int)r_scaled, (int)g_scaled, (int)b_scaled);
            HAL_UART_Transmit(&huart2, (uint8_t*)sample_msg, strlen(sample_msg), 100);

            /* 🧠 Log individual sample votes into data array */
            if (Clear_Color > 16000 && r_scaled > 140 && g_scaled > 130)
            {
                white_votes++;
            }
            else if (!(r_scaled < 15 && g_scaled < 15 && b_scaled < 15))
            {
                uint32_t current_max = r_scaled;
                uint8_t dominant_color = 1; // 1 = Red, 2 = Green, 3 = Blue

                if (g_scaled > current_max) { current_max = g_scaled; dominant_color = 2; }
                if (b_scaled > current_max) { current_max = b_scaled; dominant_color = 3; }

                if (dominant_color == 1)      red_votes++;
                else if (dominant_color == 2) green_votes++;
                else if (dominant_color == 3) blue_votes++;
            }

            // ⏱️ 110ms Delay: Allows the 101ms sensor conversion window to dump fresh values
            HAL_Delay(110);
        }

        /* 📊 EVALUATE WINNING VOTE RESULT */
        char tally_msg[128];
        sprintf(tally_msg, "🗳️ TALLY -> RED:%d | GREEN:%d | BLUE:%d | WHITE:%d\r\n", red_votes, green_votes, blue_votes, white_votes);
        HAL_UART_Transmit(&huart2, (uint8_t*)tally_msg, strlen(tally_msg), 100);

        uint8_t executed_turn = 0;

        // Process final democratic winner selection (Requires at least 3 out of 5 checks)
        if (white_votes >= 3)
        {
            HAL_UART_Transmit(&huart2, (uint8_t*)"↳ Winner: WHITE -> U-Turn\r\n", 27, 100);
            Robot_UTurn();
            executed_turn = 1;
        }
        else if (red_votes >= 3)
        {
            HAL_UART_Transmit(&huart2, (uint8_t*)"↳ Winner: RED -> 5-Sec Stop\r\n", 29, 100);
            Robot_Stop();
            HAL_Delay(5000);
            executed_turn = 1;
        }
        else if (green_votes >= 3)
        {
            HAL_UART_Transmit(&huart2, (uint8_t*)"↳ Winner: GREEN -> Pivot RIGHT\r\n", 32, 100);
            Robot_Turn_Right();
            executed_turn = 1;
        }
        else if (blue_votes >= 3)
        {
            HAL_UART_Transmit(&huart2, (uint8_t*)"↳ Winner: BLUE -> Pivot LEFT\r\n", 30, 100);
            Robot_Turn_Left();
            executed_turn = 1;
        }
        else
        {
            HAL_UART_Transmit(&huart2, (uint8_t*)"↳ Winner: SPLIT VOTE -> Guard Brake\r\n", 37, 100);
            Robot_Stop();
            HAL_Delay(500);
        }

        /* 🚀 Resume forward drive sequence */
        Robot_Forward();

        /* 🛡️ BLIND-SPOT ESCAPE WINDOW */
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
}

/* 🎨 Hardware Initialization Profile */
void Color_Sensor_Init(void) {
    uint8_t tx_data[2];

    // ⏱️ Set Integration Time (ATIME) to 0xD5 (approx 101ms fast conversion time)
    tx_data[0] = CMD_BIT | 0x01;
    tx_data[1] = 0xD5;
    HAL_I2C_Master_Transmit(&hi2c1, SENSOR_ADDR, tx_data, 2, 100);

    // Set Gain Register (CONTROL) to 1x to duplicate your working test parameters
    tx_data[0] = CMD_BIT | 0x0F;
    tx_data[1] = 0x00;
    HAL_I2C_Master_Transmit(&hi2c1, SENSOR_ADDR, tx_data, 2, 100);

    // Power ON, Enable Internal ADC (ENABLE)
    tx_data[0] = CMD_BIT | 0x00;
    tx_data[1] = 0x03;
    HAL_I2C_Master_Transmit(&hi2c1, SENSOR_ADDR, tx_data, 2, 100);
}

/* I2C Data Retrieval Routine */
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

/* =========================================================================
   🏎️ CHASSIS MOVEMENT DRIVER DEFINITIONS
   ========================================================================= */
void Set_Motor_Speed(uint32_t left_speed, uint32_t right_speed) {
    __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_3, left_speed);
    __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_4, right_speed);
}

void Robot_Forward(void) {
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_10, GPIO_PIN_SET);   // IN1 = H
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_3,  GPIO_PIN_RESET); // IN2 = L
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_5,  GPIO_PIN_SET);   // IN3 = H
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_4,  GPIO_PIN_RESET); // IN4 = L
    Set_Motor_Speed(420, 415);
}

void Robot_Stop(void) {
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_10, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_3,  GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_5,  GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_4,  GPIO_PIN_RESET);
    Set_Motor_Speed(0, 0);
}

void Robot_Turn_Left(void) {
    // Left side backward, Right side forward
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_10, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_3,  GPIO_PIN_SET);
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_5,  GPIO_PIN_SET);
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_4,  GPIO_PIN_RESET);

    Set_Motor_Speed(400, 400);
    HAL_Delay(120);
    Robot_Stop();
}

void Robot_Turn_Right(void) {
    // Left side forward, Right side backward
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_10, GPIO_PIN_SET);
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_3,  GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_5,  GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_4,  GPIO_PIN_SET);

    Set_Motor_Speed(400, 400);
    HAL_Delay(120);
    Robot_Stop();
}

void Robot_UTurn(void) {
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_10, GPIO_PIN_SET);
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_3,  GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_5,  GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_4,  GPIO_PIN_SET);

    Set_Motor_Speed(450, 450);
    HAL_Delay(280);
    Robot_Stop();
}

/* Core System Configuration Engine */
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
  HAL_RCC_OscConfig(&RCC_OscInitStruct);

  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK|RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;
  HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5);
}

static void MX_I2C1_Init(void)
{
  hi2c1.Instance = I2C1;
  hi2c1.Init.ClockSpeed = 10000;
  hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
  hi2c1.Init.OwnAddress1 = 0;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLED;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLED;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLED;
  HAL_I2C_Init(&hi2c1);
}

static void MX_TIM2_Init(void)
{
  TIM_OC_InitTypeDef sConfigOC = {0};

  htim2.Instance = TIM2;
  htim2.Init.Prescaler = 90-1;
  htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim2.Init.Period = 1000-1;
  htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1; // Clock macro fix verified
  HAL_TIM_PWM_Init(&htim2);

  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 0;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  HAL_TIM_PWM_ConfigChannel(&htim2, &sConfigOC, TIM_CHANNEL_3);
  HAL_TIM_PWM_ConfigChannel(&htim2, &sConfigOC, TIM_CHANNEL_4);
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

  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();

  /* Set H-Bridge control lines to safe default LOW values */
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_10, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_3 | GPIO_PIN_4 | GPIO_PIN_5, GPIO_PIN_RESET);

  /* Configure Motor H-Bridge Pins: PA10 */
  GPIO_InitStruct.Pin = GPIO_PIN_10;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /* Configure Motor H-Bridge Pins: PB3, PB4, PB5 */
  GPIO_InitStruct.Pin = GPIO_PIN_3 | GPIO_PIN_4 | GPIO_PIN_5;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /* Configure Physical IR Sensor Input Line: PA8 */
  GPIO_InitStruct.Pin = GPIO_PIN_8;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /* Configure Native I2C AF Pins (PB8->SCL, PB9->SDA) explicitly */
  GPIO_InitStruct.Pin = GPIO_PIN_8 | GPIO_PIN_9;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_OD;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF4_I2C1;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
}

void Error_Handler(void)
{
  __disable_irq();
  while (1) {}
}
