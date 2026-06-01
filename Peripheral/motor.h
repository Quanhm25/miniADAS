#ifndef MOTOR_H
#define MOTOR_H
#include <stdint.h>
// Định nghĩa tốc độ PWM tối đa




#define MAX_PWM_SPEED 999
void PWM_Hardware_Init(void);
void Motor_SetSpeed(uint16_t speedLeft, uint16_t speedRight);
void Motor_Forward(uint16_t speed);
void Motor_Stop(void);
#endif /* MOTOR_H */



