#include "stm32f10x.h"                  // Device header
#include "Delay.h"
#include "Serial.h"
#include "AD.h"
#include "LED.h"
#include "Key.h"
#include "Encoder.h"
#include "stdio.h"
#include "MPU6050.h"
#include "inv_mpu.h"
#include "inv_mpu_dmp_motion_driver.h"
#include <stdlib.h>  // 提供abs()函数声明
#include <math.h>    // 提供fabsf()函数声明


//数值限幅
#define CONSTRAIN(value, min, max)  ((value) < (min) ? (min) : ((value) > (max) ? (max) : (value)))

float Pitch,Roll,Yaw;								//俯仰角默认跟中值一样，翻滚角，偏航角
int16_t ax,ay,az,gx,gy,gz;							//加速度，陀螺仪角速度

uint8_t Buf[4];//hid通信四字节
float BAT_Value;//电量值
uint8_t bat_flag=0;//电量监测标志
uint8_t Key_Num=0;//按键值
uint8_t Key_SendFlag;//按键发送标志
 
volatile uint8_t Mode_Flag=0;//模式值
uint8_t key_delay_falg=0;//第二模式的按键防连续误触的延时标志

float angel_pitch,angel_roll,angel_yaw; //姿态角数据
uint8_t i;
static uint8_t TimeOUT;//尝试获取姿态角数据的最大次数
int8_t mouse_x = 0, mouse_y = 0;          // 最终光标移动量

void AirMouse_Update(float roll, float yaw,int8_t *xdata,int8_t *ydata)  ;
void TIM1_Init(void);

int main(void)
{
	AD_Init();//adc电量采集初始化
	Key_Init();//按键初始化
	LED_Init();//led灯初始化
	BAT_Value=((float)AD_GetValue()*3.3/4095/2.1*100);//电池电量
	Bat_LED_Show(BAT_Value);//电源灯
	Encoder_Init();//旋转编码器初始化
	Serial_Init();//串口初始化
	MPU6050_Init();//mpu6050初始化
	MPU6050_DMP_Init();//dmp库初始化
	TIM4_Init();//定时器4初始化
	TIM1_Init();
	while (1)
	{ 
		 while(MPU6050_DMP_Get_Data(&angel_pitch, &angel_roll, &angel_yaw));
//		while(MPU6050_DMP_Get_Data(&angel_pitch, &angel_roll, &angel_yaw)&&TimeOUT<3)//获取姿态角数据，如果返回值非0则会再次尝试读取
//		{
//			MPU6050_DMP_Get_Data(&angel_pitch, &angel_roll, &angel_yaw);//再次尝试获取姿态角数据
//			TimeOUT++;//尝试三次
//		}
//		TimeOUT%=3;//清除计数
		AirMouse_Update(angel_roll, angel_yaw, &mouse_x,&mouse_y);
		if(Mode_Flag==0){
			Serial_TxPacket[1]= (-mouse_x) ;//光标
		  Serial_TxPacket[2]=(mouse_y) ;
		}
		else if(key_delay_falg==0){
			if(mouse_x>8)
			{
				Key_Num|=0x01;//左键
				key_delay_falg=1;
				TIM_SetCounter(TIM1,0);//清除计数
		    TIM_ITConfig(TIM1, TIM_IT_Update, ENABLE);					//开启TIM1的更新中断
			}
			else if(mouse_x<(-8)){
				Key_Num|=0x02;//右键
				key_delay_falg=1;
				TIM_SetCounter(TIM1,0);//清除计数
		    TIM_ITConfig(TIM1, TIM_IT_Update, ENABLE);					//开启TIM1的更新中断
			}
		}	
		Serial_TxPacket[0]=Key_Num;//按键
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
		Delay_ms(5);
	}
}

//3秒中断一次
//定时器4中断函数
void TIM4_IRQHandler(void)
{
	if (TIM_GetITStatus(TIM4, TIM_IT_Update) == SET)
	{
		TIM_ClearITPendingBit(TIM4, TIM_IT_Update);
		bat_flag++;
		
	}
}

