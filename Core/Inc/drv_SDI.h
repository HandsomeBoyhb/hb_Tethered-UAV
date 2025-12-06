#ifndef DRV_SDI_H
#define DRV_SDI_H

#include <stdint.h>
#include <stdbool.h>

#define PROTOCOL_VERSION    0x0100  // 协议版本号
#define FRAME_HEADER1       0xFE    // 帧头字节1 (ACFly兼容)
#define FRAME_HEADER2       0x01    // 帧头字节2 (ACFly兼容)

// ACFly协议消息ID枚举
typedef enum {
    // 系统信息
    MSG_HEARTBEAT           = 0,   // 心跳包
    MSG_SYS_STATUS          = 1,   // 系统状态
    MSG_SYSTEM_TIME         = 2,   // 系统时间
    MSG_SYSTEM_RESET        = 11,  // 系统复位
    MSG_SYSTEM_STATUS       = 11,  // 系统状态
    
    // 遥控器控制
    MSG_MANUAL_CONTROL      = 69,  // 手动控制
    MSG_RC_CHANNELS         = 65,  // 通道数据
    MSG_RC_CHANNELS_SCALED  = 66,  // 缩放信号通道数据
    MSG_COMMAND_LONG        = 76,  // 长命令
    MSG_COMMAND_INT         = 75,  // 中断命令
    
    // 飞行模式控制
    MSG_FLIGHT_MODE         = 43,  // 飞行模式
    MSG_VIRTUAL_RC          = 65,  // 虚拟遥控 - 与RC_CHANNELS共享ID
    MSG_SET_MODE            = 11,  // 设置模式
    
    // 姿态位置控制
    MSG_ATTITUDE            = 30,  // 姿态数据
    MSG_POSITION_CTRL       = 84,  // 位置控制
    MSG_VELOCITY_CTRL       = 84,  // 速度控制 - 与位置控制共享ID
    MSG_ATTITUDE_CTRL       = 82,  // 姿态控制
    MSG_SIMPLE_CMD          = 76,  // 简单命令 - 与COMMAND_LONG共享ID
    
    // 状态查询
    MSG_QUERY_STATUS        = 1,   // 使用系统状态ID查询
    MSG_QUERY_SENSORS       = 29,  // 使用传感器数据ID查询
    MSG_QUERY_BATTERY       = 1,   // 使用系统状态ID查询(包含电池信息)
    MSG_QUERY_GPS           = 33,  // 使用全局定位ID查询
    
    // 参数管理
    MSG_PARAM_READ          = 21,  // 参数读取列表
    MSG_PARAM_WRITE         = 23,  // 参数写入
    MSG_PARAM_SAVE          = 23,  // 参数保存设置
    
    // 任务控制
    MSG_MISSION_UPLOAD      = 44,  // 任务设置/上传
    MSG_MISSION_START       = 41,  // 设置当前任务
    MSG_MISSION_PAUSE       = 76,  // 使用长命令暂停
    MSG_MISSION_RESUME      = 76,  // 使用长命令恢复
    
    // 扩展功能
    MSG_SWARM_BASE          = 0xF0  // 自定义功能起始ID
} MessageID;

// 飞行模式枚举
typedef enum {
    FLIGHT_MODE_MANUAL      = 0,   // 手动模式
    FLIGHT_MODE_STABILIZE   = 1,   // 自稳模式
    FLIGHT_MODE_ALT_HOLD    = 2,   // 定高模式
    FLIGHT_MODE_POS_HOLD    = 3,   // 定点模式
    FLIGHT_MODE_LOITER      = 4,   // 留待模式
    FLIGHT_MODE_RTL         = 5,   // 返航模式
    FLIGHT_MODE_LAND        = 6,   // 降落模式
    FLIGHT_MODE_OFFBOARD    = 7,   // 离线模式
    FLIGHT_MODE_FOLLOW      = 8,   // 跟随模式
    FLIGHT_MODE_MISSION     = 9    // 任务模式
} FlightMode;

// 错误码定义
typedef enum {
    ERROR_NONE              = 0x00, // 无错误
    ERROR_CRC_FAILED        = 0x01, // CRC校验失败
    ERROR_MSG_TOO_LONG      = 0x02, // 消息过长
    ERROR_MSG_INVALID       = 0x03, // 无效消息
    ERROR_CMD_NOT_SUPPORT   = 0x04, // 命令不支持
    ERROR_SYSTEM_BUSY       = 0x05, // 系统繁忙
    ERROR_PARAM_INVALID     = 0x06  // 参数无效
} ErrorCode;

// 系统状态枚举
typedef enum {
    SYSTEM_INIT             = 0,   // 系统初始化
    SYSTEM_READY            = 1,   // 系统就绪
    SYSTEM_ARMED            = 2,   // 系统已解锁
    SYSTEM_FLYING           = 3,   // 飞行中
    SYSTEM_ERROR            = 4,   // 系统错误
    SYSTEM_CRITICAL         = 5    // 系统严重错误
} SystemState;

