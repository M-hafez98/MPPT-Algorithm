/* 
 * File:   var_D.c
 * Author: Mohammed
 *
 * Created on 13 6, 2018, 12:17 ?
 */

/*
 * The program is about generating square wave PWM-based for MOSFET with variable duty cycle using potentiomter
 */
#include<pic18f458.h>
/* configuration bits setup */
#pragma config OSC = HS, OSCS = OFF // XTAL = 16 MHZ
#pragma config PWRT = ON, BOR = ON, BORV = 45
#pragma config WDT = OFF
#pragma config DEBUG = ON, LVP = OFF, STVR = OFF // DEBUG bit is ON
/*preprocessor directives*/
#define TPERIOD (16000/(4*4*20)) -1 // for Fosc= 16MHZ, Fpwm=20 kHz, and pre-scaler of 4
#define SZ 5.0/1024.0 // ADC step size(SZ)

void delay_us(unsigned int);

void main (void)
{
	unsigned int D_mask;
	float V;
	float D;
	/* pin configuration */
    TRISCbits.TRISC2 = 0; // configure RC2 (PWM pin) as an output pin
	TRISAbits.TRISA2 = 1; // configure pin RA2 as input for the potentiometer
	ADCON0 = 0x91; // ADC is on, AN2 channel and Fosc/32 clock source (1001,0001)
	ADCON1 = 0x8E; // right justification, Fosc/32 clock source, AN0 with Vref+ is Vdd and Vref- is Vss (1000,1110)
	CCP1CON = 0x1C; // PWM mode, DC1B1 & DC1B0 values for fraction 0.5 of the value of CCPR1L register (0001,1100)
	T2CON = 0x01; // timer off, pre-scaler 4 (0000,0001)
	PR2 = TPERIOD;
	while(1)
	{
		delay_us(60); // acquisition time
		ADCON0bits.GO = 1; // start A/D conversion. Conversion time is 48 usec
		while(ADCON0bits.DONE == 1); // wait till the conversion ends
		D_mask = ADRESH;
		D_mask = (D_mask<<8) | ADRESL; // digital output
        V = D_mask * SZ; // voltage of potentiometer
		D = V / 5.0;
		if (D <= 0.3)
		{
			CCPR1L = 0.3 * TPERIOD;
		}
		else if (D >= 0.8)
		{
			CCPR1L = 0.8 * TPERIOD;
		}
		else
		{
			CCPR1L = D * TPERIOD;
		}
		TMR2 = 0x00; // timer2 starts from zero
		T2CONbits.TMR2ON = 1; // timer2 on
        delay_us(4000); // wait for the second change
	}
}
void delay_us(unsigned int c)
{
    unsigned int i;
    for (i=0; i<c; i++);
}