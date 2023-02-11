#include "Adafruit_ST7302.h"


static uint8_t st7302_init[] = {
  0x01, 0x38,       // High Power Mode ON
  0x02, 0xeb, 0x02, // NVM Load Enable with not be trigger by sleep Out.
  0x02, 0xd7, 0x68, // NVM Load Control with Enable ID1/ID/ID3 Load, with Enable Source High/Low Voltage Load
  0x02, 0xd1, 0x01, // Booster Enable switch, set enable
  0x02, 0xc0, 0x80, // Gate Voltage Control
  0x07, 0xc1, 0x28, 0x28, 0x28, 0x28, 0x14, 0x00,   // Source Voltage Control1: Source High Voltage set 5v, Gamma1 Voltage set 3.5v, Gamma2 Voltage set 1.5v
  0x05, 0xc2, 0x00, 0x00, 0x00, 0x00, // Source Voltage Control2: Source Low Voltage set 0v
  0x02, 0xcb, 0x14, // VCOMH Voltage Setting set 4v
  0x0b, 0xb4, 0xe5, 0x77, 0xf1, 0xff, 0xff, 0x4f, 0xf1, 0xff, 0xff, 0x4f, // Update Period Gate EQ Control
  0x01, 0x11,       // Sleep Out, This command turn off sleep mode.In this mode the DC/DC converter is enable, internal display oscillator is started, and panel scanning is started.
  0x03, 0xc7, 0xa6, 0xe9, // OSC Enable: set enable
  0x02, 0xb0, 0x40, // DutySetting: bug?? 0x64*4=400,  250/4 = 63.3 -> 64(0x40)
  0x02, 0x36, 0x00, // Memory Data Access Control: Page Address Order:Top to Bottom, Column Address Order:Left to Right, Page/Column Order:Column Direction Mode, Line Address Order: LCD Refresh Top to Bottom, Data Order:Left to Right, Gate Scan Order: Data refresh Top to Bottom
  0x02, 0x3a, 0x11, // Data Format Select: Data Up Down Switch->on, Bytes Per Pixel Select->3 write operations for 24-bit data
  0x02, 0xb9, 0x23, // Source Setting: Clear RAM ON/OFF -> OFF
  0x02, 0xb8, 0x09, // Panel Setting: Display Gate Scan Mode Select->Frame interval, Panel Layout Select ->One Line Interlace
  0x03, 0x2a, 0x05, 0x36, // Column Address Set: 5-54 ?? (S61~S182)
  0x03, 0x2b, 0x00, 0xc7, // Row Address Set: 0-199 (G1~G250)
  0x02, 0xd0, 0x1f, // UNKNONW COMMAND
  0x01, 0x29,       // Display On
  0x02, 0xb9, 0xe3, // Source Setting: Clear RAM ON/OFF -> ON
  0x02, 0xb9, 0x23, // Source Setting: Clear RAM ON/OFF -> OFF
  0x02, 0x72, 0x00, // NKNONW COMMAND
  0x01, 0x39,       // Low Power Mode ON
  0x00              // END-FLAG
};


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
  int32_t cmd_pos = 0;
  uint8_t cmd_sz = st7302_init[cmd_pos];
  while(cmd_sz){
    _send_command(st7302_init[cmd_pos + 1]);
    if(cmd_sz > 1){
      _send_params(&st7302_init[cmd_pos+2], cmd_sz - 1);
    }
    // next command
    cmd_pos += (cmd_sz + 1);
    cmd_sz = st7302_init[cmd_pos];
  }
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
