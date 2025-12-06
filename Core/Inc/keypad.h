#ifndef __KEYPAD_H
#define __KEYPAD_H

#include "main.h"
#include "cmsis_os.h"
#include "stdbool.h"

// 按键定义
#define KEY_S1  0
#define KEY_S2  1
#define KEY_S3  2
#define KEY_S4  3
#define KEY_S5  4
#define KEY_S6  5
#define KEY_S7  6
#define KEY_S8  7
#define KEY_S9  8
#define KEY_S10 9
#define KEY_S11 10
#define KEY_S12 11
#define KEY_S13 12
#define KEY_S14 13
#define KEY_S15 14
#define KEY_S16 15

#define NO_KEY 255

// 按键长按时间定义 (毫秒)
#define LONG_PRESS_TIME 2000

// 低功耗时间定义（单位：毫秒）
#define POWER_SAVE_TIMEOUT     (10 * 60 * 1000)  // 10分钟无操作进入睡眠
#define POWER_WAKEUP_DEBOUNCE  50                // 唤醒按键消抖时间

// 按键事件
typedef enum {
    KEY_EVENT_NONE,      // 无事件
    KEY_EVENT_PRESS,     // 按键按下
    KEY_EVENT_RELEASE,   // 按键释放  
    KEY_EVENT_LONG_PRESS // 长按事件
} KeyEvent_t;

// 按键数据结构
typedef struct {
    uint8_t keyCode;    // 按键编码 (0-15)
    KeyEvent_t event;   // 按键事件类型
    uint32_t pressTime; // 按键按下时间(毫秒)
} KeyData_t;

// 按键状态结构
typedef struct {
    uint8_t currentState;   // 当前按键状态
    uint8_t lastState;      // 上一次按键状态
    uint32_t pressStartTime;// 按键开始时间戳
    uint8_t longPressSent;  // 长按事件已发送标志
} KeyState_t;

// 模式定义
typedef enum {
    MODE1 = 0,  // 基础控制模式：上升、下降、左转、右转
    MODE2 = 1   // 平移控制模式：向前、向后、向左、向右
} OperationMode_t;

// 锁定状态
typedef enum {
    LOCKED = 0,   // 遥控器锁定状态
    UNLOCKED = 1  // 遥控器解锁状态
} LockStatus_t;

// 连接状态
typedef enum {
    DISCONNECTED = 0, // 无人机未连接
    CONNECTED = 1     // 无人机已连接
} ConnectionStatus_t;

// 低功耗模式定义
typedef enum {
    POWER_MODE_NORMAL = 0,    // 正常模式
    POWER_MODE_SLEEP = 1,     // 睡眠模式（OLED息屏）
    POWER_MODE_STANDBY = 2    // 待机模式（可选的深度睡眠）
} PowerMode_t;

// 全局变量声明
extern OperationMode_t currentMode;
extern LockStatus_t lockStatus;
extern ConnectionStatus_t droneConnection;  // 添加连接状态
extern uint8_t signalStrength;              // 添加信号强度

//低功耗全局变量声明
extern PowerMode_t currentPowerMode;
extern uint32_t lastActivityTime;
extern bool oledDisplayState;  // OLED显示状态


// 函数声明
void UpdateConnectionDisplay(void);
void UpdateSignalDisplay(void);

// 全局变量声明
extern OperationMode_t currentMode;
extern LockStatus_t lockStatus;

// 函数声明
void Keypad_Init(void);
uint8_t Keypad_Scan(void);
//低功耗函数声明
void EnterPowerSaveMode(void);
void ExitPowerSaveMode(void);
void UpdateActivityTime(void);
bool CheckPowerSaveTimeout(void);
void HandleWakeupInterrupt(void);

#endif

