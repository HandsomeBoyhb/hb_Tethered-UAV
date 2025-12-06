#include "oled.h"
#include "cmsis_os.h"
#include "stdbool.h"

//初始化命令序列
uint8_t CMD_Data[]={
0xAE, 0xD5, 0x80, 0xA8, 0x3F, 0xD3, 0x00, 0x40,0xA1, 0xC8, 0xDA,

0x12, 0x81, 0xCF, 0xD9, 0xF1, 0xDB, 0x40, 0xA4, 0xA6,0x8D, 0x14,

0xAF};

/**
 * @function: void OLED_Init(void)
 * @description: OLED初始化
 * @return {*}
 */
void OLED_Init(void)
{
	HAL_Delay(500);  // 延长初始化延时
    
    // 发送初始化命令前先重置I2C
    HAL_I2C_DeInit(&hi2c1);
    osDelay(10);
    HAL_I2C_Init(&hi2c1);
    osDelay(10);
    
    uint8_t i = 0;
    for(i = 0; i < 23; i++) {
        OLED_WR_CMD(CMD_Data[i]);
        osDelay(2);  // 命令间增加延时
    }
    
    OLED_Clear();
    OLED_Display_On();
	
}

/**
 * @function: void OLED_WR_CMD(uint8_t cmd)
 * @description: 向设备写控制命令
 * @param {uint8_t} cmd 芯片手册规定的命令
 * @return {*}
 */
void OLED_WR_CMD(uint8_t cmd)
{
	HAL_StatusTypeDef status;
    uint8_t retry = 3;
    
    while(retry--) {
        status = HAL_I2C_Mem_Write(&hi2c1, 0x78, 0x00, I2C_MEMADD_SIZE_8BIT, &cmd, 1, 100);
        if(status == HAL_OK) break;
        
        // I2C错误恢复
        if(status == HAL_ERROR || status == HAL_BUSY || status == HAL_TIMEOUT) {
            HAL_I2C_DeInit(&hi2c1);
            osDelay(1);
            HAL_I2C_Init(&hi2c1);
            osDelay(1);
        }
    }
}

/**
 * @function: void OLED_WR_DATA(uint8_t data)
 * @description: 向设备写控制数据
 * @param {uint8_t} data 数据
 * @return {*}
 */
void OLED_WR_DATA(uint8_t data)
{
	HAL_StatusTypeDef status;
    uint8_t retry = 3;
    
    while(retry--) {
        status = HAL_I2C_Mem_Write(&hi2c1, 0x78, 0x40, I2C_MEMADD_SIZE_8BIT, &data, 1, 100);
        if(status == HAL_OK) break;
        
        // I2C错误恢复
        if(status == HAL_ERROR || status == HAL_BUSY || status == HAL_TIMEOUT) {
            HAL_I2C_DeInit(&hi2c1);
            osDelay(1);
            HAL_I2C_Init(&hi2c1);
            osDelay(1);
        }
    }
}

/**
 * @function: void OLED_On(void)
 * @description: 更新显示

 * @return {*}
 */
void OLED_On(void)
{
	uint8_t i,n;
	for(i=0;i<8;i++)
	{
		OLED_WR_CMD(0xb0+i);    //设置页地址（0~7）
		OLED_WR_CMD(0x00);      //设置显示位置—列低地址
		OLED_WR_CMD(0x10);      //设置显示位置—列高地址
		for(n=0;n<128;n++)
			OLED_WR_DATA(1);
	}
}


/**
 * @function: OLED_Clear(void)
 * @description: 清屏,整个屏幕是黑色的!和没点亮一样!!!
 * @return {*}
 */
void OLED_Clear(void)
{
	uint8_t i,n;
	for(i=0;i<8;i++)
	{
		OLED_WR_CMD(0xb0+i);    //设置页地址（0~7）
		OLED_WR_CMD(0x00);      //设置显示位置—列低地址
		OLED_WR_CMD(0x10);      //设置显示位置—列高地址
		for(n=0;n<128;n++)
			OLED_WR_DATA(0);
	}
}

/**
 * @function: void OLED_Display_On(void)
 * @description: 开启OLED显示
 * @return {*}
 */
void OLED_Display_On(void)
{
	OLED_WR_CMD(0X8D);  //SET DCDC命令
	OLED_WR_CMD(0X14);  //DCDC ON
	OLED_WR_CMD(0XAF);  //DISPLAY ON,打开显示
}


/**
 * @function: void OLED_Display_Off(void)
 * @description: 关闭OLED显示
 * @return {*}
 */
