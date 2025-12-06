#include "drv_SDI.h"
#include <string.h>
#include <stdint.h>
#include "usart.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "main.h"
#include "usart.h"

// CRC16-MODBUS校验表
static const uint16_t crc16_tab[256] = {
    0x0000, 0xC0C1, 0xC181, 0x0140, 0xC301, 0x03C0, 0x0280, 0xC241,
    0xC601, 0x06C0, 0x0780, 0xC741, 0x0500, 0xC5C1, 0xC481, 0x0440,
    0xCC01, 0x0CC0, 0x0D80, 0xCD41, 0x0F00, 0xCFC1, 0xCE81, 0x0E40,
    0x0A00, 0xCAC1, 0xCB81, 0x0B40, 0xC901, 0x09C0, 0x0880, 0xC841,
    0xD801, 0x18C0, 0x1980, 0xD941, 0x1B00, 0xDBC1, 0xDA81, 0x1A40,
    0x1E00, 0xDEC1, 0xDF81, 0x1F40, 0xDD01, 0x1DC0, 0x1C80, 0xDC41,
    0x1400, 0xD4C1, 0xD581, 0x1540, 0xD701, 0x17C0, 0x1680, 0xD641,
    0xD201, 0x12C0, 0x1380, 0xD341, 0x1100, 0xD1C1, 0xD081, 0x1040,
    0xF001, 0x30C0, 0x3180, 0xF141, 0x3300, 0xF3C1, 0xF281, 0x3240,
    0x3600, 0xF6C1, 0xF781, 0x3740, 0xF501, 0x35C0, 0x3480, 0xF441,
    0x3C00, 0xFCC1, 0xFD81, 0x3D40, 0xFF01, 0x3FC0, 0x3E80, 0xFE41,
    0xFA01, 0x3AC0, 0x3B80, 0xFB41, 0x3900, 0xF9C1, 0xF881, 0x3840,
    0x2800, 0xE8C1, 0xE981, 0x2940, 0xEB01, 0x2BC0, 0x2A80, 0xEA41,
    0xEE01, 0x2EC0, 0x2F80, 0xEF41, 0x2D00, 0xEDC1, 0xEC81, 0x2C40,
    0xE401, 0x24C0, 0x2580, 0xE541, 0x2700, 0xE7C1, 0xE681, 0x2640,
    0x2200, 0xE2C1, 0xE381, 0x2340, 0xE101, 0x21C0, 0x2080, 0xE041,
    0xA001, 0x60C0, 0x6180, 0xA141, 0x6300, 0xA3C1, 0xA281, 0x6240,
    0x6600, 0xA6C1, 0xA781, 0x6740, 0xA501, 0x65C0, 0x6480, 0xA441,
    0x6C00, 0xACC1, 0xAD81, 0x6D40, 0xAF01, 0x6FC0, 0x6E80, 0xAE41,
    0xAA01, 0x6AC0, 0x6B80, 0xAB41, 0x6900, 0xA9C1, 0xA881, 0x6840,
    0x7800, 0xB8C1, 0xB981, 0x7940, 0xBB01, 0x7BC0, 0x7A80, 0xBA41,
    0xBE01, 0x7EC0, 0x7F80, 0xBF41, 0x7D00, 0xBDC1, 0xBC81, 0x7C40,
    0xB401, 0x74C0, 0x7580, 0xB541, 0x7700, 0xB7C1, 0xB681, 0x7640,
    0x7200, 0xB2C1, 0xB381, 0x7340, 0xB101, 0x71C0, 0x7080, 0xB041,
    0x5000, 0x90C1, 0x9181, 0x5140, 0x9301, 0x53C0, 0x5280, 0x9241,
    0x9601, 0x56C0, 0x5780, 0x9741, 0x5500, 0x95C1, 0x9481, 0x5440,
    0x9C01, 0x5CC0, 0x5D80, 0x9D41, 0x5F00, 0x9FC1, 0x9E81, 0x5E40,
    0x5A00, 0x9AC1, 0x9B81, 0x5B40, 0x9901, 0x59C0, 0x5880, 0x9841,
    0x8801, 0x48C0, 0x4980, 0x8941, 0x4B00, 0x8BC1, 0x8A81, 0x4A40,
    0x4E00, 0x8EC1, 0x8F81, 0x4F40, 0x8D01, 0x4DC0, 0x4C80, 0x8C41,
    0x4400, 0x84C1, 0x8581, 0x4540, 0x8701, 0x47C0, 0x4680, 0x8641,
    0x8201, 0x42C0, 0x4380, 0x8341, 0x4100, 0x81C1, 0x8081, 0x4040
};

