/**
* @file  hcsr04.h
* @brief HC-SR04 driver – Input Capture + Interrupt
*        SYSCLK 84 MHz | TIM2 (Echo IC)
*        TRIG: PA1 | ECHO: PA0
*/
#ifndef HCSR04_H
#define HCSR04_H
#include "stm32f4xx.h"
#include <stdint.h>
#define HCSR04_TRIG_PORT    GPIOC
#define HCSR04_TRIG_PIN     7       //(PC7)
#define HCSR04_ECHO_PORT    GPIOA
#define HCSR04_ECHO_PIN     6       //(PA6)
#define HCSR04_TIM_PSC      83         /* 84 MHz / 84 = 1 MHz -> 1µs */
#define HCSR04_TIM_ARR      0xFFFF;    /*2^16-1*/
#define HCSR04_MIN_CM       2
#define HCSR04_MAX_CM       400
#define HCSR04_TIMEOUT_US   38000
void HCSR04_Init(void);
void HCSR04_Trigger(void);
void HCSR04_TimeoutCheck(void);




void HCSR04_DistanceCallback(uint32_t dist_cm);
#endif /* HCSR04_H */


