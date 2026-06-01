#include "main.h"
#include "liquidcrystal_i2c.h"
#include "hcsr04.h"
#include "motor.h"
#include <stdio.h>
#define DIST_SAFE_CM    100
#define DIST_DANGER_CM  50
#define MAX_PWM_SPEED   999
#define LOOP_PERIOD_MS  80
I2C_HandleTypeDef hi2c1;
/* Shared variables between Interrupt and Main Loop */
volatile uint32_t current_dist_cm = 0;
volatile uint8_t  new_data_flag   = 0;
/* FSM DEFINITION */
typedef enum {
  STATE_SAFE = 0,     /* Khoảng cách an toàn (>100cm) -> Chạy bình thường */
  STATE_WARNING,      /* Khoảng cách cảnh báo (50-100cm) -> Giảm tốc,  LED sáng dần */
  STATE_DANGER        /* Khoảng cách nguy hiểm (<=50cm) -> Phanh gấp */
} ADAS_State_t;
ADAS_State_t system_state = STATE_SAFE;
/* --- Function Prototypes --- */
void SystemClock_Config(void);
static void MX_I2C1_Init(void);
static void LCD_UpdateDisplay(uint32_t dist_cm, ADAS_State_t state);
void HCSR04_DistanceCallback(uint32_t dist_cm);
/* ================================================================== */
/* INTERRUPT CALLBACK          */
/* ================================================================== */
void HCSR04_DistanceCallback(uint32_t dist_cm)
{
 current_dist_cm = dist_cm;
 new_data_flag = 1;
}
/* ================================================================== */
int main(void)
{
 HAL_Init();
 SystemClock_Config();
 MX_I2C1_Init();
 HCSR04_Init();
 PWM_Hardware_Init();
 HD44780_Init(2);
 HD44780_Clear();
 HD44780_SetCursor(0, 0);
 HD44780_PrintStr("  Robot Ready    ");
 HAL_Delay(1000);
 HD44780_Clear();
  /* Force the initial state */
 current_dist_cm = DIST_SAFE_CM;
 system_state = STATE_SAFE;
 uint32_t sensor_trigger_tick = HAL_GetTick();
  while (1)
 {
     // Ping the sensor every 80ms
     if (HAL_GetTick() - sensor_trigger_tick >= LOOP_PERIOD_MS)
     {
         HCSR04_Trigger();
         sensor_trigger_tick = HAL_GetTick();
     }

     // 2. Check if the sensor got stuck/unplugged
     HCSR04_TimeoutCheck();

     // 3. State transition logic
     if (current_dist_cm <= DIST_DANGER_CM) {
         system_state = STATE_DANGER;
     }
     else if (current_dist_cm >= DIST_SAFE_CM) {
         system_state = STATE_SAFE;
     }
     else {
         system_state = STATE_WARNING;
     }

     // 4. State logic
     switch (system_state)
     {
         case STATE_DANGER:
             Motor_Stop();
             TIM2->CCR1 = MAX_PWM_SPEED; /* Bật Max LED */
             break;

         case STATE_SAFE:
             Motor_Forward(MAX_PWM_SPEED);
             TIM2->CCR1 = 0;             /* Tắt LED */
             break;

         case STATE_WARNING:
             {
                 uint32_t dist_offset = current_dist_cm - DIST_DANGER_CM;
                 uint32_t dist_range  = DIST_SAFE_CM - DIST_DANGER_CM;

                 /* Logic tính toán theo lũy thừa bậc n */
                 uint32_t calc_pwm = (dist_offset * dist_offset * dist_offset * MAX_PWM_SPEED)
                                   / (dist_range * dist_range * dist_range);

                 Motor_Forward((uint16_t)calc_pwm);

                 /* LED cảnh báo sẽ nhạy sáng hơn, bừng lên rất nhanh khi sát vùng nguy hiểm */
                 uint32_t led_pwm = MAX_PWM_SPEED - calc_pwm;
                 TIM2->CCR1 = led_pwm;
                 break;
             }
     }

     /* 5. Update LCD */
     if (new_data_flag == 1)
     {
         new_data_flag = 0;
         LCD_UpdateDisplay(current_dist_cm, system_state);
     }
 }
}
static void LCD_UpdateDisplay(uint32_t dist_cm, ADAS_State_t state)
{
  char line[17];
  HD44780_SetCursor(0, 0);
  snprintf(line, sizeof(line), "Dist: %-4lu cm   ", (unsigned long)dist_cm);
  HD44780_PrintStr(line);
  HD44780_SetCursor(0, 1);
  switch (state)
  {
      case STATE_SAFE:
          HD44780_PrintStr("Status: Forward ");
          break;
      case STATE_WARNING:
          HD44780_PrintStr("Status: Slowing ");
          break;
      case STATE_DANGER:
          HD44780_PrintStr("Status: STOPPED ");
          break;
  }
}
/* ================================================================== */
/* SystemClock_Config – HSI → PLL → SYSCLK 84 MHz                   */
/* ================================================================== */
void SystemClock_Config(void)
{
RCC_OscInitTypeDef RCC_OscInitStruct = {0};
RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
RCC_OscInitStruct.OscillatorType      = RCC_OSCILLATORTYPE_HSI;
RCC_OscInitStruct.HSIState            = RCC_HSI_ON;
RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
RCC_OscInitStruct.PLL.PLLState        = RCC_PLL_ON;
RCC_OscInitStruct.PLL.PLLSource       = RCC_PLLSOURCE_HSI;
RCC_OscInitStruct.PLL.PLLM            = 16;
RCC_OscInitStruct.PLL.PLLN            = 336;
RCC_OscInitStruct.PLL.PLLP            = RCC_PLLP_DIV4;
RCC_OscInitStruct.PLL.PLLQ            = 7;
HAL_RCC_OscConfig(&RCC_OscInitStruct);
RCC_ClkInitStruct.ClockType      = RCC_CLOCKTYPE_HCLK  | RCC_CLOCKTYPE_SYSCLK
                                 | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
RCC_ClkInitStruct.SYSCLKSource   = RCC_SYSCLKSOURCE_PLLCLK;
RCC_ClkInitStruct.AHBCLKDivider  = RCC_SYSCLK_DIV1;
RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;   /* APB1 = 42 MHz → TIM clock = 84 MHz */
RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2);
}
static void MX_I2C1_Init(void)
{
__HAL_RCC_GPIOB_CLK_ENABLE();
__HAL_RCC_I2C1_CLK_ENABLE();
GPIO_InitTypeDef GPIO_InitStruct = {0};
GPIO_InitStruct.Pin       = GPIO_PIN_6 | GPIO_PIN_7;
GPIO_InitStruct.Mode      = GPIO_MODE_AF_OD;
GPIO_InitStruct.Pull      = GPIO_PULLUP;
GPIO_InitStruct.Speed     = GPIO_SPEED_FREQ_VERY_HIGH;
GPIO_InitStruct.Alternate = GPIO_AF4_I2C1;
HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
hi2c1.Instance             = I2C1;
hi2c1.Init.ClockSpeed      = 100000;
hi2c1.Init.DutyCycle       = I2C_DUTYCYCLE_2;
hi2c1.Init.OwnAddress1     = 0;
hi2c1.Init.AddressingMode  = I2C_ADDRESSINGMODE_7BIT;
hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLED;
hi2c1.Init.OwnAddress2     = 0;
hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLED;
hi2c1.Init.NoStretchMode   = I2C_NOSTRETCH_DISABLED;
HAL_I2C_Init(&hi2c1);
}
void Error_Handler(void)
{
__disable_irq();
while (1) {}
}