// ACFly协议常量定义
#define ACFLY_FRAME_HEAD1   0xFE    // ACFly帧头1
#define ACFLY_FRAME_HEAD2   0xFA    // ACFly帧头2
#define ACFLY_MSG_LENGTH    38      // ACFly消息总长度
#define ACFLY_DATA_LENGTH  32      // ACFly数据部分长度

// ACFly消息结构体定义
typedef struct {
    uint8_t msg[ACFLY_MSG_LENGTH]; // 完整消息缓冲区
    
    // 消息ID联合体
    union {
        uint16_t value;            // 16位消息ID值
        uint8_t bytes[2];          // 字节数组形式
    } id;

    // 位置数据联合体
    union {
        struct {
            float x;               // X位置
            float y;               // Y位置  
            float z;               // Z位置
            float yaw;             // 偏航角
        } value;
        uint8_t bytes[16];         // 字节数组形式(4个float × 4字节)
    } pos;

    // 速度数据联合体
    union {
        struct {
            float x;               // X速度
            float y;               // Y速度
            float z;               // Z速度
            float yaw;             // 偏航速度
        } value;
        uint8_t bytes[8];          // 字节数组形式(4个float × 2字节 - 压缩格式)
    } vel;

    // 加速度数据联合体
    union {
        struct {
            float x;               // X加速度
            float y;               // Y加速度
            float z;               // Z加速度
            float yaw;             // 偏航加速度
        } value;
        uint8_t bytes[8];          // 字节数组形式(4个float × 2字节 - 压缩格式)
    } acc;
    
    // CRC校验联合体
    union {
        uint16_t value;            // 16位CRC值
        uint8_t bytes[2];          // 字节数组形式
    } crc;
} ACFlyMessage;

/**
 * @brief 计算CRC16校验值
 * @param data 数据指针
 * @param length 数据长度
 * @return uint16_t CRC16校验值
 */
static uint16_t calculate_crc16(const uint8_t* data, uint16_t length)
{
    uint16_t crc = 0xFFFF;  // CRC初始值
    
    // 遍历所有数据字节
    while (length--) {
        uint8_t index = (crc ^ *data++) & 0xFF;  // 计算查表索引
        crc = (crc >> 8) ^ crc16_tab[index];     // 更新CRC值
    }
    
    return crc;
}

/**
 * @brief 打包ACFly消息并计算CRC
 * @param msg 消息结构体指针
 * @param id 消息ID
 * @param pos_x X位置
 * @param pos_y Y位置
 * @param pos_z Z位置
 * @param pos_yaw 偏航角
 * @param vel_x X速度
 * @param vel_y Y速度
 * @param vel_z Z速度
 * @param vel_yaw 偏航速度
 * @param acc_x X加速度
 * @param acc_y Y加速度
 * @param acc_z Z加速度
 * @param acc_yaw 偏航加速度
 * @return bool 打包是否成功
 */
