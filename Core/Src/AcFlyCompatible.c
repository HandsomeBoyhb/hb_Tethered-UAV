#include "CommManager.h"
#include "drv_SDI.h"
#include "AcFlyCompatible.h"
#include <string.h>
#include "FreeRTOS.h"
#include "task.h"

// 外部函数声明
extern uint8_t get_battery_level(void);
extern uint8_t get_current_flight_mode(void);

// 内部变量
static uint8_t acfly_system_id = 1;     // 本系统ID
static uint8_t acfly_component_id = 1;  // 本组件ID
static uint8_t acfly_target_system = 1; // 目标系统ID
static uint8_t acfly_target_component = 1; // 目标组件ID

/**
 * @brief 设置ACFly系统ID和组件ID
 */
void acfly_set_system_id(uint8_t system_id, uint8_t component_id) {
    acfly_system_id = system_id;
    acfly_component_id = component_id;
}

/**
 * @brief ACFly协议处理消息
 */
void acfly_process_message(uint16_t msg_id, const void* payload, uint16_t payload_len) {
    // 更新目标系统ID (用于回复消息)
    if (payload_len >= 6) {  // 假设消息至少有一个系统ID和组件ID
        const uint8_t* data = (const uint8_t*)payload;
        // 通常，系统ID和组件ID在消息的特定位置
        // 这里我们假设它们在第5和第6个字节位置
        acfly_target_system = data[4];
        acfly_target_component = data[5];
    }

    switch (msg_id) {
        case MSG_HEARTBEAT: 
        {
            // 处理心跳包
            if (payload_len >= 9) {  // ACFly心跳包有9个字节
                const uint8_t* data = (const uint8_t*)payload;
                uint8_t type = data[0];         // 系统类型
                uint8_t autopilot = data[1];    // 自动驾驶仪类型
                uint8_t base_mode = data[2];    // 基本模式
                uint32_t custom_mode;           // 自定义模式
                memcpy(&custom_mode, &data[3], sizeof(uint32_t));
                uint8_t system_status = data[7]; // 系统状态
                
                // 这里可以添加对心跳包数据的处理
                // 例如，更新连接状态、检查飞控类型等
                
                // 响应心跳包，维持连接
                acfly_send_heartbeat();
            }
            break;
        }
            
        case MSG_SET_MODE: 
        {
            // 处理模式设置
            if (payload_len >= 6) {
                const uint8_t* data = (const uint8_t*)payload;
                uint8_t target_system = data[0];
                uint32_t custom_mode = 0;
                memcpy(&custom_mode, &data[1], sizeof(uint32_t));
                uint8_t mode = custom_mode & 0xFF;  // 提取低8位作为飞行模式
                
                // 处理模式切换逻辑
                // 这里应该根据mode值进行飞行模式切换
                
                // 发送模式切换确认
                acfly_send_command_ack(MSG_SET_MODE, ACFLY_RESULT_ACCEPTED);
            }
            break;
        }
        
        case MSG_RC_CHANNELS:
        {
            // 处理RC通道数据
            if (payload_len >= 22) { // RC_CHANNELS消息格式
                const uint8_t* data = (const uint8_t*)payload;
                uint32_t time_boot_ms;
                memcpy(&time_boot_ms, &data[0], sizeof(uint32_t));
                uint8_t chan_count = data[4];
                uint16_t channels[8];
                
                // 读取8个通道数据
                for (int i = 0; i < 8 && i < chan_count; i++) {
                    memcpy(&channels[i], &data[5 + i*2], sizeof(uint16_t));
                }
                
                // 转换为我们的控制格式
                FlightControlMsg rc_data;
                // 通道值范围通常为1000-2000，转换为-1.0到1.0
                rc_data.roll = (channels[0] - 1500) / 500.0f;     // Roll (通道1)
                rc_data.pitch = (channels[1] - 1500) / 500.0f;    // Pitch (通道2)
                rc_data.throttle = (channels[2] - 1000) / 1000.0f; // Throttle (通道3)
                rc_data.yaw = (channels[3] - 1500) / 500.0f;      // Yaw (通道4)
                rc_data.mode = (channels[4] > 1500) ? 1 : 0;      // 模式切换 (通道5)
                
                // 处理控制数据
                // TODO: 添加处理遥控数据的函数调用
                // process_control_data(&rc_data);
            }
            break;
        }
        
        case MSG_MANUAL_CONTROL:
        {
            // 处理手动控制数据
            if (payload_len >= 11) { // MANUAL_CONTROL消息格式
                const uint8_t* data = (const uint8_t*)payload;
                int16_t x, y, z, r;
                uint16_t buttons;
                
                memcpy(&x, &data[0], sizeof(int16_t)); // 横向
                memcpy(&y, &data[2], sizeof(int16_t)); // 纵向
                memcpy(&z, &data[4], sizeof(int16_t)); // 油门
                memcpy(&r, &data[6], sizeof(int16_t)); // 偏航
                memcpy(&buttons, &data[8], sizeof(uint16_t)); // 按钮
                
                // 转换为我们的控制格式
                FlightControlMsg rc_data;
                rc_data.roll = x / 1000.0f;     // 范围 [-1.0, 1.0]
                rc_data.pitch = y / 1000.0f;    // 范围 [-1.0, 1.0]
                rc_data.throttle = (z + 1000) / 2000.0f; // 范围 [0.0, 1.0]
                rc_data.yaw = r / 1000.0f;      // 范围 [-1.0, 1.0]
                rc_data.mode = 0;               // 默认模式
                
                // 按钮处理
                if (buttons & 0x01) {
                    rc_data.mode = 1; // 示例：按钮1用于模式切换
                }
                
                // TODO: 添加处理控制数据的函数调用
                // process_control_data(&rc_data);
            }
            break;
        }
        
        case MSG_COMMAND_LONG:
        {
            // 处理长命令
            if (payload_len >= 30) { // COMMAND_LONG消息格式
                const uint8_t* data = (const uint8_t*)payload;
                uint16_t command;
                float param1, param2, param3, param4, param5, param6, param7;
                uint8_t target_system, target_component, confirmation;
                
                // 解析命令数据
                target_system = data[0];
                target_component = data[1];
                memcpy(&command, &data[2], sizeof(uint16_t));
                memcpy(&param1, &data[4], sizeof(float));
                memcpy(&param2, &data[8], sizeof(float));
                memcpy(&param3, &data[12], sizeof(float));
                memcpy(&param4, &data[16], sizeof(float));
                memcpy(&param5, &data[20], sizeof(float));
                memcpy(&param6, &data[24], sizeof(float));
                memcpy(&param7, &data[28], sizeof(float));
                confirmation = data[32];
                
                // 根据不同命令ID处理
                switch (command) {
                    case ACFLY_CMD_COMPONENT_ARM_DISARM:
                    {
                        bool arm = (param1 > 0.5f);
                        // TODO: 处理解锁/上锁命令
                        // process_arm_command(arm);
                        
                        // 发送确认
                        acfly_send_command_ack(command, ACFLY_RESULT_ACCEPTED);
                    }
                    break;
                    
                    case ACFLY_CMD_NAV_RETURN_TO_LAUNCH:
                    {
                        // TODO: 处理返航命令
                        // process_rtl_command();
                        
                        // 发送确认
                        acfly_send_command_ack(command, ACFLY_RESULT_ACCEPTED);
                    }
                    break;
                    
                    case ACFLY_CMD_NAV_LAND:
                    {
                        // TODO: 处理降落命令
                        // process_land_command();
                        
                        // 发送确认
                        acfly_send_command_ack(command, ACFLY_RESULT_ACCEPTED);
                    }
                    break;
                    
                    case ACFLY_CMD_NAV_TAKEOFF:
                    {
                        // TODO: 处理起飞命令
                        // process_takeoff_command(param7); // param7是起飞高度
                        
                        // 发送确认
                        acfly_send_command_ack(command, ACFLY_RESULT_ACCEPTED);
                    }
                    break;
                    
                    default:
                    {
                        // 未知命令，发送不支持响应
                        acfly_send_command_ack(command, ACFLY_RESULT_UNSUPPORTED);
                    }
                    break;
                }
            }
            break;
        }
        
        case MSG_QUERY_STATUS:
        {
            // 响应状态查询 - 发送系统状态
            uint16_t voltage = 11000; // 11.0V
            int16_t current = 0;      // 0A
            int8_t remaining = get_battery_level();
            
            acfly_send_system_status(voltage, current, remaining);
            break;
        }
            
        default:
        {
            // 未知消息，如果是命令类型，发送不支持响应
            if (msg_id >= 150 && msg_id <= 250) {
                // 假设150-250是命令ID范围
                acfly_send_command_ack(msg_id, ACFLY_RESULT_UNSUPPORTED);
            }
            break;
        }
    }
}

