#include "stm32f10x.h"                  // Device header
#include "Delay.h"

//定时初始化
void TIM2_Init(void)
{
	/*开启时钟*/
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);			//开启TIM2的时钟
	
	/*时基单元初始化*/
	TIM_TimeBaseInitTypeDef TIM_TimeBaseInitStructure;				//定义结构体变量
	TIM_TimeBaseInitStructure.TIM_ClockDivision = TIM_CKD_DIV1;		//时钟分频，选择不分频，此参数用于配置滤波器时钟，不影响时基单元功能
	TIM_TimeBaseInitStructure.TIM_CounterMode = TIM_CounterMode_Up;	//计数器模式，选择向上计数
	TIM_TimeBaseInitStructure.TIM_Period = 2000 - 1;				//计数周期，即ARR的值 实现20ms定时中断
	TIM_TimeBaseInitStructure.TIM_Prescaler = 720 - 1;				//预分频器，即PSC的值
	TIM_TimeBaseInitStructure.TIM_RepetitionCounter = 0;			//重复计数器，高级定时器才会用到
	TIM_TimeBaseInit(TIM2, &TIM_TimeBaseInitStructure);				//将结构体变量交给TIM_TimeBaseInit，配置TIM2的时基单元	
	
	/*中断输出配置*/
	TIM_ClearFlag(TIM2, TIM_FLAG_Update);						//清除定时器更新标志位
																//TIM_TimeBaseInit函数末尾，手动产生了更新事件
																//若不清除此标志位，则开启中断后，会立刻进入一次中断
																//如果不介意此问题，则不清除此标志位也可
	TIM_ITConfig(TIM2, TIM_IT_Update, ENABLE);		
	/*NVIC中断分组*/
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);				//配置NVIC为分组2
																//即抢占优先级范围：0~3，响应优先级范围：0~3
																//此分组配置在整个工程中仅需调用一次
																//若有多个中断，可以把此代码放在main函数内，while循环之前
																//若调用多次配置分组的代码，则后执行的配置会覆盖先执行的配置
	
	/*NVIC配置*/
	NVIC_InitTypeDef NVIC_InitStructure;						//定义结构体变量
	NVIC_InitStructure.NVIC_IRQChannel = TIM2_IRQn;				//选择配置NVIC的TIM2线
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;				//指定NVIC线路使能
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 2;	//指定NVIC线路的抢占优先级为2
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;			//指定NVIC线路的响应优先级为1
	NVIC_Init(&NVIC_InitStructure);								//将结构体变量交给NVIC_Init，配置NVIC外设
	
	/*TIM使能*/
	TIM_Cmd(TIM2, ENABLE);			//使能TIM2，定时器开始运行
	TIM_ITConfig(TIM2, TIM_IT_Update, DISABLE);					//关闭TIM2的更新中断，当外部中断触发时才进行
}