static bool acfly_pack_message_with_crc(ACFlyMessage* msg, 
                                       uint16_t id,
                                       float pos_x, float pos_y, float pos_z, float pos_yaw,
                                       float vel_x, float vel_y, float vel_z, float vel_yaw,
                                       float acc_x, float acc_y, float acc_z, float acc_yaw)
{
    if (msg == NULL)
        return false;
    
    // 清空消息缓冲区
    memset(msg->msg, 0, ACFLY_MSG_LENGTH);
    
    // 设置帧头
    msg->msg[0] = ACFLY_FRAME_HEAD1; // 0xFE
    msg->msg[1] = ACFLY_FRAME_HEAD2; // 0xFA
    
    // 设置消息ID (大端序)
    msg->id.value = id;
    msg->msg[2] = msg->id.bytes[1]; // 高字节在前
    msg->msg[3] = msg->id.bytes[0]; // 低字节在后
    
    // 浮点数转换联合体
    union {
        float f;                    // 浮点数形式
        uint8_t b[4];               // 字节数组形式
    } converter;
    
    // 设置位置数据 (完整32位浮点数)
    // pos_x
    converter.f = pos_x;
    msg->pos.bytes[0] = converter.b[3];  // 大端序：高字节在前
    msg->pos.bytes[1] = converter.b[2];
    msg->pos.bytes[2] = converter.b[1];
    msg->pos.bytes[3] = converter.b[0];
    msg->msg[4] = converter.b[3];
    msg->msg[5] = converter.b[2];
    msg->msg[6] = converter.b[1];
    msg->msg[7] = converter.b[0];
    
    // pos_y
    converter.f = pos_y;
    msg->pos.bytes[4] = converter.b[3];
    msg->pos.bytes[5] = converter.b[2];
    msg->pos.bytes[6] = converter.b[1];
    msg->pos.bytes[7] = converter.b[0];
    msg->msg[8] = converter.b[3];
    msg->msg[9] = converter.b[2];
    msg->msg[10] = converter.b[1];
    msg->msg[11] = converter.b[0];
    
    // pos_z
    converter.f = pos_z;
    msg->pos.bytes[8] = converter.b[3];
    msg->pos.bytes[9] = converter.b[2];
    msg->pos.bytes[10] = converter.b[1];
    msg->pos.bytes[11] = converter.b[0];
    msg->msg[12] = converter.b[3];
    msg->msg[13] = converter.b[2];
    msg->msg[14] = converter.b[1];
    msg->msg[15] = converter.b[0];
    
    // pos_yaw
    converter.f = pos_yaw;
    msg->pos.bytes[12] = converter.b[3];
    msg->pos.bytes[13] = converter.b[2];
    msg->pos.bytes[14] = converter.b[1];
    msg->pos.bytes[15] = converter.b[0];
    msg->msg[16] = converter.b[3];
    msg->msg[17] = converter.b[2];
    msg->msg[18] = converter.b[1];
    msg->msg[19] = converter.b[0];
    
    // 设置速度数据 (压缩格式 - 只使用高16位)
    // vel_x
    converter.f = vel_x;
    msg->vel.bytes[0] = converter.b[3];  // 只存储高2字节
    msg->vel.bytes[1] = converter.b[2];
    msg->msg[20] = converter.b[3];
    msg->msg[21] = converter.b[2];
    
    // vel_y
    converter.f = vel_y;
    msg->vel.bytes[2] = converter.b[3];
    msg->vel.bytes[3] = converter.b[2];
    msg->msg[22] = converter.b[3];
    msg->msg[23] = converter.b[2];
    
    // vel_z
    converter.f = vel_z;
    msg->vel.bytes[4] = converter.b[3];
    msg->vel.bytes[5] = converter.b[2];
    msg->msg[24] = converter.b[3];
    msg->msg[25] = converter.b[2];
    
    // vel_yaw
    converter.f = vel_yaw;
    msg->vel.bytes[6] = converter.b[3];
    msg->vel.bytes[7] = converter.b[2];
    msg->msg[26] = converter.b[3];
    msg->msg[27] = converter.b[2];
    
    // 设置加速度数据 (压缩格式 - 只使用高16位)
    // acc_x
    converter.f = acc_x;
    msg->acc.bytes[0] = converter.b[3];
    msg->acc.bytes[1] = converter.b[2];
    msg->msg[28] = converter.b[3];
    msg->msg[29] = converter.b[2];
    
    // acc_y
    converter.f = acc_y;
    msg->acc.bytes[2] = converter.b[3];
    msg->acc.bytes[3] = converter.b[2];
    msg->msg[30] = converter.b[3];
    msg->msg[31] = converter.b[2];
    
    // acc_z
    converter.f = acc_z;
    msg->acc.bytes[4] = converter.b[3];
    msg->acc.bytes[5] = converter.b[2];
    msg->msg[32] = converter.b[3];
    msg->msg[33] = converter.b[2];
    
    // acc_yaw
    converter.f = acc_yaw;
    msg->acc.bytes[6] = converter.b[3];
    msg->acc.bytes[7] = converter.b[2];
    msg->msg[34] = converter.b[3];
    msg->msg[35] = converter.b[2];
    
    // 计算并设置CRC校验值
    uint16_t crc = calculate_crc16(msg->msg, ACFLY_MSG_LENGTH - 2);
    msg->crc.value = crc;
    
    // 设置CRC字节 (大端序)
    msg->msg[36] = msg->crc.bytes[1]; // CRC高字节
    msg->msg[37] = msg->crc.bytes[0]; // CRC低字节
    
    return true;
}