/**
 * @brief ACFly兼容心跳包发送函数
 */
void acfly_send_heartbeat(void) {
    struct {
        uint8_t type;          // 系统类型
        uint8_t autopilot;     // 自动驾驶类型
        uint8_t base_mode;     // 基本模式
        uint32_t custom_mode;  // 自定义模式
        uint8_t system_status; // 系统状态
    } heartbeat = {
        .type = ACFLY_TYPE_GCS,        // 遥控器/地面站类型
        .autopilot = ACFLY_AUTOPILOT_GENERIC, // 通用自动驾驶
        .base_mode = 0,                // 基本模式
        .custom_mode = 0,              // 自定义模式
        .system_status = ACFLY_STATE_ACTIVE // 活动状态
    };
    
    comm_send_message(MSG_HEARTBEAT, &heartbeat, sizeof(heartbeat));
}

/**
 * @brief 发送ACFly姿态数据
 */
void acfly_send_attitude(float roll, float pitch, float yaw) {
    struct {
        uint32_t time_boot_ms;  // 启动后的时间(ms)
        float roll;             // 横滚角(rad)
        float pitch;            // 俯仰角(rad)
        float yaw;              // 偏航角(rad)
        float rollspeed;        // 横滚角速度(rad/s)
        float pitchspeed;       // 俯仰角速度(rad/s)
        float yawspeed;         // 偏航角速度(rad/s)
    } attitude = {
        .time_boot_ms = xTaskGetTickCount(),
        .roll = roll,
        .pitch = pitch,
        .yaw = yaw,
        .rollspeed = 0.0f,
        .pitchspeed = 0.0f,
        .yawspeed = 0.0f
    };
    
    comm_send_message(MSG_ATTITUDE, &attitude, sizeof(attitude));
}

