---
typora-root-url: ./
---
# 支持UTF8的改动说明



### 效果

![README_zh.jpg](/README_zh.jpg)



### 主要改动

新增一个字体类型 uufont_t，定义位于uufont.h

```cpp
/* 
	新增一个设置uufont字体的方法，在设置这类字体后。
	Adafruit_GFX::printXX系列的将优先使用该字体进行绘制。
	从而实现对utf8的支持，具体修改请看代码部分
*/
void Adafruit_GFX::setUUFont(const uufont_t *f = NULL);
```
##### （一）修改以下文件
- Adafruit_GFX.h
- Adafruit_GFX.cpp

##### （二）新增以下文件
- uufont.h
- uufont.cpp

##### （三）新增两个字体文件
- Fonts/Dinkie7_8pt.h  丁卯点阵体试用版 (字体大小 (80.332KB),  glyphs: 2574)
- Fonts/Dinkie9_10pt.h 丁卯点阵体试用版 (字体大小 (90.128KB),  glyphs: 2574)

### 参考

- https://github.com/adafruit/Adafruit-GFX-Library
- https://github.com/adafruit/Adafruit-GFX-Library/pull/244
- [https://github.com/joeycastillo/Adafruit-GFX-Library](https://github.com/joeycastillo/Adafruit-GFX-Library)
- [BillyDonahue/utf8_decoder.cpp](https://gist.github.com/BillyDonahue/232420eb6eeeee4130c7803c4d59f1bd)



### 测试代码

```c
#include <Arduino.h>
#include "gfxfont.h"
#include "uufont.h"
#include "Fonts/dinkie7_8pt.h"
#include "Fonts/dinkie9_10pt.h"
#include "Fonts/FreeMono9pt7b.h"

#define PxMATRIX_MAX_WIDTH 64
#define PxMATRIX_MAX_HEIGHT 32
#define double_buffer
#include <PxMatrix.h>

#define DRAW_TIME   15
hw_timer_t * timer = NULL;
portMUX_TYPE timerMux = portMUX_INITIALIZER_UNLOCKED;
TaskHandle_t displayUpdateTaskHandle = NULL;

#define LED_P_LAT 22
#define LED_P_A 19
#define LED_P_B 23
#define LED_P_C 18
#define LED_P_D 5
#define LED_P_E 15
#define LED_P_OE 16



PxMATRIX dp(64,32,LED_P_LAT, LED_P_OE,LED_P_A,LED_P_B,LED_P_C,LED_P_D);
uint16_t clrDark = dp.color565(32, 32, 32);
uint16_t clrBlue = dp.color565(48, 52, 109);

void DisplayUpdateTask(void *) {
  while(true) {
    if (ulTaskNotifyTake( pdTRUE, portMAX_DELAY))
      dp.display(DRAW_TIME);
  }
}

void IRAM_ATTR display_updater(){
  portENTER_CRITICAL_ISR(&timerMux);
  BaseType_t xHigherPriorityTaskWoken = pdFALSE;
  vTaskNotifyGiveFromISR(displayUpdateTaskHandle, &xHigherPriorityTaskWoken);
  if(xHigherPriorityTaskWoken)
    portYIELD_FROM_ISR();
  portEXIT_CRITICAL_ISR(&timerMux);
}

void display_update_enable(bool is_enable){
  if (is_enable){
    timer = timerBegin(0, 80, true);
    timerAttachInterrupt(timer, &display_updater, true);
    timerAlarmWrite(timer, 4000, true);
    timerAlarmEnable(timer);
  }else{
    timerDetachInterrupt(timer);
    timerAlarmDisable(timer);
  }
}

void setup() {
  Serial.begin(115200);
  dp.begin(16);
  dp.setColorOrder(RRBBGG);
  dp.clearDisplay();
  dp.setBrightness(255);
  xTaskCreatePinnedToCore(DisplayUpdateTask, "DisplayTask", 2048, NULL, 3, &displayUpdateTaskHandle, 1);
  display_update_enable(true);
}

void loop(){
  int tick = millis()/2000;

  dp.clearDisplay();
  switch (tick % 5){
  case 0:
    dp.setTextColor(clrDark);
    dp.setUUFont(&Dinkie7demo_8pt);
    dp.setCursor(4, 7);
    dp.print("0123456789");
    dp.setTextColor(clrBlue);
    dp.setUUFont(&Dinkie9demo_10pt);
    dp.setCursor(6, 17);
    dp.print("0123456789");
    dp.setUUFont(NULL);
    dp.setFont(&FreeMono9pt7b);
    dp.setCursor(4, 30);
    dp.print("0123456789");
    break;  
  case 1:
    dp.setTextColor(clrDark);
    dp.setUUFont(&Dinkie7demo_8pt);
    dp.setCursor(5, 8);
    dp.print("Hello World");
    dp.setTextColor(clrBlue);
    dp.setUUFont(&Dinkie9demo_10pt);
    dp.setCursor(7, 18);
    dp.print("Hello World");
    break;  
  case 2:
    dp.setTextColor(clrDark);
    dp.setUUFont(&Dinkie7demo_8pt);
    dp.setCursor(6, 9);
    dp.print("你好，ESP32");
    dp.setTextColor(clrBlue);
    dp.setUUFont(&Dinkie9demo_10pt);
    dp.setCursor(8, 19);
    dp.print("你好，ESP32");
    break;  
  case 3:
    dp.setTextColor(clrDark);
    dp.setUUFont(&Dinkie7demo_8pt);
    dp.setCursor(5, 8);
    dp.print("丁卯点阵体!");
    dp.setTextColor(clrBlue);
    dp.setUUFont(&Dinkie9demo_10pt);
    dp.setCursor(7, 18);
    dp.print("丁卯点阵体!");
    break;  
  case 4:
    dp.setTextColor(clrDark);
    dp.setUUFont(&Dinkie7demo_8pt);
    dp.setCursor(6, 9);
    dp.print("爱屋及呜呜");
    dp.setTextColor(clrBlue);
    dp.setUUFont(&Dinkie9demo_10pt);
    dp.setCursor(8, 19);
    dp.print("爱屋及呜呜");
    break;  
  default:
    break;
  }
  delay(2000);
  dp.showBuffer();
}
```

