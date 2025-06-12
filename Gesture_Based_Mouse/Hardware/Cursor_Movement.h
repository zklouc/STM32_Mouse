#ifndef __CURSOR_MOVEMENT_H__
#define __CURSOR_MOVEMENT_H__

extern int8_t mouse_x,mouse_y;// 最终光标移动量

void TIM1_Init(void);
void Cursor_Init(void);
void ProcessSensorData(int16_t raw_ax, int16_t raw_ay);
#endif
