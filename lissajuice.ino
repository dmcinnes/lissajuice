/* LissaJuice

   Based on Tiny Function Generator PCB
   by David Johnson-Davies - www.technoblogy.com - 7th February 2019
   ATtiny85 @ 8 MHz (internal PLL; BOD disabled)

   CC BY 4.0
   Licensed under a Creative Commons Attribution 4.0 International license:
   http://creativecommons.org/licenses/by/4.0/
*/

#include <Arduino.h>
#define NOINIT __attribute__ ((section (".noinit")))

// Don't initialise these on reset
unsigned int Freq NOINIT;
int8_t Sinewave[256] NOINIT;


const unsigned int ReadingsCount = 10;
unsigned int readIndex = 0;

unsigned int Acc1FreqReadings[ReadingsCount];
unsigned int Acc1FreqTotal = 0;
unsigned int Acc2FreqReadings[ReadingsCount];
unsigned int Acc2FreqTotal = 0;
unsigned int Acc2OffsetReadings[ReadingsCount];
unsigned int Acc2OffsetTotal = 0;

// Direct Digital Synthesis **********************************************

volatile unsigned int Acc1Freq, Acc2Freq, Acc2Offset;
volatile unsigned int Acc1, Acc2, Jump;
volatile signed int X, Y;

void SetupDDS () {
  // Enable 64 MHz PLL and use as source for Timer1
  PLLCSR = 1<<PCKE | 1<<PLLE;

  // Set up Timer/Counter1 for PWM output
  TIMSK = 0;                               // Timer interrupts OFF
  TCCR1 = 1<<PWM1A | 2<<COM1A0 | 1<<CS10;  // PWM A, clear on match, 1:1 prescale
  GTCCR = 1<<PWM1B | 3<<COM1B0;            // PWM B, clear on match
  pinMode(1, OUTPUT);                      // Enable OC1A PWM output pin
  pinMode(4, OUTPUT);                      // Enable OC1B PWM output pin

  // Set up Timer/Counter0 for 20kHz interrupt to output samples.
  TCCR0A = 3<<WGM00;                       // Fast PWM
  TCCR0B = 1<<WGM02 | 2<<CS00;             // 1/8 prescale
  TIMSK = 1<<OCIE0A;                       // Enable compare match, disable overflow
  OCR0A = 60;                              // Divide by 61
}

// Calculate sine wave
void CalculateSine () {
  int X=0, Y=8180;
  for (int i=0; i<256; i++) {
    X = X + (Y*4)/163;
    Y = Y - (X*4)/163;
    Sinewave[i] = X>>6;
  }
}

void Sine () {
  Acc1 = Acc1 + Jump * Acc1Freq;
  Acc2 = Acc2 + Jump * Acc2Freq + Acc2Offset - 64;
  OCR1A = Sinewave[Acc1>>8] + 128;
  OCR1B = Sinewave[Acc2>>8] + 128;
}

ISR(TIM0_COMPA_vect) {
  Sine();
}

// Setup **********************************************

void setup() {
  pinMode(0, INPUT);
  pinMode(1, INPUT);
  pinMode(3, INPUT);
  for (unsigned int i = 0; i < ReadingsCount; i++) {
    Acc1FreqReadings[i]   = 0;
    Acc2FreqReadings[i]   = 0;
    Acc2OffsetReadings[i] = 0;
  }
  Acc1Freq = 1;
  Acc2Freq = 1;
  Acc2Offset = 0;
  Freq = 100;
  CalculateSine();
  SetupDDS();
  Jump = Freq*4;
}

// Everything done by interrupts
void loop() {
  Acc1FreqTotal   = Acc1FreqTotal   - Acc1FreqReadings[readIndex];
  Acc2FreqTotal   = Acc2FreqTotal   - Acc2FreqReadings[readIndex];
  Acc2OffsetTotal = Acc2OffsetTotal - Acc2OffsetReadings[readIndex];

  Acc1FreqReadings[readIndex] = analogRead(1) >> 6;
  Acc1FreqTotal = Acc1FreqTotal + Acc1FreqReadings[readIndex];
  Acc1Freq = Acc1FreqTotal / ReadingsCount;

  // offset as it's on the reset pin and we need to keep that above ~2.2v
  Acc2FreqReadings[readIndex] = (analogRead(0) - 675) >> 5;
  Acc2FreqTotal = Acc2FreqTotal + Acc2FreqReadings[readIndex];
  Acc2Freq = Acc2FreqTotal / ReadingsCount;

  Acc2OffsetReadings[readIndex] = analogRead(3) >> 3;
  Acc2OffsetTotal = Acc2OffsetTotal + Acc2OffsetReadings[readIndex];
  Acc2Offset = Acc2OffsetTotal / ReadingsCount;

  readIndex++;
  if (readIndex >= ReadingsCount) {
    readIndex = 0;
  }

  delay(100);
}