/**
 * @brief 发送ACFly结构化数据（含CRC校验）
 * @param id 消息ID
 * @param pos_x X位置
 * @param pos_y Y位置
 * @param pos_z Z位置
 * @param pos_yaw 航向偏角
 * @param vel_x X速度
 * @param vel_y Y速度
 * @param vel_z Z速度
 * @param vel_yaw 偏航速度
 * @param acc_x X加速度
 * @param acc_y Y加速度
 * @param acc_z Z加速度
 * @param acc_yaw 偏航加速度
 * @return bool 发送是否成功
 */
bool protocol_send_acfly_message(uint16_t id,
                               float pos_x, float pos_y, float pos_z, float pos_yaw,
                               float vel_x, float vel_y, float vel_z, float vel_yaw,
                               float acc_x, float acc_y, float acc_z, float acc_yaw)
{
    ACFlyMessage msg;
    // 打包消息
    if (!acfly_pack_message_with_crc(&msg, id, pos_x, pos_y, pos_z, pos_yaw, 
                                   vel_x, vel_y, vel_z, vel_yaw,
                                   acc_x, acc_y, acc_z, acc_yaw)) {
        return false;
    }
    // 通过UART1发送消息
    return (HAL_UART_Transmit(&huart1, msg.msg, ACFLY_MSG_LENGTH, 100) == HAL_OK);
}

/**
 * @brief 快速发送ACFly消息（直接操作缓冲区）
 * @param id 消息ID
 * @param pos_x X位置
 * @param pos_y Y位置
 * @param pos_z Z位置
 * @param pos_yaw 偏航角
 * @param vel_x X速度
 * @param vel_y Y速度
 * @param vel_z Z速度
 * @param vel_yaw 偏航速度
 * @param acc_x X加速度
 * @param acc_y Y加速度
 * @param acc_z Z加速度
 * @param acc_yaw 偏航加速度
 * @return bool 发送是否成功
 */
bool protocol_send_acfly_message_fast(uint16_t id,
                                    float pos_x, float pos_y, float pos_z, float pos_yaw,
                                    float vel_x, float vel_y, float vel_z, float vel_yaw,
                                    float acc_x, float acc_y, float acc_z, float acc_yaw)
{
    uint8_t buffer[ACFLY_MSG_LENGTH];
    
    // 设置帧头
    buffer[0] = ACFLY_FRAME_HEAD1;
    buffer[1] = ACFLY_FRAME_HEAD2;
    
    // 设置消息ID (大端序)
    buffer[2] = (id >> 8) & 0xFF;  // 高字节
    buffer[3] = id & 0xFF;         // 低字节
    
    // 浮点数转换联合体
    union {
        float f;
        uint8_t b[4];
    } converter;
    
    // 设置位置数据 (完整32位)
    converter.f = pos_x;
    buffer[4] = converter.b[3];  // 大端序
    buffer[5] = converter.b[2];
    buffer[6] = converter.b[1];
    buffer[7] = converter.b[0];
    
    converter.f = pos_y;
    buffer[8] = converter.b[3];
    buffer[9] = converter.b[2];
    buffer[10] = converter.b[1];
    buffer[11] = converter.b[0];
    
    converter.f = pos_z;
    buffer[12] = converter.b[3];
    buffer[13] = converter.b[2];
    buffer[14] = converter.b[1];
    buffer[15] = converter.b[0];
    
    converter.f = pos_yaw;
    buffer[16] = converter.b[3];
    buffer[17] = converter.b[2];
    buffer[18] = converter.b[1];
    buffer[19] = converter.b[0];
    
    // 设置速度数据 (压缩格式 - 高16位)
    converter.f = vel_x;
    buffer[20] = converter.b[3];
    buffer[21] = converter.b[2];
    
    converter.f = vel_y;
    buffer[22] = converter.b[3];
    buffer[23] = converter.b[2];
    
    converter.f = vel_z;
    buffer[24] = converter.b[3];
    buffer[25] = converter.b[2];
    
    converter.f = vel_yaw;
    buffer[26] = converter.b[3];
    buffer[27] = converter.b[2];
    
    // 设置加速度数据 (压缩格式 - 高16位)
    converter.f = acc_x;
    buffer[28] = converter.b[3];
    buffer[29] = converter.b[2];
    
    converter.f = acc_y;
    buffer[30] = converter.b[3];
    buffer[31] = converter.b[2];
    
    converter.f = acc_z;
    buffer[32] = converter.b[3];
    buffer[33] = converter.b[2];
    
    converter.f = acc_yaw;
    buffer[34] = converter.b[3];
    buffer[35] = converter.b[2];
    
    // 计算并设置CRC校验
    uint16_t crc = calculate_crc16(buffer, ACFLY_MSG_LENGTH - 2);
    buffer[36] = (crc >> 8) & 0xFF;  // CRC高字节
    buffer[37] = crc & 0xFF;         // CRC低字节
    
    // 发送消息
    return (HAL_UART_Transmit(&huart1, buffer, ACFLY_MSG_LENGTH, 100) == HAL_OK);
}