void OLED_Display_Off(void)
{
	OLED_WR_CMD(0X8D);  //SET DCDC命令
	OLED_WR_CMD(0X10);  //DCDC OFF
	OLED_WR_CMD(0XAE);  //DISPLAY OFF，关闭显示
}

/**
 * @function: void OLED_Set_Pos(uint8_t x, uint8_t y)
 * @description: 坐标设置
 * @param {uint8_t} x,y
 * @return {*}
 */
void OLED_Set_Pos(uint8_t x, uint8_t y)
{
	OLED_WR_CMD(0xb0+y);	//设置页地址（0~7）
	OLED_WR_CMD(((x&0xf0)>>4)|0x10); //设置显示位置—列高地址
	OLED_WR_CMD(x&0x0f);	//设置显示位置—列低地址
}


/**
 * @function: unsigned int oled_pow(uint8_t m,uint8_t n)
 * @description: m^n函数
 * @param {uint8_t} m,n
 * @return {unsigned int} result
 */
unsigned int oled_pow(uint8_t m,uint8_t n)
{
	unsigned int result=1;
	while(n--)result*=m;
	return result;
}

/**
 * @function: void OLED_ShowChar(uint8_t x, uint8_t y, uint8_t chr, uint8_t Char_Size,uint8_t Color_Turn)
 * @description: 在OLED12864特定位置开始显示一个字符
 * @param {uint8_t} x字符开始显示的横坐标
 * @param {uint8_t} y字符开始显示的纵坐标
 * @param {uint8_t} chr待显示的字符
 * @param {uint8_t} Char_Size待显示字符的字体大小,选择字体 16/12
 * @param {uint8_t} Color_Turn是否反相显示(1反相、0不反相)
 * @return {*}
 */
void OLED_ShowChar(uint8_t x,uint8_t y,uint8_t chr,uint8_t Char_Size,uint8_t Color_Turn)
{
	unsigned char c=0,i=0;
		c=chr-' ';//得到偏移后的值
		if(x>128-1){x=0;y=y+2;}
		if(Char_Size ==16)
		{
			OLED_Set_Pos(x,y);
			for(i=0;i<8;i++)
				{
				  if(Color_Turn)
					  OLED_WR_DATA(~F8X16[c*16+i]);
				  else
					  OLED_WR_DATA(F8X16[c*16+i]);
				}
			OLED_Set_Pos(x,y+1);
			for(i=0;i<8;i++)
			    {
				  if(Color_Turn)
					  OLED_WR_DATA(~F8X16[c*16+i+8]);
				  else
					  OLED_WR_DATA(F8X16[c*16+i+8]);
			    }

			}
	     else
	     {
				OLED_Set_Pos(x,y);
				for(i=0;i<6;i++)
			    {
				  if(Color_Turn)
					  OLED_WR_DATA(~F6x8[c][i]);
				  else
					  OLED_WR_DATA(F6x8[c][i]);
			    }
		  }
}

/**
 * @function: void OLED_ShowString(uint8_t x, uint8_t y, uint8_t *chr, uint8_tChar_Size, uint8_t Color_Turn)
 * @description: 在OLED12864特定位置开始显示字符串
 * @param {uint8_t} x待显示字符串的开始横坐标x:0~127
 * @param {uint8_t} y待显示字符串的开始纵坐标 y:0~7，若选择字体大小为16，则两行数字之间需要间隔2，若选择字体大小为12，间隔1
 * @param {uint8_t} *chr待显示的字符串
 * @param {uint8_t} Char_Size待显示字符串的字体大小,选择字体 16/12，16为8X16，12为6x8
 * @param {uint8_t} Color_Turn是否反相显示(1反相、0不反相)
 * @return {*}
 */
void OLED_ShowString(uint8_t x,uint8_t y,char*chr,uint8_t Char_Size, uint8_t Color_Turn)
{
	uint8_t  j=0;
	while (chr[j]!='\0')
	{		OLED_ShowChar(x,y,chr[j],Char_Size, Color_Turn);
			if (Char_Size == 12) //6X8的字体列加6，显示下一个字符
				x += 6;
			else  //8X16的字体列加8，显示下一个字符
				x += 8;

			if (x > 122 && Char_Size==12) //TextSize6x8如果一行不够显示了，从下一行继续显示
			{
				x = 0;
				y++;
			}
			if (x > 120 && Char_Size== 16) //TextSize8x16如果一行不够显示了，从下一行继续显示
			{
				x = 0;
				y++;
			}
			j++;
	}
}

