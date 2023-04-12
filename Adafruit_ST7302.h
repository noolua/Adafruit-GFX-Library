#include <SPI.h>
#include <Adafruit_GFX.h>

/*
  REF:
    https://blog.csdn.net/zhuoqingjoking97298/article/details/125880583
*/

#define ST7302_WIDTH    250
#define ST7302_HEIGHT   122
#define ST7302_WIDTH_PAD 256
#define TFT_IC_ST7302   "ST7302"
#define TFT_IC_ST7305   "ST7305"


#define LCD_COLUNM      (ST7302_WIDTH/2)
#define LCD_ROW         (ST7302_HEIGHT/4+3)     // 122/4 = floor(30.5)+3 = 33
#define LCD_FRAME_SIZE  (LCD_COLUNM * LCD_ROW)  // 125 * 33 = 4125 (bytes)
#define LCD_WINDOW_SIZE ((ST7302_WIDTH_PAD*ST7302_HEIGHT)/8)  // (250+6)*122/8 = 3904 (bytes)

class Adafruit_ST7302 : public Adafruit_GFX {
public:
  Adafruit_ST7302(int8_t SID, int8_t SCLK, int8_t DC, int8_t RST, int8_t CS);
  void begin(int32_t ic7302=0, int32_t freq=8000000);
  void clearDisplay();
  void display();
  virtual void drawPixel(int16_t x, int16_t y, uint16_t color);
  inline uint8_t *buffer() {return _window_buffer;}
protected:
  uint32_t _convert2frame();
  void _flush_frame();
  void _send_param(uint8_t param);
  void _send_params(const uint8_t *params, int params_sz);
  void _send_command(uint8_t cmd);
  int8_t _sid, _sclk, _dc, _rst, _cs;
  int32_t _reserved_aligned0;
  uint8_t _frame_buffer[LCD_FRAME_SIZE];
  int32_t _reserved_aligned1;
  uint8_t _window_buffer[LCD_WINDOW_SIZE];
  volatile uint32_t *_csport, *_dcport;
  volatile uint32_t _cspinmask, _dcpinmask;  
};