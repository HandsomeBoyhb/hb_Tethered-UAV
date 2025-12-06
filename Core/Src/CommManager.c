#include "CommManager.h"
#include "drv_SDI.h"
#include <string.h>
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include "usart.h"

// 外部函数声明
extern uint8_t get_current_flight_mode(void);
extern uint8_t get_battery_level(void);

// 定义消息包结构体
typedef struct {
    uint16_t msg_id;
    uint8_t payload[256];
    uint16_t payload_len;
    uint32_t timestamp;
} MessagePacket;

// 通信管理器结构体
typedef struct {
    QueueHandle_t tx_queue;     // 发送队列
    QueueHandle_t rx_queue;     // 接收队列
    SemaphoreHandle_t tx_mutex; // 发送互斥锁
    TaskHandle_t rx_task_handle; // 接收任务句柄
    TaskHandle_t tx_task_handle; // 发送任务句柄
    uint16_t sequence;          // 消息序列号
    bool is_initialized;        // 初始化标志
    
    // 通信统计信息
    uint32_t tx_count;          // 发送消息计数
    uint32_t rx_count;          // 接收消息计数
    uint32_t error_count;       // 错误计数
    uint32_t last_heartbeat_time; // 上次心跳时间
    uint32_t last_rx_time;      // 上次接收时间
    uint32_t last_tx_time;      // 上次发送时间
    uint8_t connection_status;  // 连接状态 (0:未连接, 1:连接)
} CommManager;

// 全局通信管理器实例
static CommManager comm_mgr;

/**
 * @brief 消息接收处理函数
 * 
 * @param msg_id 消息ID
 * @param payload 消息数据
 * @param payload_len 数据长度
 */
/**
 * @brief 发送消息
 * 
 * @param msg_id 消息ID
 * @param payload 消息数据
 * @param payload_len 数据长度
 * @return true 发送成功
 * @return false 发送失败
 */
bool comm_send_message(uint16_t msg_id, const void* payload, uint16_t payload_len) {
    // 检查初始化状态
    if (!comm_mgr.is_initialized) {
        return false;
    }
    
    // 检查参数
    if (payload_len > 0 && payload == NULL) {
        return false;
    }
    
    // 创建消息包
    MessagePacket packet;
    packet.msg_id = msg_id;
    packet.payload_len = payload_len;
    packet.timestamp = xTaskGetTickCount();
    
    // 复制消息数据
    if (payload && payload_len > 0) {
        memcpy(packet.payload, payload, payload_len);
    }
    
    // 获取互斥锁
    if (xSemaphoreTake(comm_mgr.tx_mutex, pdMS_TO_TICKS(100)) != pdTRUE) {
        // 获取互斥锁失败
        comm_mgr.error_count++;
        return false;
    }
    
    // 将消息加入发送队列
    bool result = false;
    if (xQueueSend(comm_mgr.tx_queue, &packet, pdMS_TO_TICKS(100)) == pdTRUE) {
        result = true;
    } else {
        // 队列已满
        comm_mgr.error_count++;
    }
    
    // 释放互斥锁
    xSemaphoreGive(comm_mgr.tx_mutex);
    
    return result;
}

/**
 * @brief 获取通信状态
 * 
 * @param tx_count 发送计数
 * @param rx_count 接收计数
 * @param error_count 错误计数
 * @param connection_status 连接状态
 */
void comm_get_status(uint32_t* tx_count, uint32_t* rx_count, uint32_t* error_count, uint8_t* connection_status) {
    if (tx_count) *tx_count = comm_mgr.tx_count;
    if (rx_count) *rx_count = comm_mgr.rx_count;
    if (error_count) *error_count = comm_mgr.error_count;
    if (connection_status) *connection_status = comm_mgr.connection_status;
}