//按键相关配置初始化
void Key_Init(void)
{
	TIM2_Init();
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);   //开启GPIOA的时钟
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);		//开启AFIO的时钟，外部中断必须开启AFIO的时钟
	//初始化GPIO引脚
	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1 | GPIO_Pin_2| GPIO_Pin_3;//左右中键
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
	
	/*AFIO选择中断引脚*/
	GPIO_EXTILineConfig(GPIO_PortSourceGPIOA, GPIO_PinSource1);//将外部中断的1号线映射到GPIOA，即选择PA1为外部中断引脚
	GPIO_EXTILineConfig(GPIO_PortSourceGPIOA, GPIO_PinSource2);//将外部中断的2号线映射到GPIOA，即选择PA2为外部中断引脚
	GPIO_EXTILineConfig(GPIO_PortSourceGPIOA, GPIO_PinSource3);//将外部中断的3号线映射到GPIOA，即选择PA3为外部中断引脚
	
  /*EXTI初始化*/
	EXTI_InitTypeDef EXTI_InitStructure;														//定义结构体变量
	EXTI_InitStructure.EXTI_Line=EXTI_Line1|EXTI_Line2|EXTI_Line3;	//选择配置外部中断的0号线和1号线
	EXTI_InitStructure.EXTI_LineCmd = ENABLE;												//指定外部中断线使能
	EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;							//指定外部中断线为中断模式
	EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Falling;					//指定外部中断线为下降沿触发，按键按下出现下降沿
	EXTI_Init(&EXTI_InitStructure);																	//将结构体变量交给EXTI_Init，配置EXTI外设 
	
	/*NVIC配置*/
	NVIC_InitTypeDef NVIC_InitStructure;										 	//定义结构体变量
	NVIC_InitStructure.NVIC_IRQChannel = EXTI1_IRQn;			   	//选择配置NVIC的EXTI1线
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;					 	//指定NVIC线路使能
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;	//指定NVIC线路的抢占优先级为1
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;			  //指定NVIC线路的响应优先级为0
	NVIC_Init(&NVIC_InitStructure);														//将结构体变量交给NVIC_Init，配置NVIC外设
	
	NVIC_InitStructure.NVIC_IRQChannel = EXTI2_IRQn;			 	  //选择配置NVIC的EXTI2线
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;					 	//指定NVIC线路使能
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;	//指定NVIC线路的抢占优先级为1
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 2;			  //指定NVIC线路的响应优先级为2
	NVIC_Init(&NVIC_InitStructure);														//将结构体变量交给NVIC_Init，配置NVIC外设

	NVIC_InitStructure.NVIC_IRQChannel = EXTI3_IRQn;			 	  //选择配置NVIC的EXTI3线
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;					 	//指定NVIC线路使能
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;	//指定NVIC线路的抢占优先级为1
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;			  //指定NVIC线路的响应优先级为3
	NVIC_Init(&NVIC_InitStructure);														//将结构体变量交给NVIC_Init，配置NVIC外设
	
}

//外部中断函数
void EXTI1_IRQHandler(void)
{
	if (EXTI_GetITStatus(EXTI_Line1) == SET)		//判断是否是外部中断1号线触发的中断
	{
		TIM_SetCounter(TIM2,0);//清除计数
		TIM_ITConfig(TIM2, TIM_IT_Update, ENABLE);					//开启TIM2的更新中断
		EXTI_ClearITPendingBit(EXTI_Line1);			//清除外部中断1号线的中断标志位
													//中断标志位必须清除
													//否则中断将连续不断地触发，导致主程序卡死
	}
}
//外部中断函数
void EXTI2_IRQHandler(void)
{
	if (EXTI_GetITStatus(EXTI_Line2) == SET)		//判断是否是外部中断2号线触发的中断
	{
		TIM_SetCounter(TIM2,0);//清除计数
		TIM_ITConfig(TIM2, TIM_IT_Update, ENABLE);	
		EXTI_ClearITPendingBit(EXTI_Line2);		
	}
}
//外部中断函数
void EXTI3_IRQHandler(void)
{
	if (EXTI_GetITStatus(EXTI_Line3) == SET)		//判断是否是外部中断3号线触发的中断
	{
		TIM_SetCounter(TIM2,0);//清除计数
		TIM_ITConfig(TIM2, TIM_IT_Update, ENABLE);	
		EXTI_ClearITPendingBit(EXTI_Line3);		
	}
}

extern uint8_t Key_Num;
extern uint8_t Mode_Flag;

void TIM2_IRQHandler(void)
{
	if (TIM_GetITStatus(TIM2, TIM_IT_Update) == SET)
	{
		TIM_ClearITPendingBit(TIM2, TIM_IT_Update);
		if(GPIO_ReadInputDataBit(GPIOA,GPIO_Pin_1)==RESET)//左键按下
		{
			Key_Num|=0x01;
		}
		else if(GPIO_ReadInputDataBit(GPIOA,GPIO_Pin_3)==RESET)//右键按下
		{
			Key_Num|=0x02;
		}
		else if(GPIO_ReadInputDataBit(GPIOA,GPIO_Pin_2)==RESET)//中键按下
		{
			//Key_Num|=0x04;
			Mode_Flag=~Mode_Flag;
		}
		TIM_ITConfig(TIM2, TIM_IT_Update, DISABLE);	
		
	}
}


