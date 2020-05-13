
/* 
 * File:   PWM1.c
 * Author: Mohammed
 *
 * Created on 6 4, 2018, 11:10 PM
 * The program is about PWM for the MOSFET
 */
#include<pic18f458.h>
/* configuration bits setup */
#pragma config OSC = HS, OSCS = OFF // XTAL = 16 MHZ
#pragma config PWRT = ON, BOR = ON, BORV = 45
#pragma config WDT = OFF
#pragma config DEBUG = OFF, LVP = OFF, STVR = OFF
//preprocessor directives
#define TPERIOD (16000/(4*4*20)) -1 // for Fosc= 16MHZ, Fpwm=20 kHz, and pre-scaler of 4

void main (void)
{
    TRISCbits.TRISC2 = 0; // configure RC2 (PWM pin) as an output pin
	PR2 = TPERIOD;
	CCPR1L = 0.5 * TPERIOD; // For a duty cycle 0.5
	CCP1CON = 0x1C; // PWM mode, DC1B1 & DC1B0 values for fraction 0.5 of the value of CCPR1L register (0001,1100)
	T2CON = 0x01; // pre-scaler 4 (0000,0001)
	TMR2 = 0x00; // timer2 starts from zero
    T2CONbits.TMR2ON = 1; // timer2 on
    while(1);
}

