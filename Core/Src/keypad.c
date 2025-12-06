#include "keypad.h"

// 全局变量定义
OperationMode_t currentMode = MODE1;//当前模式初始为模式一
LockStatus_t lockStatus = LOCKED;//遥控器锁定状态
ConnectionStatus_t droneConnection = DISCONNECTED;  // 默认未连接
uint8_t signalStrength = 0;                        // 默认信号强度为0

static const uint8_t keyMap[4][4] = {
    {KEY_S1,  KEY_S2,  KEY_S3,  KEY_S4},
    {KEY_S5,  KEY_S6,  KEY_S7,  KEY_S8},
    {KEY_S9,  KEY_S10, KEY_S11, KEY_S12},
    {KEY_S13, KEY_S14, KEY_S15, KEY_S16}
};

void Keypad_Init(void)
{
    // 初始化GPIO已经在MX_GPIO_Init中完成
    // 这里不需要额外的初始化
}

uint8_t Keypad_Scan(void)
{
    uint8_t row;
    uint8_t key = NO_KEY;
    
    // 扫描每一行
    for(row = 0; row < 4; row++) {
        // 设置当前行为低电平，其他行为高电平
        switch(row) {
            case 0:
                HAL_GPIO_WritePin(R1_GPIO_Port, R1_Pin, GPIO_PIN_RESET);
                HAL_GPIO_WritePin(R2_GPIO_Port, R2_Pin, GPIO_PIN_SET);
                HAL_GPIO_WritePin(R3_GPIO_Port, R3_Pin, GPIO_PIN_SET);
                HAL_GPIO_WritePin(R4_GPIO_Port, R4_Pin, GPIO_PIN_SET);
                break;
            case 1:
                HAL_GPIO_WritePin(R1_GPIO_Port, R1_Pin, GPIO_PIN_SET);
                HAL_GPIO_WritePin(R2_GPIO_Port, R2_Pin, GPIO_PIN_RESET);
                HAL_GPIO_WritePin(R3_GPIO_Port, R3_Pin, GPIO_PIN_SET);
                HAL_GPIO_WritePin(R4_GPIO_Port, R4_Pin, GPIO_PIN_SET);
                break;
            case 2:
                HAL_GPIO_WritePin(R1_GPIO_Port, R1_Pin, GPIO_PIN_SET);
                HAL_GPIO_WritePin(R2_GPIO_Port, R2_Pin, GPIO_PIN_SET);
                HAL_GPIO_WritePin(R3_GPIO_Port, R3_Pin, GPIO_PIN_RESET);
                HAL_GPIO_WritePin(R4_GPIO_Port, R4_Pin, GPIO_PIN_SET);
                break;
            case 3:
                HAL_GPIO_WritePin(R1_GPIO_Port, R1_Pin, GPIO_PIN_SET);
                HAL_GPIO_WritePin(R2_GPIO_Port, R2_Pin, GPIO_PIN_SET);
                HAL_GPIO_WritePin(R3_GPIO_Port, R3_Pin, GPIO_PIN_SET);
                HAL_GPIO_WritePin(R4_GPIO_Port, R4_Pin, GPIO_PIN_RESET);
                break;
        }
        
        // 短暂延时确保电平稳定
        osDelay(5);
        
        // 读取列状态
        if(HAL_GPIO_ReadPin(C1_GPIO_Port, C1_Pin) == GPIO_PIN_RESET) {
            key = keyMap[row][0];
            break;
        }
        if(HAL_GPIO_ReadPin(C2_GPIO_Port, C2_Pin) == GPIO_PIN_RESET) {
            key = keyMap[row][1];
            break;
        }
        if(HAL_GPIO_ReadPin(C3_GPIO_Port, C3_Pin) == GPIO_PIN_RESET) {
            key = keyMap[row][2];
            break;
        }
        if(HAL_GPIO_ReadPin(C4_GPIO_Port, C4_Pin) == GPIO_PIN_RESET) {
            key = keyMap[row][3];
            break;
        }
    }
    
    // 所有行恢复高电平
    HAL_GPIO_WritePin(R1_GPIO_Port, R1_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(R2_GPIO_Port, R2_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(R3_GPIO_Port, R3_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(R4_GPIO_Port, R4_Pin, GPIO_PIN_SET);
    
    return key;
}

