#include <lcd7920.h>
#include <RotaryEncoder.h>
#include <PushButton.h>

#define DEBUG_OUTPUT  (0)

extern const PROGMEM LcdFont font10x10; 

// Induction balance metal detector

// We run the CPU at 16MHz and the ADC clock at 1MHz. ADC resolution is reduced to 8 bits at this speed.

// Timer 1 is used to divide the system clock by about 256 to produce a 62.5kHz square wave. 
// This is used to drive timer 0 and also to trigger ADC conversions.
// Timer 0 is used to divide the output of timer 1 by 8, giving a 7.8125kHz signal for driving the transmit coil.
// This gives us 16 ADC clock cycles for each ADC conversion (it actually takes 13.5 cycles), and we take 8 samples per cycle of the coil drive voltage.
// The ADC implements four phase-sensitive detectors at 45 degree intervals. Using 4 instead of just 2 allows us to cancel the third harmonic of the
// coil frequency.

// Timer 2 will be used to generate a tone for the earpiece or headset.

// Other division ratios for timer 1 are possible, from about 235 upwards.

// Wiring:
// Connect digital pin 4 (alias T0) to digital pin 9
// Connect digital pin 5 through resistor to primary coil and tuning capacitor
// Connect output from receive amplifier to analog pin 0. Output of receive amplifier should be biased to about half of the analog reference.
// When using USB power, change analog reference to the 3.3V pin, because there is too much noise on the +5V rail to get good sensitivity.

#define TIMER1_TOP  (244)         // can adjust this to fine-tune the frequency to get the coil tuned (see above)

#define USE_3V3_AREF  (1)         // set to 1 if running on an Arduino with USB power, 0 for an embedded atmega328p with no 3.3V supply available

// Digital pin definitions
// Digital pin 0 not used, however if we are using the serial port for debugging then it's serial input
const int debugTxPin = 1;         // transmit pin reserved for debugging

const int BuzzerPin = 3;          // earpiece, aka OCR2B for tone generation
const int T0InputPin = 4;
const int coilDrivePin = 5;
const int T0OutputPin = 9;

const int EncoderAPin = 6;
const int EncoderBPin = 7;
const int EncoderButtonPin = 8;
const int LcdCsPin = 10;          // LCD chip select (active high for 12964)
const int LcdDataPin = 11;        // pins 11-13 also used for ICSP
const int LcdMosiPin = 12;        // pin 12 is not used, so we enable its pullup to keep it high
const int LcdSclkPin = 13;

// Analog pin definitions
const int receiverInputPin = 0;
// Analog pins 1-5 not used

const int EncoderPulsesPerClick = 4;

// LCD rows
const uint16_t row0 = 0;
const uint16_t row1 = 11;
const uint16_t row2 = 22;
const uint16_t row3 = 33;
const uint16_t row4 = 44;
const uint16_t row5 = 55;

// Variables used only by the ISR
int16_t bins[4];                 // bins used to accumulate ADC readings, one for each of the 4 phases
uint16_t numSamples = 0;
const uint16_t numSamplesToAverage = 1024;

// Variables used by the ISR and outside it
volatile int16_t averages[4];    // when we've accumulated enough readings in the bins, the ISR copies them to here and starts again
volatile uint16_t ticks = 0;     // system tick counter for timekeeping
volatile bool sampleReady = false;  // indicates that the averages array has been updated
bool printCalibration = true;
bool printSensitivity = true;

// Variables used only outside the ISR
int16_t calib[4];                // values (set during calibration) that we subtract from the averages

volatile uint8_t lastctr;
volatile uint16_t misses = 0;    // this counts how many times the ISR has been executed too late. Should remain at zero if everything is working properly.
uint32_t lastPollTime = 0;
const uint16_t PollInterval = 256; // Poll the button and the encoder every 256 ticks = every 4.096ms

const double halfRoot2 = sqrt(0.5);
const double quarterPi = 3.1415927/4.0;
const double radiansToDegrees = 180.0/3.1415927;

