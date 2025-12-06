#ifndef MOTOR_MOTOR_H_
#define MOTOR_MOTOR_H_

#include "main.h"

#define ADDR          0x01	 //设备地址		
#define CHECKSUM      0x6B   //校验和

/* 力矩值单位 mA */
#define TORQUE_SMALL  60     // 微调模式下的力矩
#define TORQUE_LIMIT  1500   // 最大安全电流限制
#define TENSION_LOW   300    // 张力下限
#define TENSION_HIGH  600    // 张力上限
#define RUN_RPM_SMALL 5      // 微调速度 (RPM)
#define RUN_RPM_LARGE 15     // 正常运转速度
#define ACCEL_RATE    1000   // 加速度RPM/s

/*不同飞行阶段的力矩值 */
#define TORQUE_TAKEOFF_ASCEND   800   // 起飞和上升阶段力矩
#define TORQUE_HORIZONTAL       400   // 前后左右阶段力矩  
#define TORQUE_DESCEND_LAND     600   // 下降和降落阶段力矩

void UART_Send(uint8_t *data, uint16_t len);
void Motor_Enable(void);
void Motor_Disable(void);
void Motor_Stop(void);
void Motor_Torque_Control(uint8_t dir, uint16_t slope, uint16_t current);
void Motor_Takeoff_Ascend_Torque(void);
void Motor_Horizontal_Torque(void);
void Motor_Descend_Land_Torque(void);
uint16_t Motor_ReadCurrent(void);
void Motor_Speed(uint8_t dir, uint16_t slope, uint16_t rpm);

#endif /* MOTOR_MOTOR_H_ */
