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
  Acc = Acc + Jump;
  OCR0A = Sinewave[Acc>>8] + 128;
  OCR0B = Sinewave[Acc>>8] + 128;
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

ISR(TIM1_COMPA_vect) {
  Wavefun();
}

// Setup **********************************************

void setup() {
  // Is it a power-on reset?
  Wave = 0; Freq = 100;
  CalculateSine();
  Wavefun = Waves[Wave];
  SetupDDS();
  Jump = Freq*4;
}

// Everything done by interrupts
void loop() {
}