// The ADC sample and hold occurs 2 ADC clocks (= 32 system clocks) after the timer 1 overflow flag is set.
// This introduces a slight phase error, which we adjust for in the calculations.
const float phaseAdjust = (float)((45.0 * 32.0)/(double)(TIMER1_TOP + 1));

int sensitivity = 5;              // lower = greater sensitivity. This is multipled by 5 to get the threshold.
float threshold;

Lcd7920 *lcd;
RotaryEncoder *encoder;
PushButton *button;

void setup()
{
  digitalWrite(T0OutputPin, LOW);
  pinMode(T0OutputPin, OUTPUT);       // pulse pin from timer 1 used to feed timer 0
  digitalWrite(coilDrivePin, LOW);
  pinMode(coilDrivePin, OUTPUT);      // timer 0 output, square wave to drive transmit coil
  pinMode(LcdMosiPin, INPUT_PULLUP);
  pinMode(BuzzerPin, OUTPUT);
  
  lcd = new Lcd7920(LcdSclkPin, LcdDataPin, LcdCsPin, false /*true*/);
  lcd->begin();
  button = new PushButton(EncoderButtonPin);
  button->init();
  encoder = new RotaryEncoder(EncoderAPin, EncoderBPin, EncoderPulsesPerClick);
  encoder->init();

  lcd->setFont(&font10x10);
  lcd->setCursor(row0, 0);
  lcd->clear();
  lcd->print("IB Metal Detector v0.0");
  lcd->flush();
  lcd->setRightMargin(128);

  // WARNING! Do not call delay() or millis() after here, because the following code takes over the timer that Arduino uses for its tick counter

  cli();
  // Stop timer 0 which was set up by the Arduino core
  TCCR0B = 0;        // stop the timer
  TIMSK0 = 0;        // disable interrupt
  TIFR0 = 0x07;      // clear any pending interrupt
  
  // Set up ADC to trigger and read channel 0 on timer 1 overflow
#if USE_3V3_AREF
  ADMUX = (1 << ADLAR);                   // use AREF pin (connected to 3.3V) as voltage reference, read pin A0, left-adjust result
#else
  ADMUX = (1 << REFS0) | (1 << ADLAR);    // use Avcc as voltage reference, read pin A0, left-adjust result
#endif  
  ADCSRB = (1 << ADTS2) | (1 << ADTS1);   // auto-trigger ADC on timer/counter 1 overflow
  ADCSRA = (1 << ADEN) | (1 << ADSC) | (1 << ADATE) | (1 << ADPS2);  // enable adc, enable auto-trigger, prescaler = 16 (1MHz ADC clock)
  DIDR0 = 1;

  // Set up timer 1.
  // Prescaler = 1, phase correct PWM mode, TOP = ICR1A
  TCCR1A = (1 << COM1A1) | (1 << WGM11);
  TCCR1B = (1 << WGM12) | (1 << WGM13) | (1 << CS10);    // CTC mode, prescaler = 1
  TCCR1C = 0;
  OCR1AH = (TIMER1_TOP/2 >> 8);
  OCR1AL = (TIMER1_TOP/2 & 0xFF);
  ICR1H = (TIMER1_TOP >> 8);
  ICR1L = (TIMER1_TOP & 0xFF);
  TCNT1H = 0;
  TCNT1L = 0;
  TIFR1 = 0x07;      // clear any pending interrupt
  TIMSK1 = (1 << TOIE1);

  // Set up timer 0
  // Clock source = T0, fast PWM mode, TOP (OCR0A) = 7, PWM output on OC0B
  TCCR0A = (1 << COM0B1) | (1 << WGM01) | (1 << WGM00);
  TCCR0B = (1 << CS00) | (1 << CS01) | (1 << CS02) | (1 << WGM02);
  OCR0A = 7;
  OCR0B = 3;
  TCNT0 = 0;

  // Set up timer 2 for tone generation
  TIMSK2 = 0;
  TCCR2A = (1 << COM2B0) | (1 << COM2B1) | (1 << WGM21) | (1 << WGM20);   // set OC2B on compare match, clear OC2B at zero, fast PWM mode
  TCCR2B = (1 << WGM22) | (1 << CS22) | (1 << CS21);                      // prescaler 256, allows frequencies from 245Hz upwards
  OCR2A = 62;   // 1kHz tone
  OCR2B = 255;  // greater than OCR2A to disable the tone for now. Set OCR2B = half OCR2A to generate a tone.
  
  sei();

  while (!sampleReady) {}    // discard the first sample
  misses = 0;
  sampleReady = false;

#if DEBUG_OUTPUT
  Serial.begin(19200);
#endif
}

