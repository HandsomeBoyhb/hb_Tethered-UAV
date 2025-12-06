#include "motor.h"
#include "main.h"
#include "usart.h"

uint8_t tx_buf[16];
uint8_t rx_buf[8];

/* 串口发送函数 */
void UART_Send(uint8_t *data, uint16_t len)
{
    HAL_UART_Transmit(&huart3, data, len, 100);
}

/* 电机使能 */
void Motor_Enable(void)
{
    uint8_t cmd[6] = {ADDR, 0xF3, 0xAB, 0x01, 0x00, CHECKSUM};
    UART_Send(cmd, 6);
    HAL_Delay(10);
}

/* 电机失能 */
void Motor_Disable(void)
{
    uint8_t cmd[6] = {ADDR, 0xF3, 0xAB, 0x00, 0x00, CHECKSUM};
    UART_Send(cmd, 6);
    HAL_Delay(10);
}

/* 电机停止 */
void Motor_Stop(void)
{
    uint8_t cmd[5] = {ADDR, 0xFE, 0x98, 0x00, CHECKSUM};
    UART_Send(cmd, 5);
}

/* 通用力矩模式控制 */
void Motor_Torque_Control(uint8_t dir, uint16_t slope, uint16_t current)
{
    uint8_t cmd[9];
    cmd[0] = ADDR;
    cmd[1] = 0xF5;            // 力矩模式命令
    cmd[2] = dir;             // 方向: 0=正向(收线), 1=反向(放线)
    cmd[3] = (slope >> 8) & 0xFF;
    cmd[4] = slope & 0xFF;    // 力矩斜率
    cmd[5] = (current >> 8) & 0xFF;
    cmd[6] = current & 0xFF;  // 力矩目标值
    cmd[7] = 0x00;            // 保留同步标志
    cmd[8] = CHECKSUM;
    UART_Send(cmd, 9);
}

/* 起飞和上升阶段力矩控制 - 放线 */
void Motor_Takeoff_Ascend_Torque(void)
{
    // 起飞和上升阶段需要放线，使用反向力矩
    Motor_Torque_Control(1, 1000, TORQUE_TAKEOFF_ASCEND);
}

/* 水平移动阶段力矩控制 - 保持张力 */
void Motor_Horizontal_Torque(void)
{
    // 前后左右阶段保持适当张力，使用正向力矩
    Motor_Torque_Control(0, 1000, TORQUE_HORIZONTAL);
}

/* 下降和降落阶段力矩控制 - 收线 */
void Motor_Descend_Land_Torque(void)
{
    // 下降和降落阶段需要收线，使用正向力矩
    Motor_Torque_Control(0, 1000, TORQUE_DESCEND_LAND);
}

/* 读取电机电流 */
uint16_t Motor_ReadCurrent(void)
{
    uint8_t cmd[3] = {ADDR, 0x27, CHECKSUM};
    UART_Send(cmd, 3);

    HAL_UART_Receive(&huart3, rx_buf, 5, 50); // 接收固定5字节
    if (rx_buf[0] == ADDR && rx_buf[1] == 0x27)
    {
        uint16_t val = (rx_buf[2] << 8) | rx_buf[3];
        return val;  // 单位 mA
    }
    return 0;
}

/* 速度控制命令 (0xF6) */
void Motor_Speed(uint8_t dir, uint16_t slope, uint16_t rpm)
{
    uint16_t rpm10 = rpm * 10;
    uint8_t cmd[9] = {
        ADDR,
        0xF6,
        dir,
        (slope >> 8) & 0xFF,
        slope & 0xFF,
        (rpm10 >> 8) & 0xFF,
        rpm10 & 0xFF,
        0x00,
        CHECKSUM
    };
    UART_Send(cmd, 9);
}