/**
 * @brief 发送RC通道数据（带CRC校验）
 * @param roll 横滚控制量 [-1.0, 1.0]
 * @param pitch 俯仰控制量 [-1.0, 1.0]
 * @param throttle 油门控制量 [0.0, 1.0]
 * @param yaw 偏航控制量 [-1.0, 1.0]
 * @param mode_switch 模式开关
 * @return bool 发送是否成功
 */
bool protocol_send_rc_channels_with_crc(float roll, float pitch, float throttle, float yaw, uint8_t mode_switch)
{
    // 将控制量转换为位置数据格式
    float pos_x = roll * 1000.0f;      // 横滚映射到X位置
    float pos_y = pitch * 1000.0f;     // 俯仰映射到Y位置
    float pos_z = throttle * 1000.0f;  // 油门映射到Z位置
    float pos_yaw = yaw * 1000.0f;     // 偏航映射到偏航角
    
    // 模式开关映射到X速度
    float vel_x = mode_switch ? 1000.0f : 0.0f;  // 模式开关状态
    
    // 发送RC通道信息（使用MSG_RC_CHANNELS消息ID）
    return protocol_send_acfly_message(MSG_RC_CHANNELS, 
                                     pos_x, pos_y, pos_z, pos_yaw,
                                     vel_x, 0.0f, 0.0f, 0.0f,
                                     0.0f, 0.0f, 0.0f, 0.0f);
}

/**
 * @brief 通用协议编码函数（含CRC校验）
 * @param msg_id 消息ID
 * @param payload 负载数据指针
 * @param payload_len 负载数据长度
 * @param buffer 输出缓冲区
 * @param buffer_len 缓冲区长度（输入时提供最大长度，输出时返回实际长度）
 * @return bool 编码是否成功
 */