void tone(unsigned int freq)
{
  if (freq == 0)
  {
    OCR2B = 255;
  }
  else
  {
    const uint8_t divisor = 62500u/constrain(freq, 245u, 6000);
    OCR2A = divisor;
    OCR2B = divisor/2;
  }
}

// Timer 0 overflow interrupt. This serves 2 purposes:
// 1. It clears the timer 0 overflow flag. If we don't do this, the ADC will not see any more Timer 0 overflows and we will not get any more conversions.
// 2. It increments the tick counter, allowing is to do timekeeping. We get 62500 ticks/second.
// We now read the ADC in the timer interrupt routine instead of having a separate conversion complete interrupt.
ISR(TIMER1_OVF_vect)
{
  uint8_t ctr = TCNT0;
  uint8_t val = ADCH;    // only need to read most significant 8 bits
  if (ctr != ((lastctr + 1) & 7))
  {
    ++misses;
  }
  lastctr = ctr;
  int16_t *p = &bins[ctr & 3];
  if (ctr < 4)
  {
    int16_t temp = *p + (int16_t)val;
    *p = (temp > 15000) ? 15000 : temp;
  }
  else
  {
    int16_t temp = *p - (int16_t)val;
    *p = (temp < -15000) ? -15000 : temp;
  } 
  if (ctr == 7)
  {
    ++numSamples;
    if (numSamples == numSamplesToAverage)
    {
      numSamples = 0;
      if (!sampleReady)      // if previous sample has been consumed
      {
        memcpy((void*)averages, bins, sizeof(averages));
        sampleReady = true;
      }
      bins[0] = bins[1] = bins[2] = bins[3] = 0;
    }
  }
  ++ticks;
}