/**
 * @brief 发送ACFly系统状态
 */
void acfly_send_system_status(uint16_t battery_voltage, int16_t battery_current, int8_t battery_remaining) {
    struct {
        uint32_t onboard_control_sensors_present;
        uint32_t onboard_control_sensors_enabled;
        uint32_t onboard_control_sensors_health;
        uint16_t load;
        uint16_t voltage_battery;
        int16_t current_battery;
        int8_t battery_remaining;
        uint16_t drop_rate_comm;
        uint16_t errors_comm;
        uint16_t errors_count1;
        uint16_t errors_count2;
        uint16_t errors_count3;
        uint16_t errors_count4;
    } sys_status = {0};
    
    // 填充系统状态数据
    sys_status.onboard_control_sensors_present = 0x1FFF; // 所有传感器都存在
    sys_status.onboard_control_sensors_enabled = 0x1FFF; // 所有传感器都启用
    sys_status.onboard_control_sensors_health = 0x1FFF;  // 所有传感器都健康
    sys_status.load = 500;                               // 系统负载 (50.0%)
    sys_status.voltage_battery = battery_voltage;        // 电池电压 (mV)
    sys_status.current_battery = battery_current;        // 电池电流 (10*mA)
    sys_status.battery_remaining = battery_remaining;    // 剩余电量百分比
    
    comm_send_message(MSG_SYS_STATUS, &sys_status, sizeof(sys_status));
}

/**
 * @brief 发送ACFly命令确认
 */
void acfly_send_command_ack(uint16_t command_id, ACFlyCommandResult result) {
    struct {
        uint16_t command;
        uint8_t result;
        uint8_t progress;
        int32_t result_param2;
        uint8_t target_system;
        uint8_t target_component;
    } cmd_ack = {
        .command = command_id,
        .result = (uint8_t)result,
        .progress = (result == ACFLY_RESULT_ACCEPTED) ? 100 : 0,
        .result_param2 = 0,
        .target_system = acfly_target_system,
        .target_component = acfly_target_component
    };
    
    comm_send_message(ACFLY_MSG_ID_COMMAND_ACK, &cmd_ack, sizeof(cmd_ack));
}