// 简单命令枚举
typedef enum {
    CMD_MODE_SWITCH_COMPLETE = 0x01,     // 模式切换完成
    CMD_UNLOCK_COMPLETE      = 0x02,     // 解锁完成
    CMD_UP                  = 0x03,      // 上升
    CMD_DOWN                = 0x04,      // 下降
    CMD_TURN_LEFT           = 0x05,      // 左转
    CMD_TURN_RIGHT          = 0x06,      // 右转
    CMD_FORWARD             = 0x07,      // 前进
    CMD_BACKWARD            = 0x08,      // 后退
    CMD_LEFT_FORWARD        = 0x09,      // 左前进
    CMD_RIGHT_FORWARD       = 0x0A,      // 右前进
} SimpleCommand;

// 使用紧凑结构体打包（1字节对齐）
#pragma pack(push, 1)

// 通信消息头 - 兼容ACFly协议
typedef struct {
    uint8_t header1;         // 帧头1: 0xFE
    uint8_t payload_len;     // 消息负载长度（不包含校验和）
    uint8_t seq;             // 包序列号
    uint8_t sysid;           // 系统ID
    uint8_t compid;          // 组件ID
    uint8_t msgid;           // 消息ID
    // 接下来是消息数据(payload)，最后是CRC校验
} MsgHeader;

// 心跳消息结构体
typedef struct {
    uint32_t timestamp;      // 时间戳
    uint8_t system_state;    // 系统状态
    uint8_t flight_mode;     // 飞行模式
    uint8_t battery_level;   // 电池电量
    uint8_t error_code;      // 错误码
} HeartbeatMsg;

// 飞行控制消息结构体
typedef struct {
    float roll;         // 横滚 [-1.0, 1.0]
    float pitch;        // 俯仰 [-1.0, 1.0]
    float yaw;          // 偏航 [-1.0, 1.0]
    float throttle;     // 油门 [0.0, 1.0]
    uint8_t mode;       // 模式
    uint8_t reserved[3]; // 保留字段
} FlightControlMsg;

// 位置控制消息结构体
typedef struct {
    double lat;         // 纬度
    double lon;         // 经度
    float alt;          // 高度
    float yaw;          // 偏航角
    float tolerance;    // 容差
} PositionControlMsg;

// 传感器数据消息结构体
typedef struct {
    float accel[3];     // 加速度计 [m/s²]
    float gyro[3];      // 陀螺仪 [rad/s]
    float mag[3];       // 磁力计 [uT]
    float baro_alt;     // 气压高度 [m]
    float temp;         // 温度 [℃]
} SensorDataMsg;

// GPS数据消息结构体
typedef struct {
    double lat;         // 纬度
    double lon;         // 经度
    float alt;          // 高度
    float hdop;         // 水平精度因子
    uint8_t fix_type;   // 定位类型
    uint8_t satellites; // 卫星数量
    uint16_t reserved;  // 保留字段
} GpsDataMsg;

// 电池状态消息结构体
typedef struct {
    float voltage;       // 电压
    float current;       // 电流
    uint8_t capacity;    // 容量百分比
    int8_t temperature;  // 温度
    uint16_t reserved;   // 保留字段
} BatteryStatusMsg;

// 参数读写消息结构体
typedef struct {
    char param_id[16];   // 参数ID
    float value;         // 参数值
    uint8_t operation;   // 操作类型: 0=读, 1=写
    uint8_t result;      // 结果: 0=成功, 其他=错误码
} ParamMsg;

// 恢复默认结构体对齐
#pragma pack(pop)

// 协议编码解码函数声明
bool protocol_encode(uint16_t msg_id, const void* payload, uint16_t payload_len, 
                    uint8_t* buffer, uint16_t* buffer_len);
bool protocol_decode(const uint8_t* data, uint16_t data_len);
uint16_t protocol_crc16(const uint8_t* data, uint16_t length);

// 通信功能函数声明
bool protocol_send_acfly_message(uint16_t id,
                               float pos_x, float pos_y, float pos_z, float pos_yaw,
                               float vel_x, float vel_y, float vel_z, float vel_yaw,
                               float acc_x, float acc_y, float acc_z, float acc_yaw);
bool protocol_send_rc_channels(float roll, float pitch, float throttle, float yaw, uint8_t mode_switch);
bool protocol_send_heartbeat(void);
bool protocol_send_set_mode(uint8_t mode);
bool protocol_init(void);
bool protocol_receive_data(uint8_t* buffer, uint16_t length, unsigned int timeout);
bool protocol_receive_acfly_frame(uint16_t* msg_id, void* payload, uint16_t* payload_len, unsigned int timeout);
bool protocol_enable_rx_interrupt(void);
uint16_t protocol_read_rx_buffer(uint8_t* buffer, uint16_t max_length);

#endif
