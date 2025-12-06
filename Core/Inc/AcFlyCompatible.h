#ifndef ACFLY_COMPATIBLE_H
#define ACFLY_COMPATIBLE_H

#include <stdint.h>
#include <stdbool.h>

/**
 * @brief ACFly协议常量定义
 */
// 系统类型
#define ACFLY_TYPE_GCS                  6  // 地面站/遥控器
#define ACFLY_TYPE_QUADROTOR            2  // 四旋翼
#define ACFLY_TYPE_HEXAROTOR            13 // 六旋翼

// 自动驾驶仪类型
#define ACFLY_AUTOPILOT_GENERIC         0  // 通用
#define ACFLY_AUTOPILOT_ACFLY           15 // ACFly飞控

// 系统状态
#define ACFLY_STATE_STANDBY             3  // 待命
#define ACFLY_STATE_ACTIVE              4  // 活动
#define ACFLY_STATE_CRITICAL            5  // 紧急状态
#define ACFLY_STATE_EMERGENCY           6  // 应急状态

// 基本模式标志
#define ACFLY_MODE_FLAG_SAFETY_ARMED    128 // 解锁标志

// ACFly命令ID
#define ACFLY_CMD_NAV_WAYPOINT          16  // 航点导航
#define ACFLY_CMD_NAV_RETURN_TO_LAUNCH  20  // 返航
#define ACFLY_CMD_NAV_LAND              21  // 降落
#define ACFLY_CMD_NAV_TAKEOFF           22  // 起飞
#define ACFLY_CMD_COMPONENT_ARM_DISARM  400 // 解锁/上锁

// ACFly特定消息ID
#define ACFLY_MSG_ID_COMMAND_ACK        77  // 命令确认
#define ACFLY_MSG_ID_COMMAND_LONG       76  // 长指令
#define ACFLY_MSG_ID_ATTITUDE_TARGET    83  // 姿态目标
#define ACFLY_MSG_ID_POSITION_TARGET    85  // 位置目标

/**
 * @brief ACFly协议结构体定义
 */

// ACFly命令结果
typedef enum {
    ACFLY_RESULT_ACCEPTED  = 0, // 命令接受
    ACFLY_RESULT_DENIED    = 1, // 命令拒绝
    ACFLY_RESULT_FAILED    = 4, // 命令失败
    ACFLY_RESULT_UNSUPPORTED = 3 // 不支持的命令
} ACFlyCommandResult;

// ACFly飞行模式
typedef enum {
    ACFLY_MODE_MANUAL     = 0, // 手动模式
    ACFLY_MODE_STABILIZE  = 1, // 自稳模式
    ACFLY_MODE_ALTITUDE   = 2, // 定高模式
    ACFLY_MODE_POSITION   = 3, // 定点模式
    ACFLY_MODE_MISSION    = 4, // 任务模式
    ACFLY_MODE_RTL        = 5, // 返航模式
    ACFLY_MODE_LAND       = 6  // 降落模式
} ACFlyFlightMode;

/**
 * @brief 处理ACFly协议消息
 * 
 * @param msg_id 消息ID
 * @param payload 消息数据
 * @param payload_len 数据长度
 */
void acfly_process_message(uint16_t msg_id, const void* payload, uint16_t payload_len);

/**
 * @brief 发送ACFly兼容心跳包
 */
void acfly_send_heartbeat(void);

/**
 * @brief 发送ACFly姿态数据
 * 
 * @param roll 横滚角 (rad)
 * @param pitch 俯仰角 (rad)
 * @param yaw 偏航角 (rad)
 */
void acfly_send_attitude(float roll, float pitch, float yaw);

/**
 * @brief 发送ACFly系统状态
 * 
 * @param battery_voltage 电池电压 (mV)
 * @param battery_current 电池电流 (10*mA)
 * @param battery_remaining 剩余电量百分比 (0-100)
 */
void acfly_send_system_status(uint16_t battery_voltage, int16_t battery_current, int8_t battery_remaining);

/**
 * @brief 发送ACFly命令确认
 * 
 * @param command_id 命令ID
 * @param result 命令结果
 */
void acfly_send_command_ack(uint16_t command_id, ACFlyCommandResult result);

/**
 * @brief 设置ACFly系统ID和组件ID
 * 
 * @param system_id 系统ID
 * @param component_id 组件ID
 */
void acfly_set_system_id(uint8_t system_id, uint8_t component_id);

#endif // ACFLY_COMPATIBLE_H
