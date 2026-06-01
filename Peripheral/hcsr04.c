#include "hcsr04.h"
typedef enum {
 ECHO_IDLE = 0,
 ECHO_WAIT_FALLING
} EchoState_t;
static volatile EchoState_t _echo_state = ECHO_IDLE;
static void delay_us(uint32_t us)
{
 uint32_t start = TIM5->CNT;
 while ((TIM5->CNT - start) < us);
}
void HCSR04_Init(void)
{
 /* Bật xung nhịp cho GPIOA, GPIOC, TIM3 (IC) và TIM5 (Delay) */
 RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN | RCC_AHB1ENR_GPIOCEN;
 RCC->APB1ENR |= RCC_APB1ENR_TIM3EN | RCC_APB1ENR_TIM5EN;
 /* 1. PC7 Trigger (Push-Pull Output) */
 HCSR04_TRIG_PORT->MODER   &= ~(3 << (HCSR04_TRIG_PIN * 2));
 HCSR04_TRIG_PORT->MODER   |=  (1 << (HCSR04_TRIG_PIN * 2));
 HCSR04_TRIG_PORT->BSRR     =  (1 << (HCSR04_TRIG_PIN + 16)); // Kéo mức LOW mặc định
 /* 2. PA6 Echo (Alternate Function TIM3_CH1) */
 HCSR04_ECHO_PORT->MODER   &= ~(3 << (HCSR04_ECHO_PIN * 2));
 HCSR04_ECHO_PORT->MODER   |=  (2 << (HCSR04_ECHO_PIN * 2));
 HCSR04_ECHO_PORT->AFR[0]  &= ~(0xF << (HCSR04_ECHO_PIN * 4));
 HCSR04_ECHO_PORT->AFR[0]  |=  (2 << (HCSR04_ECHO_PIN * 4)); // AF2 cho TIM3
 /* 3. TIM3 Input Capture - Cấu hình BOTHEDGE */
 TIM3->CR1   = 0;
 TIM3->PSC   = HCSR04_TIM_PSC; // 83 -> 1 tick = 1us
 TIM3->ARR   = HCSR04_TIM_ARR;
 TIM3->CCMR1 &= ~TIM_CCMR1_CC1S;
  // Noise resistant by ensuring trigger for 8 clock cycles
 TIM3->CCMR1 &= ~TIM_CCMR1_IC1F;
 TIM3->CCMR1 |= (3 << TIM_CCMR1_IC1F_Pos);
 TIM3->CCMR1 |= TIM_CCMR1_CC1S_0; // Map Channel 1 vào TI1
 /* Cấu hình bắt cả cạnh LÊN và XUỐNG (BOTHEDGE) */
 TIM3->CCER  |= (TIM_CCER_CC1P | TIM_CCER_CC1NP);
 TIM3->CCER  |= TIM_CCER_CC1E;
 TIM3->DIER  |= TIM_DIER_CC1IE; /* Bật interrupt của TIMER */
 NVIC_SetPriority(TIM3_IRQn, 0); /* Mức ưu tiên ngắt cao nhất */
 NVIC_EnableIRQ(TIM3_IRQn);
 TIM3->EGR   = TIM_EGR_UG; /* Ép Prescaler hoạt động */
 TIM3->CR1  |= TIM_CR1_CEN;
 /* 4. TIM5 dùng để chạy Delay US */
 TIM5->CR1 = 0;
 TIM5->PSC = 83;
 TIM5->ARR = 0xFFFFFFFF;
 TIM5->EGR = TIM_EGR_UG;
 TIM5->CR1 |= TIM_CR1_CEN;
}
/* Đổi hàm ngắt sang TIM3_IRQHandler */
void TIM3_IRQHandler(void)
{
  if (TIM3->SR & TIM_SR_CC1IF)
  {
      /* Đọc trạng thái vật lý của chân PA6 để biết xung đang LÊN hay XUỐNG */
      uint8_t pin_is_high = (HCSR04_ECHO_PORT->IDR & (1 << HCSR04_ECHO_PIN)) ? 1 : 0;

      if (pin_is_high)
      {
          /* CẠNH LÊN: Bắt đầu đo */
          TIM3->CNT = 0; /* Reset bộ đếm về 0 để đo độ rộng  */
          _echo_state = ECHO_WAIT_FALLING;
      }
      else
      {
          /* CẠNH XUỐNG: Kết thúc đo */
          if (_echo_state == ECHO_WAIT_FALLING)
          {
              uint32_t pulse_us = TIM3->CCR1;
              _echo_state = ECHO_IDLE;

              /* Tính toán khoảng cách (cm) */
              uint32_t cm = pulse_us / 58;
              if (cm < HCSR04_MIN_CM) cm = HCSR04_MIN_CM;
              if (cm > HCSR04_MAX_CM) cm = HCSR04_MAX_CM;
              HCSR04_DistanceCallback(cm);
          }
      }
      /* Xóa cờ ngắt sau khi xử lý xong */
      TIM3->SR = ~TIM_SR_CC1IF;
  }
}
void HCSR04_Trigger(void)
{
 HCSR04_TRIG_PORT->BSRR = (1 << (HCSR04_TRIG_PIN + 16));
 delay_us(2);
 HCSR04_TRIG_PORT->BSRR = (1 << HCSR04_TRIG_PIN);
 delay_us(10);
 HCSR04_TRIG_PORT->BSRR = (1 << (HCSR04_TRIG_PIN + 16));
}
void HCSR04_TimeoutCheck(void)
{
  if (_echo_state == ECHO_WAIT_FALLING && (TIM3->CNT > HCSR04_TIMEOUT_US))
  {
      _echo_state = ECHO_IDLE;
      HCSR04_DistanceCallback(HCSR04_MAX_CM);
  }
}


