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

volatile unsigned int Acc, Jump;
volatile signed int X, Y;

void SetupDDS () {
  // Enable 64 MHz PLL and use as source for Timer1
  PLLCSR = 1<<PCKE | 1<<PLLE;

  // Set up Timer/Counter1 for PWM output
  TIMSK = 0;                               // Timer interrupts OFF
  TCCR1 = 1<<PWM1A | 2<<COM1A0 | 1<<CS10;  // PWM A, clear on match, 1:1 prescale
  GTCCR = 1<<PWM1B | 3<<COM1B0;

  pinMode(1, OUTPUT);                      // Enable PWM output pin
  pinMode(4, OUTPUT);                      // Enable PWM output pin

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
  Acc = Acc + Jump;
  OCR1A = Sinewave[Acc>>8] + 128;
  OCR1B = Sinewave[Acc>>8] + 128;
}

void Sawtooth () {
  Acc = Acc + Jump;
  OCR1A = Acc >> 8;
}

void Square () {
  Acc = Acc + Jump;
  int8_t temp = Acc>>8;
  OCR1A = temp>>7;
}

void Rectangle () {
  Acc = Acc + Jump;
  int8_t temp = Acc>>8;
  temp = temp & temp<<1;
  OCR1A = temp>>7;
}

void Triangle () {
  int8_t temp, mask;
  Acc = Acc + Jump;
  temp = Acc>>8;
  mask = temp>>7;
  temp = temp ^ mask;
  OCR1A = temp<<1;
}

void Chainsaw () {
  int8_t temp, mask, top;
  Acc = Acc + Jump;
  temp = Acc>>8;
  mask = temp>>7;
  top = temp & 0x80;
  temp = (temp ^ mask) | top;
  OCR1A = temp;
}

void Pulse () {
  Acc = Acc + Jump;
  int8_t temp = Acc>>8;
  temp = temp & temp<<1 & temp<<2 & temp<<3;
  OCR1A = temp>>7;
}

void Noise () {
  int8_t temp = Acc & 1;
  Acc = Acc >> 1;
  if (temp == 0) Acc = Acc ^ 0xB400;
  OCR1A = Acc;
}

const int nWaves = 8;
wavefun_t Waves[nWaves] = {Sine, Triangle, Sawtooth, Square, Rectangle, Pulse, Chainsaw, Noise};
wavefun_t Wavefun;

ISR(TIM0_COMPA_vect) {
  Wavefun();
}

// Setup **********************************************

void setup() {
  // Is it a power-on reset?
  Wave = 0; Freq = 100;
  CalculateSine();
  Wavefun = Waves[Wave];
  MCUSR = 0;
  SetupDDS();
  Jump = Freq*4;
}

// Everything done by interrupts
void loop() {
}