//定时器1初始化
void TIM1_Init(void)
{
	/*开启时钟*/
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_TIM1, ENABLE);			//开启TIM1的时钟
	
	/*时基单元初始化*/
	TIM_TimeBaseInitTypeDef TIM_TimeBaseInitStructure;				//定义结构体变量
	TIM_TimeBaseInitStructure.TIM_ClockDivision = TIM_CKD_DIV1;		//时钟分频，选择不分频，此参数用于配置滤波器时钟，不影响时基单元功能
	TIM_TimeBaseInitStructure.TIM_CounterMode = TIM_CounterMode_Up;	//计数器模式，选择向上计数
	TIM_TimeBaseInitStructure.TIM_Period = 5000 - 1;				//计数周期，即ARR的值 实现0.5S定时中断
	TIM_TimeBaseInitStructure.TIM_Prescaler = 7200 - 1;				//预分频器，即PSC的值
	TIM_TimeBaseInitStructure.TIM_RepetitionCounter = 0;			//重复计数器，高级定时器才会用到
	TIM_TimeBaseInit(TIM1, &TIM_TimeBaseInitStructure);				//将结构体变量交给TIM_TimeBaseInit，配置TIM1的时基单元	
	
	/*中断输出配置*/
	TIM_ClearFlag(TIM1, TIM_FLAG_Update);						//清除定时器更新标志位
																//TIM_TimeBaseInit函数末尾，手动产生了更新事件
																//若不清除此标志位，则开启中断后，会立刻进入一次中断
																//如果不介意此问题，则不清除此标志位也可
	TIM_ITConfig(TIM1, TIM_IT_Update, ENABLE);		
	/*NVIC中断分组*/
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);				//配置NVIC为分组2
																//即抢占优先级范围：0~3，响应优先级范围：0~3
																//此分组配置在整个工程中仅需调用一次
																//若有多个中断，可以把此代码放在main函数内，while循环之前
																//若调用多次配置分组的代码，则后执行的配置会覆盖先执行的配置
	
	/*NVIC配置*/
	NVIC_InitTypeDef NVIC_InitStructure;						//定义结构体变量
	NVIC_InitStructure.NVIC_IRQChannel = TIM1_UP_IRQn;				//选择配置NVIC的TIM1线
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;				//指定NVIC线路使能
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 2;	//指定NVIC线路的抢占优先级为2
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;			//指定NVIC线路的响应优先级为1
	NVIC_Init(&NVIC_InitStructure);								//将结构体变量交给NVIC_Init，配置NVIC外设
	
	/*TIM使能*/
	TIM_Cmd(TIM1, ENABLE);			//使能TIM1，定时器开始运行
	TIM_ITConfig(TIM1, TIM_IT_Update, DISABLE);					//关闭TIM1的更新中断，当外部中断触发时才进行
}


//定时器1中断函数
void TIM1_UP_IRQHandler(void)
{
	if (TIM_GetITStatus(TIM1, TIM_IT_Update) == SET)
	{
		TIM_ClearITPendingBit(TIM1, TIM_IT_Update);
		key_delay_falg=0;
		TIM_ITConfig(TIM1, TIM_IT_Update, DISABLE);	
		
	}
}



/**
 * @brief     将横滚角和偏航角数据转换为鼠标光标的xy轴移动数据
 * @param     横滚角和偏航角
 * @param     存储xy轴数据数组 
 * @retval    无
 */
void AirMouse_Update(float roll, float yaw,int8_t *xdata,int8_t *ydata) 
{
    // 参数定义
    const float CALIBRATION_THRESHOLD = 0.8f; // 限制零漂
    const float MAX_DELTA_ANGLE = 15.0f;     // 最大变化角度
    const float SENSITIVITY = 10.0f;          // 灵敏度系数
    const uint8_t DEAD_ZONE = 3;              //死区角度

    // 计算角度变化量
    static float last_roll = 0, last_yaw = 0;
    float delta_roll = roll - last_roll;
    float delta_yaw = yaw - last_yaw;
    last_roll = roll;
    last_yaw = yaw;
    
    // 动态零漂校准
    if(fabs(delta_roll) < CALIBRATION_THRESHOLD && 
       fabs(delta_yaw) < CALIBRATION_THRESHOLD) {
        delta_roll = delta_yaw = 0;
    }
    
    // 速度限制
    delta_roll = CONSTRAIN(delta_roll, -MAX_DELTA_ANGLE, MAX_DELTA_ANGLE);
    delta_yaw = CONSTRAIN(delta_yaw, -MAX_DELTA_ANGLE, MAX_DELTA_ANGLE);
    
    // 转换为有符号HID报告值，-1是保证选择方向和鼠标移动方向一致
    int8_t x_move = (int8_t)(delta_yaw * SENSITIVITY);
    int8_t y_move = (int8_t)(delta_roll * SENSITIVITY)*(-1); 
		
		// 死区处理，即小于这个值的数据会被清零，减小误差，abs函数是取绝对值函数
    if(abs(x_move) < DEAD_ZONE) x_move = 0;
    if(abs(y_move) < DEAD_ZONE) y_move = 0;

     *xdata=(x_move & 0xFF);
     *ydata=(y_move & 0xFF);
 
}


