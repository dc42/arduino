// ST7920-based 128x64 pixel graphic LCD demo program
// Written by D. Crocker, Escher Technologies Ltd.

#include <avr/pgmspace.h>
#include "lcd7920.h"

extern PROGMEM LcdFont font10x10;    // in glcd10x10.cpp
extern PROGMEM LcdFont font16x16;    // in glcs16x16.cpp

// Define the following as true to use SPI to drive the LCD, false to drive it slowly instead
#define LCD_USE_SPI    (true)

// Definition of Arduino type
#if defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__)
#define  IS_MEGA  (1)
#define  IS_UNO   (0)
#else
#define  IS_MEGA  (0)
#define  IS_UNO   (1)
#endif

typedef unsigned char uint8_t;
typedef unsigned int uint16_t;

// Pins for serial LCD interface. For speed, we use the SPI interface, which means we need the MOSI and SCLK pins.
// Connection of the 7920-based GLCD:
//  Vss to Gnd
//  Vdd to +5v
//  Vo through 10k variable resistor to +5v (not needed if your board has a contrast adjust pot on the back already, like mine does)
//  RS to +5v
//  PSB to gnd
//  RW to Arduino MOSI (see below for pin mapping)
//  E to Arduino SCLK (see below for pin mapping)
//  RST to +5v
//  D0-D7 unconnected
//  BLK to ground (or collector of NPN transistor if controlling backlight by PWM)
//  BLA via series resistor (if not included in LCD module) to +5v or other supply

// The following pin numbers must not be changed because they must correspond to the processor MOSI and SCLK pins
// We don't actually use the MISO pin, but it gets forced as an input when we enable SPI
// The SS pin MUST be configured as an output (if it is used as an input and goes low, it will kill the communication with the LCD).
#if IS_UNO
const int MOSIpin = 11;
const int MISOpin = 12;
const int SCLKpin = 13;
const int SSpin = 10;
#else
const int MOSIpin = 51;
const int MISOpin = 50;
const int SCLKpin = 52;
const int SSpin = 53;
#endif

// End of configuration section

// LCD
static Lcd7920 lcd(SCLKpin, MOSIpin, LCD_USE_SPI);

// Initialization
void setup()
{
  pinMode(SSpin, OUTPUT);   // must do this before we use the lcd in SPI mode
  
  lcd.begin(true);          // init lcd in graphics mode
  }

// Find out how much free memory we have
unsigned int getFreeMemory()
{
  uint8_t* temp = (uint8_t*)malloc(16);    // assumes there are no free holes so this is allocated at the end
  unsigned int rslt = (uint8_t*)SP - temp;
  free(temp);
  return rslt;
}

void loop()
{
  lcd.setFont(&font10x10);
  lcd.setCursor(0, 0);
  lcd.print("Hello ");
  lcd.setFont(&font16x16);
  lcd.print("world!");
  
  lcd.circle(110, 31, 12, PixelSet);
  lcd.circle(110, 31, 16, PixelSet);
  lcd.line(3, 60, 127, 33, PixelFlip);

  lcd.setCursor(44, 14);
  lcd.setFont(&font10x10);
  lcd.print("Free RAM ");
  lcd.print(getFreeMemory());
  lcd.print(" bytes  ");
  lcd.flush();
  
  delay(200);
}

// End


