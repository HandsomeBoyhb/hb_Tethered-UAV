#ifndef OLED_OLED_H_
#define OLED_OLED_H_

#include "stm32f1xx_hal.h"
#include "oledfont.h"
#include "stdbool.h"
extern I2C_HandleTypeDef  hi2c1;

void OLED_WR_CMD(uint8_t cmd);
void OLED_WR_DATA(uint8_t data);
void OLED_Init(void);
void OLED_Clear(void);
void OLED_Display_On(void);
void OLED_Display_Off(void);
void OLED_Set_Pos(uint8_t x, uint8_t y);
void OLED_On(void);
void OLED_ShowNum(uint8_t x,uint8_t y,unsigned int num,uint8_t len,uint8_t size2,uint8_t Color_Turn);
void OLED_Showdecimal(uint8_t x,uint8_t y,float num,uint8_t z_len,uint8_t f_len,uint8_t size2, uint8_t Color_Turn);
void OLED_ShowChar(uint8_t x,uint8_t y,uint8_t chr,uint8_t Char_Size,uint8_t Color_Turn);
void OLED_ShowString(uint8_t x,uint8_t y,char*chr,uint8_t Char_Size,uint8_t Color_Turn);
void OLED_ShowCHinese(uint8_t x,uint8_t y,uint8_t no,uint8_t Color_Turn);
void OLED_DisplayMode(uint8_t mode);
void OLED_IntensityControl(uint8_t intensity);
void OLED_ShowIcon(uint8_t x, uint8_t y, const uint8_t *icon, uint8_t width, uint8_t height, uint8_t Color_Turn);

// OLED显示状态控制函数
void OLED_SleepMode_Enter(void);      // 进入睡眠模式（息屏）
void OLED_SleepMode_Exit(void);       // 退出睡眠模式（恢复显示）
bool OLED_GetDisplayState(void);      // 获取当前显示状态
void OLED_SaveDisplayState(void);     // 保存当前显示内容状态
void OLED_RestoreDisplayState(void);  // 恢复显示内容
extern bool oledDisplayState;  // OLED显示状态
extern bool displayContentSaved;// 显示内容是否已保存


void OLED_ShowChinese16x16(uint8_t x, uint8_t y, uint8_t no, uint8_t Color_Turn);
#endif /* OLED_OLED_H_ */