/**
 * @function: void OLED_ShowNum(uint8_t x,uint8_t y,unsigned int num,uint8_t len,uint8_t size2, Color_Turn)
 * @description: 显示数字
 * @param {uint8_t} x待显示的数字起始横坐标,x:0~126
 * @param {uint8_t} y待显示的数字起始纵坐标, y:0~7，若选择字体大小为16，则两行数字之间需要间隔2，若选择字体大小为12，间隔1
 * @param {unsigned int} num:输入的数据
 * @param {uint8_t } len:输入的数据位数
 * @param {uint8_t} size2:输入的数据大小，选择 16/12，16为8X16，12为6x8
 * @param {uint8_t} Color_Turn是否反相显示(1反相、0不反相)
 * @return {*}
 */
void OLED_ShowNum(uint8_t x,uint8_t y,unsigned int num,uint8_t len,uint8_t size2, uint8_t Color_Turn)
{
	uint8_t t,temp;
	uint8_t enshow=0;
	for(t=0;t<len;t++)
	{
		temp=(num/oled_pow(10,len-t-1))%10;
		if(enshow==0&&t<(len-1))
		{
			if(temp==0)
			{
				OLED_ShowChar(x+(size2/2)*t,y,' ',size2, Color_Turn);
				continue;
			}else enshow=1;

		}
	 	OLED_ShowChar(x+(size2/2)*t,y,temp+'0',size2, Color_Turn);
	}
}

/**
 * @function: void OLED_Showdecimal(uint8_t x,uint8_t y,float num,uint8_t z_len,uint8_t f_len,uint8_t size2, uint8_t Color_Turn)
 * @description: 显示正负浮点数
 * @param {uint8_t} x待显示的数字起始横坐标,x:0~126
 * @param {uint8_t} y待显示的数字起始纵坐标, y:0~7，若选择字体大小为16，则两行数字之间需要间隔2，若选择字体大小为12，间隔1
 * @param {float} num:输入的浮点型数据
 * @param {uint8_t } z_ len:整数部分的位数
 * @param {uint8_t } f_len: 小数部分的位数
 * @param {uint8_t} size2:输入的数据大小，选择 16/12，16为8X16，12为6x8
 * @param {uint8_t} Color_Turn是否反相显示(1反相、0不反相)
 * @return {*}
 */
void OLED_Showdecimal(uint8_t x,uint8_t y,float num,uint8_t z_len,uint8_t f_len,uint8_t size2, uint8_t Color_Turn)
{
	uint8_t t,temp,i=0;//i为负数标志位
	uint8_t enshow;
	int z_temp,f_temp;
	if(num<0)
	{
		z_len+=1;
		i=1;
		num=-num;
	}
	z_temp=(int)num;
	//整数部分
	for(t=0;t<z_len;t++)
	{
		temp=(z_temp/oled_pow(10,z_len-t-1))%10;
		if(enshow==0 && t<(z_len-1))
		{
			if(temp==0)
			{
				OLED_ShowChar(x+(size2/2)*t,y,' ',size2, Color_Turn);
				continue;
			}
			else
			enshow=1;
		}
		OLED_ShowChar(x+(size2/2)*t,y,temp+'0',size2, Color_Turn);
	}
	//小数点
	OLED_ShowChar(x+(size2/2)*(z_len),y,'.',size2, Color_Turn);

	f_temp=(int)((num-z_temp)*(oled_pow(10,f_len)));
  //小数部分
	for(t=0;t<f_len;t++)
	{
		temp=(f_temp/oled_pow(10,f_len-t-1))%10;
		OLED_ShowChar(x+(size2/2)*(t+z_len)+5,y,temp+'0',size2, Color_Turn);
	}
	if(i==1)//如果为负，就将最前的一位赋值‘-’
	{
		OLED_ShowChar(x,y,'-',size2, Color_Turn);
		i=0;
	}
}


/**
 * @function: void OLED_DisplayMode(uint8_t mode)
 * @description: 屏幕内容取反显示
 * @param {uint8_t} direction			ON	0xA7  ，
 *                                                          OFF	0xA6	默认此模式，设置像素点亮
 * @return {*}
 */
void OLED_DisplayMode(uint8_t mode)
{
	OLED_WR_CMD(mode);
}

/**
 * @function: void OLED_IntensityControl(uint8_t intensity)
 * @description: 屏幕亮度调节
 * @param  {uint8_t} intensity	0x00~0xFF,RESET=0x7F
 * @return {*}
 */
