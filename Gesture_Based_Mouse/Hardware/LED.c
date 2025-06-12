#include "stm32f10x.h"                  // Device header

#define BAT_Alarm_Value 20

void LED_Init(void)
{
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
	
	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8 | GPIO_Pin_9;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOB, &GPIO_InitStructure);
	
//	GPIO_SetBits(GPIOB, GPIO_Pin_8 | GPIO_Pin_9);
}

//灯开
void LED1_ON(void)
{
	GPIO_ResetBits(GPIOB, GPIO_Pin_8);
}

//灯开
void LED1_OFF(void)
{
	GPIO_SetBits(GPIOB, GPIO_Pin_8);
}

void LED1_Turn(void)
{
	if (GPIO_ReadOutputDataBit(GPIOB, GPIO_Pin_8) == 0)
	{
		GPIO_SetBits(GPIOB, GPIO_Pin_8);
	}
	else
	{
		GPIO_ResetBits(GPIOB, GPIO_Pin_8);
	}
}

void LED2_ON(void)
{
	GPIO_ResetBits(GPIOB, GPIO_Pin_9);
}

void LED2_OFF(void)
{
	GPIO_SetBits(GPIOB, GPIO_Pin_9);
}

void LED2_Turn(void)
{
	if (GPIO_ReadOutputDataBit(GPIOB, GPIO_Pin_9) == 0)
	{
		GPIO_SetBits(GPIOB, GPIO_Pin_9);
	}
	else
	{
		GPIO_ResetBits(GPIOB, GPIO_Pin_9);
	}
}

//电源灯显示
void Bat_LED_Show(uint8_t val)
{
	if(val>=BAT_Alarm_Value)
	{
		LED2_ON();
		LED1_OFF();
	}
	else {
		LED1_ON();
		LED2_OFF();
	}
}