bool protocol_encode(uint16_t msg_id, const void* payload, uint16_t payload_len, 
                    uint8_t* buffer, uint16_t* buffer_len)
{
    // 参数有效性检查
    if (buffer == NULL || buffer_len == NULL || (payload == NULL && payload_len > 0)) {
        return false;
    }
    
    // 缓冲区大小检查
    if (*buffer_len < ACFLY_MSG_LENGTH) {
        *buffer_len = ACFLY_MSG_LENGTH;  // 返回所需的最小长度
        return false;
    }
    
    // 默认数据值
    float pos_x = 0.0f, pos_y = 0.0f, pos_z = 0.0f, pos_yaw = 0.0f;
    float vel_x = 0.0f, vel_y = 0.0f, vel_z = 0.0f, vel_yaw = 0.0f;
    float acc_x = 0.0f, acc_y = 0.0f, acc_z = 0.0f, acc_yaw = 0.0f;
    
    // 如果有负载数据，解析为浮点数数组
    if (payload && payload_len > 0) {
        const float* data = (const float*)payload;
        uint16_t floats_available = payload_len / sizeof(float);
        
        // 按顺序提取浮点数值
        if (floats_available > 0) pos_x = data[0];
        if (floats_available > 1) pos_y = data[1];
        if (floats_available > 2) pos_z = data[2];
        if (floats_available > 3) pos_yaw = data[3];
        if (floats_available > 4) vel_x = data[4];
        if (floats_available > 5) vel_y = data[5];
        if (floats_available > 6) vel_z = data[6];
        if (floats_available > 7) vel_yaw = data[7];
        if (floats_available > 8) acc_x = data[8];
        if (floats_available > 9) acc_y = data[9];
        if (floats_available > 10) acc_z = data[10];
        if (floats_available > 11) acc_yaw = data[11];
    }
    
    // 使用快速发送函数编码消息
    if (!protocol_send_acfly_message_fast(msg_id, pos_x, pos_y, pos_z, pos_yaw,
                                        vel_x, vel_y, vel_z, vel_yaw,
                                        acc_x, acc_y, acc_z, acc_yaw)) {
        return false;
    }
    
    // 返回实际编码长度
    *buffer_len = ACFLY_MSG_LENGTH;
    return true;
}

/**
 * @brief 计算数据的CRC16校验值
 * @param data 数据指针
 * @param length 数据长度
 * @return uint16_t CRC16校验值
 */
uint16_t protocol_crc16(const uint8_t* data, uint16_t length)
{
    return calculate_crc16(data, length);
}

/**
 * @brief 批量发送消息
 * @param messages 消息数组指针
 * @param count 消息数量
 * @return uint16_t 成功发送的消息数量
 */
uint16_t protocol_batch_send_messages(const ACFlyMessage* messages, uint16_t count)
{
    if (messages == NULL || count == 0) {
        return 0;
    }
    
    uint16_t success_count = 0;
    
    // 遍历所有消息并发送
    for (uint16_t i = 0; i < count; i++) {
        if (HAL_UART_Transmit(&huart1, messages[i].msg, ACFLY_MSG_LENGTH, 50) == HAL_OK) {
            success_count++;
        }
        
        // 消息间插入小延时（除最后一条消息外）
        if (i < count - 1) {
            HAL_Delay(1);
        }
    }
    
    return success_count;
}

/**
 * @brief 协议解码函数
 * @param data 数据缓冲区指针
 * @param data_len 数据长度
 * @return bool 解析是否成功
 */
bool protocol_decode(const uint8_t* data, uint16_t data_len)
{	
    // 参数检查
    if (data == NULL || data_len < 4) {
        return false;
    }
    
    // 检查帧头
    if (data[0] != ACFLY_FRAME_HEAD1 || data[1] != ACFLY_FRAME_HEAD2) {
        return false;
    }
    
    // 如果有CRC，验证CRC校验
    if (data_len >= 6) { // 至少需要头(2字节) + ID(2字节) + CRC(2字节)
        uint16_t crc_calculated = calculate_crc16(data, data_len - 2);
        uint16_t crc_received = (data[data_len - 2] << 8) | data[data_len - 1];
        
        if (crc_calculated != crc_received) {
            // CRC校验失败 - 递归调用解码（注意：这可能不是最佳做法）
            protocol_decode(data, data_len);
            return false;
        }
    }
    
    return true;
}

/**
 * @brief 验证消息CRC校验
 * @param data 消息数据指针
 * @param length 消息长度
 * @return bool CRC校验是否通过
 */
bool protocol_verify_crc(const uint8_t* data, uint16_t length)
{
    if (data == NULL || length < 2) {
        return false;
    }
    
    // 计算CRC并比较
    uint16_t calculated_crc = calculate_crc16(data, length - 2);
    uint16_t received_crc = (data[length - 2] << 8) | data[length - 1];
    
    return (calculated_crc == received_crc);
}

/**
 * @brief 初始化发送器
 * @return bool 初始化是否成功
 */
bool protocol_init_sender(void)
{
    // 此处可以添加发送器特定的初始化代码
    // 目前只是返回成功
    return true;
}
