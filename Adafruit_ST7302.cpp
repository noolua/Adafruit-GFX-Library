#include "Adafruit_ST7302.h"


Adafruit_ST7302::Adafruit_ST7302(int8_t SID, int8_t SCLK, int8_t DC, int8_t RST, int8_t CS)
  :Adafruit_GFX(ST7302_WIDTH, ST7302_HEIGHT){
  _sid = SID;
  _sclk = SCLK;
  _dc = DC;
  _rst = RST;
  _cs = CS;
  memset(_frame_buffer, 0, sizeof(_frame_buffer));
}

void Adafruit_ST7302::begin(int32_t freq){
  pinMode(_dc, OUTPUT);
  pinMode(_cs, OUTPUT);
  pinMode(_rst, OUTPUT);
  _csport      = portOutputRegister(digitalPinToPort(_cs));
  _cspinmask   = digitalPinToBitMask(_cs);
  _dcport      = portOutputRegister(digitalPinToPort(_dc));
  _dcpinmask   = digitalPinToBitMask(_dc);
  SPI.begin(_sclk, -1, _sid, _cs);
  SPI.beginTransaction(SPISettings(freq, MSBFIRST, SPI_MODE0));

  digitalWrite(_rst, LOW);
  delay(100);
  digitalWrite(_rst, HIGH);
  delay(100);

  /*
    Init LCD Driver
    REF: ST7302 DATASHEET
  */
  _send_command(0x38);       // High Power Mode ON
  _send_command(0xeb);       // NVM Load Enable with not be trigger by sleep Out.
  _send_param(0x02);
  _send_command(0xd7);       // NVM Load Control with Enable ID1/ID/ID3 Load, with Enable Source High/Low Voltage Load
  _send_param(0x68);
  _send_command(0xd1);       // Booster Enable switch, set enable
  _send_param(0x01);
  _send_command(0xc0);       // Gate Voltage Control
  _send_param(0x80);
  _send_command(0xc1);       // Source Voltage Control1: Source High Voltage set 5v, Gamma1 Voltage set 3.5v, Gamma2 Voltage set 1.5v
  uint8_t svc1[] = {0x28,0x28,0x28,0x28,0x14,0x00};
  _send_params(svc1, sizeof(svc1));
  _send_command(0xc2);       // Source Voltage Control2: Source Low Voltage set 0v
  uint8_t svc2[] = {0x00, 0x00, 0x00, 0x00};
  _send_params(svc2, sizeof(svc2));
  _send_command(0xcb);       // VCOMH Voltage Setting set 4v
  _send_param(0x14);
  _send_command(0xb4);       // Update Period Gate EQ Control
  uint8_t upgec[] = {0xe5,0x77,0xf1,0xff,0xff,0x4f,0xf1,0xff,0xff,0x4f};
  _send_params(upgec, sizeof(upgec));
  _send_command(0x11);       // Sleep Out, This command turn off sleep mode.In this mode the DC/DC converter is enable, internal display oscillator is started, and panel scanning is started.
  delay(100);

  _send_command(0xc7);       // OSC Enable: set enable
  _send_param(0xa6);
  _send_param(0xe9);

  _send_command(0xb0);       // DutySetting: bug?? 0x64*4=400,  250/4 = 63.3 -> 64(0x40)
  _send_param(0x40);
  _send_command(0x36);       // Memory Data Access Control: Page Address Order:Top to Bottom, Column Address Order:Left to Right, Page/Column Order:Column Direction Mode, Line Address Order: LCD Refresh Top to Bottom, Data Order:Left to Right, Gate Scan Order: Data refresh Top to Bottom
  _send_param(0x00);
  _send_command(0x3a);       // Data Format Select: Data Up Down Switch->on, Bytes Per Pixel Select->3 write operations for 24-bit data
  _send_param(0x11);
  _send_command(0xb9);       // Source Setting: Clear RAM ON/OFF -> OFF
  _send_param(0x23);
  _send_command(0xb8);       // Panel Setting: Display Gate Scan Mode Select->Frame interval, Panel Layout Select ->One Line Interlace
  _send_param(0x09);
  _send_command(0x2a);       // Column Address Set: 5-54 ?? (S61~S182)
  _send_param(0x05);
  _send_param(0x36);

  _send_command(0x2b);       // Row Address Set: 0-199 (G1~G250)
  _send_param(0x00);
  _send_param(0xc7);
  _send_command(0xD0);       // UNKNONW COMMAND
  _send_param(0x1f);

  _send_command(0x29);       // Display On
  _send_command(0xb9);      // Source Setting: Clear RAM ON/OFF -> ON
  _send_param(0xe3);
  delay(100);      

  _send_command(0xb9);       // Source Setting: Clear RAM ON/OFF -> OFF
  _send_param(0x23);
  _send_command(0x72);       // UNKNONW COMMAND
  _send_param(0x00);
  _send_command(0x39);       // Low Power Mode ON
  _send_command(0x2a);       // Column Address Set: 0x19-0x23
  _send_param(0x19);
  _send_param(0x23);
  _send_command(0x2b);       // Row Address Set: 0-124
  _send_param(0x0);
  _send_param(0x7c);
  _send_command(0x2c);       // Write Frame Data    
  delay(120);
}

