/* Tiny Function Generator PCB

   David Johnson-Davies - www.technoblogy.com - 7th February 2019
   ATtiny85 @ 8 MHz (internal PLL; BOD disabled)

   CC BY 4.0
   Licensed under a Creative Commons Attribution 4.0 International license:
   http://creativecommons.org/licenses/by/4.0/
*/

#include <Arduino.h>
#define NOINIT __attribute__ ((section (".noinit")))

// Don't initialise these on reset
int Wave NOINIT;
unsigned int Freq NOINIT;
int8_t Sinewave[256] NOINIT;

typedef void (*wavefun_t)();

// Direct Digital Synthesis **********************************************

volatile unsigned int Acc1, Acc2, Jump;
volatile signed int X, Y;

void SetupDDS () {
  TIMSK = 0;                               // Timer interrupts OFF

  // Set up Timer/Counter0 for PWM output
  TCCR0A = 3<<COM0A0 | 3<<COM0B0 | 3<<WGM00; // Clear on match A&B, Fast PWM, 1:1 prescale
  TCCR0B = 0<<WGM02 | 1<<CS00;

  pinMode(0, OUTPUT);                      // Enable OC0A PWM output pin
  pinMode(1, OUTPUT);                      // Enable OC0B PWM output pin

  // Set up Timer/Counter1 for 20kHz interrupt to output samples.
  TCCR1 = 1<<PWM1A | 4<<CS10;  // PWM A, 1/8 prescale

  TIMSK = 1<<OCIE1A;                       // Enable compare match, disable overflow
  OCR1A = 60;                              // Divide by 61
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
  Acc1 = Acc1 + Jump;
  Acc2 = Acc2 + 3 * Jump + 2;
  OCR0A = Sinewave[Acc1>>8] + 128;
  OCR0B = Sinewave[Acc2>>8] + 128;
}

ISR(TIM1_COMPA_vect) {
  Sine();
}

// Setup **********************************************

void setup() {
  Freq = 100;
  CalculateSine();
  SetupDDS();
  Jump = Freq*4;
}

// Everything done by interrupts
void loop() {
}