void loop()
{
  do
  {
    const uint16_t localTicks = ticks;
    if ((localTicks - lastPollTime) >= PollInterval)
    {
      lastPollTime = localTicks;
      encoder->poll();
      button->poll();
    }
  } while (!sampleReady);
  
  if (button->getNewPress())
  {
    // Calibrate button pressed. We save the current phase detector outputs and subtract them from future results.
    // This lets us use the detector if the coil is slightly off-balance.
    // It would be better to average several samples instead of taking just one.
    for (int i = 0; i < 4; ++i)
    {
      calib[i] = averages[i];
    }
    sampleReady = false;
    printCalibration = true;
  }
  else
  {
    const int sensChange = encoder->getChange();
    if (sensChange != 0)
    {
      sensitivity = constrain(sensitivity + sensChange, 1, 50);
      printSensitivity = true;
    }
  }

  if (printCalibration)
  {
    lcd->setCursor(row1, 0);
    lcd->print("Cal");
    for (int i = 0; i < 4; ++i)
    {
      lcd->write(' ');
      lcd->print(calib[i]);
    }
    lcd->clearToMargin();
    printCalibration = false;
  }

  if (printSensitivity)
  {
    lcd->setCursor(row2, 0);
    lcd->print("Sens ");
    lcd->print(sensitivity);
    lcd->clearToMargin();
    threshold = 5 * sensitivity;
    printSensitivity = false;
  }

  // Adjust the results for the calibration and divide by 200
  const double f = 1.0/200.0;
  double bin0 = (averages[0] - calib[0]) * f;
  double bin1 = (averages[1] - calib[0]) * f;
  double bin2 = (averages[2] - calib[0]) * f;
  double bin3 = (averages[3] - calib[0]) * f;
  sampleReady = false;          // we've finished reading the averages, so the ISR is free to overwrite them again

  double amp1 = sqrt((bin0 * bin0) + (bin2 * bin2));
  double amp2 = sqrt((bin1 * bin1) + (bin3 * bin3));
  double ampAverage = (amp1 + amp2) * 0.5;
  
  double phase1 = atan2(bin0, bin2) * radiansToDegrees + 45.0;
  double phase2 = atan2(bin1, bin3) * radiansToDegrees;

  if (phase1 > phase2)
  {
    double temp = phase1;
    phase1 = phase2;
    phase2 = temp;
  }
  
  // The ADC sample/hold takes place 2 clocks after the timer overflow
  double phaseAverage = ((phase1 + phase2)/2.0) - phaseAdjust;
  if (phase2 - phase1 > 180.0)
  { 
    if (phaseAverage < 0.0)
    {
      phaseAverage += 180.0;
    }
    else
    {
      phaseAverage -= 180.0;
    }
  }
      
  // Display results on LCD
  lcd->setCursor(row3, 0);
  lcd->print(amp1, 1);
  lcd->clearToMargin();
  lcd->setCursor(row3, 32);
  lcd->print(amp2, 1);
  lcd->setCursor(row3, 64);
  lcd->print((int)phase1);
  lcd->setCursor(row3, 96);
  lcd->print((int)phase2);

  lcd->setCursor(row4, 0);
  if (ampAverage >= threshold)
  {
    // When held in line with the centre of the coil:
    // - non-ferrous metals give a negative phase shift, e.g. -90deg for thick copper or aluminium, a copper olive, -30deg for thin alumimium.
    // Ferrous metals give zero phase shift or a small positive phase shift.
    // So we'll say that anything with a phase shift below -20deg is non-ferrous.
    if (phaseAverage < -20.0)
    {
      lcd->print("Non-ferrous");
    }
    else
    {
      lcd->print("Ferrous");
    }
    tone(ampAverage * 10 + 245);
  }
  else
  {
    tone(0);
  }
  lcd->clearToMargin();
  lcd->setCursor(row5, 0);
  float temp = ampAverage;
  while (temp > threshold)
  {
    lcd->write('*');
    temp -= (threshold/2);
  }
  lcd->clearToMargin();
  lcd->flush();

#if DEBUG_OUTPUT
  // For diagnostic purposes, print the individual bin counts and the 2 independently-calculated gains and phases
  Serial.print(misses);
  Serial.write(' ');
  
  if (bin0 >= 0.0) Serial.write(' ');
  Serial.print(bin0, 2);
  Serial.write(' ');
  if (bin1 >= 0.0) Serial.write(' ');
  Serial.print(bin1, 2);
  Serial.write(' ');
  if (bin2 >= 0.0) Serial.write(' ');
  Serial.print(bin2, 2);
  Serial.write(' ');
  if (bin3 >= 0.0) Serial.write(' ');
  Serial.print(bin3, 2);
  Serial.print("    ");
  Serial.print(amp1, 2);
  Serial.write(' ');
  Serial.print(amp2, 2);
  Serial.write(' ');
  if (phase1 >= 0.0) Serial.write(' ');
  Serial.print(phase1, 2);
  Serial.write(' ');
  if (phase2 >= 0.0) Serial.write(' ');
  Serial.print(phase2, 2);
  Serial.print("    ");
  
  // Print the final amplitude and phase, which we use to decide what (if anything) we have found)
  if (ampAverage >= 0.0) Serial.write(' ');
  Serial.print(ampAverage, 1);
  Serial.write(' ');
  if (phaseAverage >= 0.0) Serial.write(' ');
  Serial.print((int)phaseAverage);
  
  // Decide what we have found and tell the user
  if (ampAverage >= threshold)
  {
    // When held in line with the centre of the coil:
    // - non-ferrous metals give a negative phase shift, e.g. -90deg for thick copper or aluminium, a copper olive, -30deg for thin alumimium.
    // Ferrous metals give zero phase shift or a small positive phase shift.
    // So we'll say that anything with a phase shift below -20deg is non-ferrous.
    if (phaseAverage < -20.0)
    {
      Serial.print(" Non-ferrous");
    }
    else
    {
      Serial.print(" Ferrous");
    }
    float temp = ampAverage;
    while (temp > threshold)
    {
      Serial.write('!');
      temp -= (threshold/2);
    }
  }   
  Serial.println();
#endif
}
