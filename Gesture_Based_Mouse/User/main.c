#include "stm32f10x.h"                  // Device header
#include "Delay.h"
#include "Serial.h"
#include "AD.h"
#include "LED.h"
#include "Key.h"
#include "Encoder.h"
#include "MPU6050.h"
#include <stdint.h>
#include "Cursor_Movement.h"

int16_t ax,ay,az;							//加速度，陀螺仪角速度
float BAT_Value;							//电量值
uint8_t bat_flag=0;           //电量监测标志
uint8_t Key_Num=0;            //按键数据
uint8_t Key_SendFlag;         //按键发送标志

int main(void)
{
	AD_Init();//adc采样初始化
	Key_Init();
	LED_Init();
	BAT_Value=((float)AD_GetValue()*3.3/4095/2.1*100);//电池电量
	Bat_LED_Show(BAT_Value);//电源灯
	Encoder_Init();//旋转编码器初始化
	Serial_Init();//串口初始化
	MPU6050_Init();//MPU6050初始化
	Cursor_Init();//加速度去除零偏
	TIM4_Init();//电量检测定时器
	TIM1_Init();//挥动按键模式防重复触发定时器
	while (1)
	{ 
		MPU6050_GetAcc(&ax,&ay,&az);//获得加速度
		ProcessSensorData(ax, ay);
		Serial_TxPacket[0]=Key_Num;//按键
		Serial_TxPacket[1]= (-mouse_x) ;//光标
		Serial_TxPacket[2]=(mouse_y) ;
		Serial_TxPacket[3]=Encoder_Get();//滚轮		
		Serial_SendPacket();
		if(Serial_TxPacket[0]!=0)
		Key_SendFlag++;//按键发送标志，对于按键需要多发生几次
		if(Key_SendFlag==3)  {Key_SendFlag=0;Key_Num=0;}	
    Serial_TxPacket[0]=0;
		Serial_TxPacket[1]=0;
		Serial_TxPacket[2]=0; 
		Serial_TxPacket[3]=0;
		if(bat_flag==20)
		{
			BAT_Value=((float)AD_GetValue()*3.3/4095/2.1*100);//电池电量
			//3.3是adc采样的满量程电压，4095是adc采样满量程，2.1是3.7v锂电池满电4.2v二分压，*100转换为百分比
			Bat_LED_Show(BAT_Value);//电源灯
			bat_flag=0;
		}
		Delay_ms(7);
	}
}

//3秒中断一次
void TIM4_IRQHandler(void)
{
	if (TIM_GetITStatus(TIM4, TIM_IT_Update) == SET)
	{
		TIM_ClearITPendingBit(TIM4, TIM_IT_Update);
		bat_flag++;
		
	}
}


