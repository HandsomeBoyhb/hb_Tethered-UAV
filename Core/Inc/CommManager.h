#ifndef COMM_MANAGER_H
#define COMM_MANAGER_H

#include <stdint.h>
#include <stdbool.h>

// 函数声明
bool comm_manager_init(void);
bool comm_send_message(uint16_t msg_id, const void* payload, uint16_t payload_len);
void comm_get_status(uint32_t* tx_count, uint32_t* rx_count, uint32_t* error_count, uint8_t* connection_status);

#endif // COMM_MANAGER_H