void Adafruit_ST7302::clearDisplay(){
  // _send_command(0x28);
  memset(_frame_buffer, 0, sizeof(_frame_buffer));
}

void Adafruit_ST7302::display(){
  // _send_command(0x29);
  _flush_frame();
}


/*
def LCDSetPixel(x, y):
    x2 = x//2
    y4 = y//4

    y2 = y-y4*4
    mask = 4**(3-y2)

    if x-x2*2 == 0: mask *= 2
    lcdbmp[x2*0x21+y4] |= mask
————————————————
//https://blog.csdn.net/zhuoqingjoking97298/article/details/125880583#987000
*/
void Adafruit_ST7302::drawPixel(int16_t x, int16_t y, uint16_t color){
  if(color){
    int x2 = x >> 1;
    int y4 = y >> 2;
    int y2 = y % 4;
    uint8_t mask = 1 << ((3-y2) << 1);
    if (x % 2 == 0){
      mask = mask << 1;
    }
    _frame_buffer[x2*LCD_ROW+y4] |= mask;
  }
}

void Adafruit_ST7302::_flush_frame() {
  /*
    Set LCDAddressSet(0, 11, 0, 125)
    11 = LCD_ROW * 8 / 24;
    125 = LCD_COLUNM;
  */ 
  _send_command(0x2a);
  _send_param(0x19);
  _send_param(0x19+10);   // 11 - 1
  _send_command(0x2b);
  _send_param(0x00);
  _send_param(124);       // 125 - 1
  _send_command(0x2c);

  //digitalWrite(_cs, LOW);
  *_csport &= ~_cspinmask;
  SPI.transfer(_frame_buffer, sizeof(_frame_buffer));
  //digitalWrite(_cs, HIGH);
  *_csport |= _cspinmask;
}

void Adafruit_ST7302::_send_param(uint8_t param) {
  //digitalWrite(_cs, LOW);
  *_csport &= ~_cspinmask;
  //digitalWrite(_dc, HIGH);
  *_dcport |= _dcpinmask;
  SPI.transfer(param);
  //digitalWrite(_cs, HIGH);
  *_csport |= _cspinmask; 
}

void Adafruit_ST7302::_send_params(uint8_t *params, int params_sz) {
  for(int i = 0; i < params_sz; i++){
    _send_param(params[i]);
  }
}

void Adafruit_ST7302::_send_command(uint8_t cmd) {
  // digitalWrite(_cs, LOW);
  *_csport &= ~_cspinmask;
  // digitalWrite(_dc, LOW);
  *_dcport &= ~_dcpinmask;
  SPI.transfer(cmd);
  // digitalWrite(_dc, HIGH);
  *_dcport |= _dcpinmask;
  // digitalWrite(_cs, HIGH);
  *_csport |= _cspinmask; 

}
