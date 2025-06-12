#include <BleMouse.h>
#include <HardwareSerial.h>

#define UART_RX_PIN 1    // 根据实际接线调整
#define UART_TX_PIN 0    // 根据实际接线调整
#define LED_BUILTIN1 12

uint8_t buttons;
uint8_t temp_data[4];
int8_t x;           // X轴移动(-127~127)
int8_t y;           // Y轴移动(-127~127)
int8_t wheel;       // 滚轮(-127~127)
uint8_t state;
uint8_t byteCount;

BleMouse bleMouse("BTMouse", "Espressif", 100);//蓝牙设备名称，制造商名称，设备的初始电量百分比
HardwareSerial SerialUART(1);  // 使用UART1

void setup() {
  Serial.begin(115200);
  SerialUART.begin(115200, SERIAL_8N1, UART_RX_PIN, UART_TX_PIN);//8位数据位，无校验位，1位停止位
  pinMode(LED_BUILTIN1, OUTPUT);
  digitalWrite(LED_BUILTIN1, LOW);
  Serial.println("初始化体感式蓝牙鼠标...");
  bleMouse.begin();
  Serial.println("蓝牙模块初始化完成");
}

// the loop function runs over and over again forever
void loop() {
  // 蓝牙连接状态指示
  digitalWrite(LED_BUILTIN1, bleMouse.isConnected() ? HIGH : LOW);
  if (SerialUART.available() > 0) {
    uint8_t byte = SerialUART.read();

    switch (state) {
    case 0: // 等待包头
      if (byte == 0xFF) {
        state = 1;
        byteCount = 0;
      }
      break;
      
    case 1: // 接收数据部分
      temp_data[byteCount++] =byte;
      if (byteCount >= 4) { 
        state = 2;
      }
      break;
      
    case 2: // 检查包尾
      if (byte == 0xFE) {
        byteCount =0;
        // Serial.write(temp_data,4);
        processHIDPacket(temp_data);
        state = 0; // 重置状态
      }
      break;
    }
  }
}


// 处理接收到的HID数据包
void processHIDPacket(uint8_t *data) {
  if (!bleMouse.isConnected()) return;
  buttons=data[0];
  x=data[1];           // X轴移动(-127~127)
  y=data[2];           // Y轴移动(-127~127)
  wheel=data[3];           // 滚轮(-127~127)
  // 1. 处理按键状态
  if (buttons != 0) {
    if (buttons & 0x01) bleMouse.press(MOUSE_LEFT);
    else if (buttons & 0x02) bleMouse.press(MOUSE_RIGHT);
    else if (buttons & 0x04) bleMouse.press(MOUSE_MIDDLE);
  }
  else {
    bleMouse.release(MOUSE_LEFT);
    bleMouse.release(MOUSE_RIGHT);
    bleMouse.release(MOUSE_MIDDLE);
  }
  // 2. 处理鼠标移动
  if (x != 0 || y != 0) {
    bleMouse.move(x, y, 0);//有效范围是 -127 到 127
  }
  // 3. 处理滚轮
  if (wheel != 0) {
    bleMouse.move(0, 0, wheel);
  }
  
}