void OLED_IntensityControl(uint8_t intensity)
{
	OLED_WR_CMD(0x81);
	OLED_WR_CMD(intensity);
}

/**
 * @function: void OLED_ShowIcon(uint8_t x, uint8_t y, const uint8_t *icon, uint8_t width, uint8_t height, uint8_t Color_Turn)
 * @description: 在指定位置显示图标
 * @param {uint8_t} x 起始横坐标 (0-127)
 * @param {uint8_t} y 起始纵坐标，以页为单位 (0-7)，1页=8行像素
 * @param {const uint8_t *} icon 图标点阵数据数组指针
 * @param {uint8_t} width 图标宽度 (像素)
 * @param {uint8_t} height 图标高度 (像素)
 * @param {uint8_t} Color_Turn 是否反相显示 (1反相, 0正常)
 * @return {*}
 */
void OLED_ShowIcon(uint8_t x, uint8_t y, const uint8_t *icon, uint8_t width, uint8_t height, uint8_t Color_Turn)
{
    uint8_t page, col;
    uint32_t index = 0; // 索引可能较大，用32位
    uint8_t pages = (height + 7) / 8; // 计算图标占用的页数

    for(page = 0; page < pages; page++)
    {
        OLED_Set_Pos(x, y + page);
        for(col = 0; col < width; col++)
        {
            // 根据Color_Turn决定是否反相显示数据
            if (Color_Turn)
                OLED_WR_DATA(~icon[index++]);
            else
                OLED_WR_DATA(icon[index++]);
        }
    }
}

// 显示16x16中文字符的函数
void OLED_ShowChinese16x16(uint8_t x, uint8_t y, uint8_t no, uint8_t Color_Turn)
{
    uint8_t t = 0;
    OLED_Set_Pos(x, y);
    for(t = 0; t < 16; t++)
    {
        if (Color_Turn)
            OLED_WR_DATA(~OLED_CF16x16[no][t]);
        else
            OLED_WR_DATA(OLED_CF16x16[no][t]);
    }

    OLED_Set_Pos(x, y + 1);
    for(t = 0; t < 16; t++)
    {
        if (Color_Turn)
            OLED_WR_DATA(~OLED_CF16x16[no][t + 16]);
        else
            OLED_WR_DATA(OLED_CF16x16[no][t + 16]);
    }
}

/**
  * @brief 进入OLED睡眠模式（息屏）
  * @retval None
  */
void OLED_SleepMode_Enter(void)
{
    // 发送关闭显示命令
    OLED_WR_CMD(0xAE);  // DISPLAY OFF
    
    // 可选：降低OLED功耗设置
    OLED_WR_CMD(0x81);  // Set Contrast Control
    OLED_WR_CMD(0x00);  // 最低对比度，进一步降低功耗
    
    // 关闭电荷泵
    OLED_WR_CMD(0x8D);  // Charge Pump Setting
    OLED_WR_CMD(0x10);  // Disable Charge Pump
}

/**
  * @brief 退出OLED睡眠模式（恢复显示）
  * @retval None
  */
void OLED_SleepMode_Exit(void)
{
    // 启用电荷泵
    OLED_WR_CMD(0x8D);  // Charge Pump Setting
    OLED_WR_CMD(0x14);  // Enable Charge Pump
    
    // 恢复对比度设置
    OLED_WR_CMD(0x81);  // Set Contrast Control
    OLED_WR_CMD(0xCF);  // 默认对比度
    
    // 开启显示
    OLED_WR_CMD(0xAF);  // DISPLAY ON
}

/**
  * @brief 获取OLED当前显示状态
  * @return bool true-显示开启，false-显示关闭
  */
bool OLED_GetDisplayState(void)
{
    return oledDisplayState;
}

/**
  * @brief 保存当前显示内容状态（可选功能）
  * @retval None
  * @note 如果需要精确恢复显示内容，可以实现此功能
  */
void OLED_SaveDisplayState(void)
{
    // 由于OLED控制器本身会保持显示内容，通常不需要软件保存
    // 此函数作为扩展接口预留
    displayContentSaved = true;
}

/**
  * @brief 恢复显示内容状态（可选功能）
  * @retval None
  */
void OLED_RestoreDisplayState(void)
{
    // 由于OLED控制器在睡眠期间保持内容，唤醒后自动恢复
    // 此函数作为扩展接口预留
    if (displayContentSaved) {
        // 如果需要特殊恢复逻辑，可以在这里实现
    }
}
