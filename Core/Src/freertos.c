/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : freertos.c
  * Description        : Code for freertos applications
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"
#include <stdint.h>  
#include <string.h>  

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "keypad.h"
#include "oled.h"
#include "oledfont.h" 
#include "CommManager.h"
#include "drv_SDI.h"
#include "usart.h" 
#include "iwdg.h"
#include "motor.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
static int hit=0;// 起飞状态标记：0-未起飞，2-已起飞
uint8_t reciver_date[38]={0};// 接收数据缓冲区

// 低功耗相关全局变量
PowerMode_t currentPowerMode = POWER_MODE_NORMAL;  // 初始为正常模式
uint32_t lastActivityTime = 0;                    // 最后活动时间
bool oledDisplayState = true;                     // OLED显示状态
bool displayContentSaved = false;                 // 显示内容是否已保存
uint32_t lastWakeupCheck = 0;                     // 上次唤醒检查时间
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */

/* USER CODE END Variables */
/* Definitions for defaultTask */
osThreadId_t defaultTaskHandle;
const osThreadAttr_t defaultTask_attributes = {
  .name = "defaultTask",
  .stack_size = 1024,
  .priority = (osPriority_t) osPriorityNormal,
};
osThreadId_t KeypadTaskHandle;
osThreadId_t KeypadTaskHandle;
const osThreadAttr_t KeypadTask_attributes = {
  .name = "KeypadTask",
  .stack_size = 384,  
  .priority = (osPriority_t) osPriorityAboveNormal,
};
osThreadId_t SystemTaskHandle;
const osThreadAttr_t SystemTask_attributes = {
  .name = "SystemTask",
  .stack_size = 1024,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */
void Keypad_Task(void *argument);
void System_Task(void *argument);
void CommManager_Task(void *argument);
// 飞行控制相关函数声明
void SendFlightControlCommand(float roll, float pitch, float yaw, float throttle, uint8_t mode);
void SendPositionCommand(double lat, double lon, float alt, float yaw);
void QueryDroneStatus(void);
void SetFlightMode(uint8_t mode);
void SendControlCommandBasedOnKey(uint8_t key, OperationMode_t mode);
void SendStopCommand(void);
/* USER CODE END FunctionPrototypes */

void StartDefaultTask(void *argument);

void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/**
  * @brief  FreeRTOS initialization
  * @param  None
  * @retval None
  */
void MX_FREERTOS_Init(void) {
  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* creation of defaultTask */
  defaultTaskHandle = osThreadNew(StartDefaultTask, NULL, &defaultTask_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
  KeypadTaskHandle = osThreadNew(Keypad_Task, NULL, &KeypadTask_attributes);
  SystemTaskHandle = osThreadNew(System_Task, NULL, &SystemTask_attributes);
//  Mode1TaskHandle = osThreadNew(Mode1_Task, NULL, &Mode1Task_attributes);
//  Mode2TaskHandle = osThreadNew(Mode2_Task, NULL, &Mode2Task_attributes);
//  DisplayTaskHandle = osThreadNew(Display_Task, NULL, &DisplayTask_attributes);
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_EVENTS */
  /* add events, ... */
  /* USER CODE END RTOS_EVENTS */

}

/* USER CODE BEGIN Header_StartDefaultTask */
/**
  * @brief  Function implementing the defaultTask thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_StartDefaultTask */
void StartDefaultTask(void *argument)
{
  /* USER CODE BEGIN StartDefaultTask */
  /* Infinite loop */
  for(;;)
  {
    osDelay(1000);
  }
  /* USER CODE END StartDefaultTask */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */
/**
  * @brief 发送简单命令到无人机
  * @param cmd 命令代码（参考SimpleCommand枚举）
  * @retval None
  */
void SendSimpleCommand(uint8_t cmd)
{
    comm_send_message(MSG_SIMPLE_CMD, &cmd, sizeof(cmd));
}

/**
  * @brief 通信管理任务（预留）
  * @param argument 任务参数
  * @retval None
  */
void CommManager_Task(void *argument)
{
    for(;;) {
        // 通信管理功能预留
        osDelay(10);
    }
}

/**
  * @brief 发送飞行控制命令
  * @param roll 横滚控制量 [-1.0, 1.0]
  * @param pitch 俯仰控制量 [-1.0, 1.0]
  * @param yaw 偏航控制量 [-1.0, 1.0]
  * @param throttle 油门控制量 [0.0, 1.0]
  * @param mode 控制模式
  * @retval None
  */
void SendFlightControlCommand(float roll, float pitch, float yaw, float throttle, uint8_t mode)
{
    FlightControlMsg ctrl_msg;
    ctrl_msg.roll = roll;
    ctrl_msg.pitch = pitch;
    ctrl_msg.yaw = yaw;
    ctrl_msg.throttle = throttle;
    ctrl_msg.mode = mode;
    memset(ctrl_msg.reserved, 0, sizeof(ctrl_msg.reserved));
    
    comm_send_message(MSG_VIRTUAL_RC, &ctrl_msg, sizeof(ctrl_msg));
}

/**
  * @brief 发送位置控制命令
  * @param lat 目标纬度
  * @param lon 目标经度
  * @param alt 目标高度
  * @param yaw 目标偏航角
  * @retval None
  */
void SendPositionCommand(double lat, double lon, float alt, float yaw)
{
    PositionControlMsg pos_msg;
    pos_msg.lat = lat;
    pos_msg.lon = lon;
    pos_msg.alt = alt;
    pos_msg.yaw = yaw;
    pos_msg.tolerance = 1.0f;
    
    comm_send_message(MSG_POSITION_CTRL, &pos_msg, sizeof(pos_msg));
}

/**
  * @brief 查询无人机状态
  * @retval None
  */
void QueryDroneStatus(void)
{
    comm_send_message(MSG_QUERY_STATUS, NULL, 0);
}

/**
  * @brief 设置飞行模式
  * @param mode 飞行模式（参考FlightMode枚举）
  * @retval None
  */
void SetFlightMode(uint8_t mode)
{
    comm_send_message(MSG_FLIGHT_MODE, &mode, sizeof(mode));
}

/**
  * @brief 根据按键发送控制命令
  * @param key 按键编码
  * @param mode 当前操作模式
  * @retval None
  */
void SendControlCommandBasedOnKey(uint8_t key, OperationMode_t mode)
{
    float roll = 0, pitch = 0, yaw = 0, throttle = 0.5f; 
    uint8_t simple_cmd = 0;
    
    if(mode == MODE1) {
		// 模式1：上升、下降、左转、右转
        switch(key) {
            case KEY_S2: //上升
                throttle = 0.7f;
                simple_cmd = CMD_UP;
                break;
            case KEY_S6: //下降
                throttle = 0.3f;
                simple_cmd = CMD_DOWN;
                break;
            case KEY_S5: //左转
                yaw = -0.5f;
                simple_cmd = CMD_TURN_LEFT;
                break;
            case KEY_S7: //右转
                yaw = 0.5f;
                simple_cmd = CMD_TURN_RIGHT;
                break;
            default:
                
                return;// 非动作按键，直接返回
        }
    } else {
		// 模式2：向前、向后、向左、向右
        switch(key) {
            case KEY_S2: //向前
                pitch = 0.5f;
                simple_cmd = CMD_FORWARD;
                break;
            case KEY_S6: //向后
                pitch = -0.5f;
                simple_cmd = CMD_BACKWARD;
                break;
            case KEY_S5: //向左
                roll = -0.5f;
                simple_cmd = CMD_LEFT_FORWARD;
                break;
            case KEY_S7: //向右
                roll = 0.5f;
                simple_cmd = CMD_RIGHT_FORWARD;
                break;
            default:
                
                return;// 非动作按键，直接返回
        }
    }
    
    // 发送飞行控制命令
    SendFlightControlCommand(roll, pitch, yaw, throttle, mode);
    
     // 发送简单命令（用于状态同步）
    if (simple_cmd != 0) {
        SendSimpleCommand(simple_cmd);
    }
}

/**
  * @brief 发送停止命令
  * @retval None
  */
void SendStopCommand(void)
{
    SendFlightControlCommand(0, 0, 0, 0.5f, currentMode); 
}

/**
  * @brief 清除动作显示区域
  * @retval None
  */
void ClearActionDisplayArea(void)
{
	// 清除OLED中间区域（页2-7，列0-95），保留顶部状态栏和右侧信息栏
    for(uint8_t page = 2; page < 8; page++) {
        OLED_Set_Pos(0, page);
        for(uint8_t col = 0; col < 96; col++) { 
            OLED_WR_DATA(0x00);
        }
    }
}

/**
  * @brief 显示当前动作和发送控制指令
  * @param key 按键编码
  * @param mode 当前操作模式
  * @retval None
  */
void DisplayAction(uint8_t key, OperationMode_t mode)
{
    if(mode == MODE1) {
		// 模式1：基础飞行控制
        switch(key) { 
            case KEY_S2: // 上升（首次按下为起飞，之后为上升）
                OLED_ShowIcon(52, 4, Takeoff, 16, 32, 0);     // 显示起飞图标
                OLED_ShowChinese16x16(46, 2, 0, 0);          // 显示"上"
                OLED_ShowChinese16x16(62, 2, 4, 0);          // 显示"升"
				if(hit==0){
				// 首次按下：起飞阶段
                Motor_Takeoff_Ascend_Torque();            // 启动电机放线
                // 发送起飞指令序列
				for(int i=0;i<5;i++){
				osDelay(5);
				protocol_send_acfly_message(1008,
											0,0,0,0,
											0,0,100,0,
											0,0,0,0);}
				hit=2;}// 标记为已起飞状态
				else
				{
                 // 后续按下：上升阶段
                 Motor_Takeoff_Ascend_Torque();            // 继续放线
                 // 发送上升指令序列
				for(int i=0;i<5;i++){
				osDelay(5);
				protocol_send_acfly_message(1020,
											0,0,50,0,
											0,0,5,0,
											0,0,0,0);}
				}
                break;
            case KEY_S6: // 下降
                OLED_ShowIcon(52, 4, Landfall, 16, 32, 0);    // 显示降落图标
                OLED_ShowChinese16x16(46, 2, 1, 0);          // 显示"下"
                OLED_ShowChinese16x16(62, 2, 5, 0);          // 显示"降"
                Motor_Descend_Land_Torque();                  // 启动电机收线
                // 发送下降指令序列
				for(int i=0;i<5;i++){
				osDelay(5);
				protocol_send_acfly_message(1020,
											0,0,-50,0,
											5,5,5,5,
											0,0,0,0);}
                break;
            case KEY_S5: // 左转
                OLED_ShowIcon(48, 4, Turnleft, 32, 16, 0);    // 显示左转图标
                OLED_ShowChinese16x16(46, 2, 2, 0);          // 显示"左"
                OLED_ShowChinese16x16(62, 2, 6, 0);          // 显示"转"
                Motor_Horizontal_Torque();                    // 水平移动张力控制
                // 发送左转指令序列
				for(int i=0;i<5;i++){
				osDelay(5);
				protocol_send_acfly_message(1030,
											0,0,0,25,
											0,0,0,0,
											0,0,0,0);}
                break;
            case KEY_S7: // 右转
                OLED_ShowIcon(54, 3, Turnright, 16, 32, 0);   // 显示右转图标
                OLED_ShowChinese16x16(46, 2, 3, 0);          // 显示"右"
                OLED_ShowChinese16x16(62, 2, 6, 0);          // 显示"转"
                Motor_Horizontal_Torque();                    // 水平移动张力控制
                // 发送右转指令序列
				for(int i=0;i<5;i++){
				osDelay(5);
				protocol_send_acfly_message(1030,
											0,0,0,-20,
											0,0,0,0,
											0,0,0,0);}
                break;
        }
    } else {
		// 模式2：平移控制
        switch(key) {
            case KEY_S2: // 向前
                OLED_ShowIcon(52, 4, Forward, 16, 32, 0);     // 显示向前图标
                OLED_ShowChinese16x16(46, 2, 9, 0);          // 显示"向"
                OLED_ShowChinese16x16(62, 2, 7, 0);          // 显示"前"
                Motor_Horizontal_Torque();                    // 水平移动张力控制
                // 发送向前指令序列
				for(int i=0;i<5;i++){
				osDelay(5);
				protocol_send_acfly_message(1024,
											0,-50,0,0,
											5,5,0,0,
											0,0,0,0);}
                break;
            case KEY_S6: // 向后
				OLED_ShowIcon(52, 4, Backward, 16, 32, 0);    // 显示向后图标
                OLED_ShowChinese16x16(46, 2, 9, 0);          // 显示"向"
                OLED_ShowChinese16x16(62, 2, 8, 0);          // 显示"后"
                Motor_Horizontal_Torque();                    // 水平移动张力控制
                // 发送向后指令序列
				for(int i=0;i<5;i++){
				osDelay(5);
				protocol_send_acfly_message(1024,
											0,50,0,0,
											5,5,0,0,
											0,0,0,0);}
                break;
            case KEY_S5: // 向左
                OLED_ShowIcon(44, 4, Toleft, 32, 16, 0);     // 显示向左图标
                OLED_ShowChinese16x16(46, 2, 9, 0);          // 显示"向"
                OLED_ShowChinese16x16(62, 2, 2, 0);          // 显示"左"
                Motor_Horizontal_Torque();                    // 水平移动张力控制
                // 发送向左指令序列
				for(int i=0;i<5;i++){
				osDelay(5);
				protocol_send_acfly_message(1024,	
											50,0,0,0,
											5,5,0,0,
											0,0,0,0);}
                break;
            case KEY_S7: // 向右
                OLED_ShowIcon(44, 4, Toright, 32, 16, 0);    // 显示向右图标
                OLED_ShowChinese16x16(46, 2, 9, 0);          // 显示"向"
                OLED_ShowChinese16x16(62, 2, 3, 0);          // 显示"右"
                Motor_Horizontal_Torque();                    // 水平移动张力控制
			    // 发送向右指令序列
				for(int i=0;i<5;i++){
				osDelay(5);
				protocol_send_acfly_message(1024,
											-50,0,0,0,
											5,5,0,0,
											0,0,0,0);}
                break;
        }
    }
}

/**
  * @brief 处理模式切换
  * @retval None
  */
void HandleModeSwitch(void)
{
    // 切换操作模式
    currentMode = (currentMode == MODE1) ? MODE2 : MODE1;
    OLED_Clear();  // 清屏重新绘制
    
     // 重新绘制所有静态内容
    if(currentMode == MODE1) 
		{
		// 显示"模式一"
        OLED_ShowChinese16x16(0, 0, 10, 0);  // "模"
        OLED_ShowChinese16x16(16, 0, 11, 0); // "式"
        OLED_ShowChinese16x16(32, 0, 12, 0); // "一"
		
		} 
	else{
		// 显示"模式二"
        OLED_ShowChinese16x16(0, 0, 10, 0);  // "模"
        OLED_ShowChinese16x16(16, 0, 11, 0); // "式"
        OLED_ShowChinese16x16(32, 0, 13, 0); // "二"
		
		}
    
	// 显示锁定状态图标
    if(lockStatus == LOCKED) {
        OLED_ShowIcon(54, 0, Lock, 16, 16, 0);      // 显示锁定图标
    } else {
        OLED_ShowIcon(54, 0, Unlock, 16, 16, 0);    // 显示解锁图标
    }  
	
    // 重新显示信号和连接状态
    OLED_ShowIcon(94, 0, SignalIcon, 16, 16, 0);  // 信号图标
    OLED_ShowChar(110, 0, ':', 16, 0);            // 分隔符
    UpdateSignalDisplay();                         // 更新信号强度显示
    UpdateConnectionDisplay();                     // 更新连接状态显示
    
	// 发送模式切换完成命令
    SendSimpleCommand(CMD_MODE_SWITCH_COMPLETE);
    
    // 设置飞行模式
    SetFlightMode(currentMode);
}

/**
  * @brief 处理锁定/解锁切换
  * @retval None
  */
void HandleLockToggle(void)
{
    // 保存旧状态用于比较
    LockStatus_t oldLockStatus = lockStatus; 
    
	// 切换锁定状态
    lockStatus = (lockStatus == LOCKED) ? UNLOCKED : LOCKED;
    
    if(lockStatus == LOCKED) {
		// 锁定状态
        OLED_ShowIcon(54, 0, Lock, 16, 16, 0);  // 显示锁定图标
        SendStopCommand();                        // 发送停止命令
        Motor_Stop();                             // 停止电机
		
        if (oldLockStatus == UNLOCKED) {
		// 从解锁状态切换到锁定，发送锁定指令序列
		for(int i=0;i<5;i++){
				osDelay(5);
				protocol_send_acfly_message(1100,
											0,0,0,0,
											0,0,0,0,
											0,0,0,0);}
//		HAL_UART_Transmit(&huart3,&d,1,5);
        }
    } else {
        // 解锁状态
        OLED_ShowIcon(54, 0, Unlock, 16, 16, 0);  // 显示解锁图标
        Motor_Enable();                             // 使能电机
        SendSimpleCommand(CMD_UNLOCK_COMPLETE);     // 发送解锁完成命令

        if (oldLockStatus == LOCKED) {
			
			// 从锁定状态切换到解锁，设置默认力矩
            Motor_Horizontal_Torque();  // 水平移动张力控制

        }
    }
}

/**
  * @brief 连续动作管理结构体
  */
typedef struct {
    uint8_t active_key;          // 当前激活的按键
    TickType_t last_send_time;   // 上次发送时间
    bool is_active;              // 是否处于激活状态          
} ContinuousActionManager;

// 连续动作管理器实例
static ContinuousActionManager action_mgr = {NO_KEY, 0, false};

/**
  * @brief 处理连续动作（预留功能）
  * @param currentTime 当前系统时间
  * @retval None
  */
void HandleContinuousAction(TickType_t currentTime)
{
    if (!action_mgr.is_active || action_mgr.active_key == NO_KEY) {
        return;// 没有激活的连续动作
    }
    
     // 每秒发送一次控制命令（连续动作模式）
    if (currentTime - action_mgr.last_send_time >= 1000) {
        
        SendControlCommandBasedOnKey(action_mgr.active_key, currentMode);
        action_mgr.last_send_time = currentTime;
    }
}

/**
  * @brief 系统主任务 - 集成显示、通信、控制、低功耗功能
  * @param argument 任务参数
  * @retval None
  */
void System_Task(void *argument)
{
     uint8_t currentKey;                    // 当前扫描到的按键
    uint32_t currentTime;                  // 当前系统时间
    uint8_t pressedActionKey = NO_KEY;     // 当前按下的动作键
    uint32_t lastSignalUpdate = 0;         // 上次信号更新时间
    uint32_t lastConnectionCheck = 0;      // 上次连接检查时间
    uint32_t lastOLEDKeepAlive = 0;        // 上次OLED保活时间
    uint8_t oledRefreshCounter = 0;        // OLED刷新计数器
    uint32_t lastFeedDog = 0;              // 上次喂狗时间
    
    // 硬件初始化
    Keypad_Init();    // 初始化键盘
    OLED_Init();      // 初始化OLED显示屏
    OLED_Clear();     // 清屏
    
    // 初始化静态显示内容
    // 显示模式标识（左上角）
    OLED_ShowChinese16x16(0, 0, 10, 0);  // "模"
    OLED_ShowChinese16x16(16, 0, 11, 0); // "式"
    OLED_ShowChinese16x16(32, 0, 12, 0); // "一"（初始为模式一）
    
    // 显示锁定状态（中间顶部）
    OLED_ShowIcon(54, 0, Lock, 16, 16, 0);
    
    // 信号强度显示（右上角）
    OLED_ShowIcon(94, 0, SignalIcon, 16, 16, 0);  // 信号图标
    OLED_ShowChar(110, 0, ':', 16, 0);            // 分隔符
    
//    // 高度显示（信号图标下方）
//    OLED_ShowChar(98, 2, 'H', 16, 0);
//    OLED_ShowChar(106, 2, ':', 16, 0);
    
     // 初始化系统状态
    signalStrength = 0;                    // 初始信号强度为0
    droneConnection = DISCONNECTED;        // 初始未连接状态
    UpdateSignalDisplay();                 // 更新信号强度显示
    UpdateConnectionDisplay();             // 更新连接状态显示
    
    // 初始化按键状态数组
    KeyState_t keyStates[16];
    for(int i = 0; i < 16; i++) {
        keyStates[i].currentState = 1;     // 初始状态为释放
        keyStates[i].lastState = 1;        // 上次状态为释放
        keyStates[i].pressStartTime = 0;   // 按下开始时间
        keyStates[i].longPressSent = 0;    // 长按事件发送标志
    }
    
    for(;;) {
        // 键盘扫描
        currentKey = Keypad_Scan();
        currentTime = osKernelGetTickCount();
		
		// 检查是否需要进入低功耗模式
        if (currentPowerMode == POWER_MODE_NORMAL) {
            if (CheckPowerSaveTimeout()) {
                EnterPowerSaveMode();
            }
        }
		
		// 每1秒喂一次看门狗
        if((currentTime - lastFeedDog) > 1000) {
            HAL_IWDG_Refresh(&hiwdg);
            lastFeedDog = currentTime;
        }
		
			// 处理按键状态（在正常模式下）
			if (currentPowerMode == POWER_MODE_NORMAL) {
        
				// 处理按键状态
				for(int i = 0; i < 16; i++) {
					keyStates[i].lastState = keyStates[i].currentState;
					keyStates[i].currentState = (currentKey == i) ? 0 : 1;
					
					// 按键按下
					if(keyStates[i].currentState == 0 && keyStates[i].lastState == 1) {
						keyStates[i].pressStartTime = currentTime;
						keyStates[i].longPressSent = 0;
						
						// 处理动作按键按下
						if((i == KEY_S2 || i == KEY_S6 || i == KEY_S5 || i == KEY_S7) && 
						   lockStatus == UNLOCKED) {
							// 只有在解锁状态下才处理动作按键
							pressedActionKey = i;  // 记录当前按下的动作键
							
							// 清除动作显示区域（只清除中间区域，保留顶部和右侧状态栏）
							ClearActionDisplayArea();
							
							// 显示当前按下的动作
							DisplayAction(i, currentMode);
							
						}
					}
					
					// 按键释放
					if(keyStates[i].currentState == 1 && keyStates[i].lastState == 0) {
						// 如果是动作按键释放
						if(i == KEY_S2 || i == KEY_S6 || i == KEY_S5 || i == KEY_S7) {
							// 检查是否还有其他动作键按下
							uint8_t anyActionKeyPressed = 0;
							for(int j = 0; j < 16; j++) {
								if((j == KEY_S2 || j == KEY_S6 || j == KEY_S5 || j == KEY_S7) && 
								   keyStates[j].currentState == 0) {
									anyActionKeyPressed = 1;
									pressedActionKey = j;  // 更新为其他按下的键
									break;
								}
							}
							
							if(!anyActionKeyPressed) {
								// 没有其他动作键按下，清除显示
								ClearActionDisplayArea();
								pressedActionKey = NO_KEY;
							} else {
								// 有其他动作键按下，更新显示
								ClearActionDisplayArea();
								DisplayAction(pressedActionKey, currentMode);
							}
						}
					}
					
					// 长按检测（2秒）
					if(keyStates[i].currentState == 0 && !keyStates[i].longPressSent) {
						if((currentTime - keyStates[i].pressStartTime) >= LONG_PRESS_TIME) {
							// 长按S1：模式切换
							if(i == KEY_S1) { 
								HandleModeSwitch();
							}
							// 长按S4：锁定/解锁切换
							else if(i == KEY_S4) { 
								HandleLockToggle();
							}
							keyStates[i].longPressSent = 1;// 标记长按事件已处理
						}
					}
				}
				
				// 定期更新信号和连接状态（频率降低，避免闪烁）
				if((currentTime - lastSignalUpdate) > 5000) { // 5秒更新一次
					signalStrength = 80 + (osKernelGetTickCount() % 20);
					UpdateSignalDisplay();
					lastSignalUpdate = currentTime;
				}
				
				if((currentTime - lastConnectionCheck) > 7000) { // 7秒检查一次
					// 模拟连接状态变化
					ConnectionStatus_t oldStatus = droneConnection;
					droneConnection = (osKernelGetTickCount() % 10 < 8) ? CONNECTED : DISCONNECTED;
					
					if(oldStatus != droneConnection) {
						UpdateConnectionDisplay();
					}
					lastConnectionCheck = currentTime;
				}	
				
				// OLED保持活跃处理（30秒轻微刷新一次，防止OLED烧屏）
				if((currentTime - lastOLEDKeepAlive) > 30000) {
					// 轻微调整对比度来保持OLED活跃
					OLED_IntensityControl(0x7F + (oledRefreshCounter % 2));
					oledRefreshCounter++;
					lastOLEDKeepAlive = currentTime;
				}
		
		}
		
	// 在睡眠模式下，只进行最基本的处理
        else if (currentPowerMode == POWER_MODE_SLEEP) {
            // 睡眠模式下只检查唤醒按键和喂狗
            // 其他功能暂停以节省功耗
        }
	
		// 任务延时10ms，保持100Hz的处理频率
        osDelay(10);
    }
}

/**
  * @brief 更新连接状态显示
  * @retval None
  */
void UpdateConnectionDisplay(void)
{
    if(droneConnection == DISCONNECTED) {
        // 未连接状态：显示遥控器图标和X标记
        OLED_ShowIcon(96, 4, RemoteIcon, 16, 32, 0);// 遥控器图标
        OLED_ShowChar(116, 6, 'X', 16, 0);// X标记
    } else {
        // 连接状态下，清除连接状态显示区域
        // 清除区域：x:96-127, y:4-7 (32像素高))
        for(uint8_t page = 4; page < 8; page++) {
            OLED_Set_Pos(96, page);
            for(uint8_t col = 0; col < 32; col++) {
                OLED_WR_DATA(0x00);// 写入0清空像素
            }
        }
    }
}

/**
  * @brief 更新信号强度显示
  * @retval None
  */
void UpdateSignalDisplay(void)
{
   // 清除信号强度数字显示区域 (x: 112-127, y: 0-1)
    for(uint8_t page = 0; page < 2; page++) {
        OLED_Set_Pos(112, page);
        for(uint8_t col = 0; col < 16; col++) {
            OLED_WR_DATA(0x00);
        }
    }
    
    // 显示信号强度数值
    OLED_ShowNum(112, 0, signalStrength, 2, 16, 0);
}

/**
  * @brief 键盘任务（预留功能）
  * @param argument 任务参数
  * @retval None
  */
void Keypad_Task(void *argument)
{
    // 键盘初始化（已在System_Task中初始化，此处为冗余）
    Keypad_Init();
    
    for(;;)
    {  
		// 键盘扫描功能已移至System_Task，此处任务保持活跃
        osDelay(50);  
    }
}

/**
  * @brief 更新活动时间（重置无操作计时器）
  * @retval None
  */
void UpdateActivityTime(void)
{
    lastActivityTime = osKernelGetTickCount();
}

/**
  * @brief 检查是否达到低功耗超时时间
  * @return bool true-需要进入低功耗，false-保持正常模式
  */
bool CheckPowerSaveTimeout(void)
{
    uint32_t currentTime = osKernelGetTickCount();
    return ((currentTime - lastActivityTime) >= POWER_SAVE_TIMEOUT);
}

/**
  * @brief 进入省电模式（OLED息屏）
  * @retval None
  */
void EnterPowerSaveMode(void)
{
    if (currentPowerMode == POWER_MODE_NORMAL) {
        // 保存当前显示状态
        OLED_SaveDisplayState();
        
        // 关闭OLED显示
        OLED_SleepMode_Enter();
        
        // 更新电源模式状态
        currentPowerMode = POWER_MODE_SLEEP;
        oledDisplayState = false;
        
    }
}

/**
  * @brief 退出省电模式（OLED亮屏）
  * @retval None
  */
void ExitPowerSaveMode(void)
{
    if (currentPowerMode == POWER_MODE_SLEEP) {
        // 恢复OLED显示
        OLED_SleepMode_Exit();
        
        // 恢复显示内容
        OLED_RestoreDisplayState();
        
        // 更新电源模式状态
        currentPowerMode = POWER_MODE_NORMAL;
        oledDisplayState = true;
        
        // 重置活动时间
        UpdateActivityTime();
        
    }
}

/**
  * @brief 唤醒中断处理函数
  * @retval None
  */
void HandleWakeupInterrupt(void)
{
    static uint32_t lastWakeupTime = 0;
    uint32_t currentTime = osKernelGetTickCount();
    
    // 消抖处理
    if ((currentTime - lastWakeupTime) > POWER_WAKEUP_DEBOUNCE) {
        // 退出省电模式
        ExitPowerSaveMode();
        
        lastWakeupTime = currentTime;
    }
}

/* USER CODE END Application */

