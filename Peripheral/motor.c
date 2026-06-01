#include "motor.h"
#include "stm32f4xx.h"




void PWM_Hardware_Init(void) {
	RCC->AHB1ENR |= ((1<<0) | (1<<1)); //GPIOA, GPIOB
	RCC->APB1ENR |= (1<<0); //TIM2
	RCC->APB2ENR |= (1<<0); //TIM1

	//L298N config mode
	GPIOB->MODER &= ~((3<<20) | (3<<26) | (3<<28) | (3<<30));
	GPIOB->MODER |= ((1<<20) | (1<<26) | (1<<28) | (3<<30)); //General Output

	//PA5 PWM mode, AF1 TIM2_CH1
	GPIOA->MODER &= ~(3<<10);
	GPIOA->MODER |= (2<<10);
	GPIOA->AFR[0] &= ~(0xF<<20);
	GPIOA->AFR[0] |= (1<<20);

	//PA8, PA9 EnA va EnB, AF1 TIM1_CH1 VA TIM1_CH2
	GPIOA->MODER &= ~((3<<16) | (3<<18));
	GPIOA->MODER |= ((1<<16) | (1<<18));
	GPIOA->AFR[1] &= ~((0xF<<0) | (0xF<<4));
	GPIOA->AFR[1] |= ((1<<0) | (1<<4));

	//TIM2 Config
	TIM2->PSC = 83;
	TIM2->ARR = 999;
	TIM2->CCMR1 &= ~(7<<4); //Clear bit capture compare mode register
	TIM2->CCMR1 |= ((6<<4) | (1<<3)); //Mode 1 and Preload
	TIM2->CCER |= (1<<0);
	TIM2->CCR1 = 0;
	TIM2->CR1 |= ((1<<0) | (1<<7)); //Enable and Buffer for ARR

	//TIM1 Config
	TIM1->PSC = 83;
	TIM1->ARR = 999;
	TIM1->CCMR1 &= ~((3<<4) | (3<<12));
	TIM1->CCMR1 |= ((6<<4) | (1<<3) | (6<<12) | (1<<11));
	TIM1->CCER |= ((1<<0) | (1<<4));
	TIM1->BDTR |= (1<<15); //Main output enable - cleared asynchronously by hardware as soon as the break input is active
	TIM1->CR1 |= ((1<<0) | (1<<7)); //Enable and Buffer for ARR
}


void Motor_SetSpeed(uint16_t speedLeft, uint16_t speedRight)
{
   /* Giới hạn tốc độ không vượt quá MAX_PWM_SPEED */
   TIM1->CCR1 = (speedLeft > MAX_PWM_SPEED) ? MAX_PWM_SPEED : speedLeft;
   TIM1->CCR2 = (speedRight > MAX_PWM_SPEED) ? MAX_PWM_SPEED : speedRight;
}
void Motor_Forward(uint16_t speed)
{
   GPIOB->ODR |=  GPIO_ODR_OD10;
   GPIOB->ODR &= ~GPIO_ODR_OD15;
   GPIOB->ODR &= ~GPIO_ODR_OD14;
   GPIOB->ODR |=  GPIO_ODR_OD13;
   Motor_SetSpeed(speed, speed);
}
void Motor_Stop(void)
{
   GPIOB->ODR &= ~GPIO_ODR_OD10;
   GPIOB->ODR &= ~GPIO_ODR_OD15;
   GPIOB->ODR &= ~GPIO_ODR_OD13;
   GPIOB->ODR &= ~GPIO_ODR_OD14;
   Motor_SetSpeed(0, 0);
}


