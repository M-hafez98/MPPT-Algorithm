/* 
 * File:   tracking.c
 * Author: MO
 *
 * Created on 20, 2018, 05:28 ?
 */

/* changing the loading operating point through manual duty cycle changing and measuring the 
 * power of the operating point */

#include<pic18f458.h>

/* configuration bits setup */
#pragma config OSC = HS, OSCS = OFF // XTAL = 16 MHZ
#pragma config PWRT = ON, BOR = ON, BORV = 45
#pragma config WDT = OFF
#pragma config DEBUG = ON, LVP = OFF, STVR = OFF

/*preprocessor directives*/
#define TPERIOD (16000/(4*4*20)) -1 // for Fosc= 16MHZ, Fpwm=20 kHz, and pre-scaler of 4
#define SZ 5.0/1024.0 // ADC step size(SZ)
#define VD 10 // volatge divider(VD) gain (R1+R2 / R2), R2=2k and R1=18k
#define CS 0.0769 // current sensitivity (CS) 76.9 mv/A
#define ACSOFFSET 2.524414

/* functions prototypes */
void delay_15us(void);
void delay_us(unsigned int);
void vary_D(void);
void read_current(void);
void read_voltage(void);

/* global variables */
unsigned int D_mask;
unsigned int voltage_mask;
unsigned int current_mask;
float PV_voltage;
float PV_current;
float PV_power;
float CV; // zero current voltage reading
float V;
float D;
void main (void)
{
	/* pins and registers configuration */
	TRISAbits.TRISA0 = 1; // configure RA0 as input pin for current sensor
    TRISAbits.TRISA1 = 1; // configure RA1 as input pin for voltage sensor
	TRISAbits.TRISA2 = 1; // configure pin RA2 as input for the potentiometer
	TRISCbits.TRISC2 = 0; // configure RC2 (PWM pin) as an output pin
	ADCON0 = 0x91; //  ADC ON, select channel 2 with conversion time FOSC/32 (1001,0001)
    ADCON1 = 0x84; // right justification, 3 analog channels AN0, AN1, AN3 are selected (1000,0100)
	CCP1CON = 0x1C; // PWM mode, DC1B1 & DC1B0 values for fraction 0.5 of the value of CCPR1L register (0001,1100)
	T2CON = 0x01; // timer off, pre-scaler 4 (0000,0001)
	PR2 = TPERIOD;
	while(1)
	{
		vary_D();
		delay_us(4000); // wait for 1 ms
		ADCON0 = 0x81; // switch to channel 0 for current sensor reading(1000,0001)
		read_current();	
		ADCON0 = 0x89; // switch to channel 1 for voltage sensor reading(1000,1001)
		read_voltage();
		PV_power = PV_voltage * PV_current;
		ADCON0 = 0x91; // switch to channel 2 for potentiometer reading
		delay_us(4000); // wait for 1 ms
	}
}

void delay_15us(void)
{
    T0CON = 0b00001000; // timer0 off, 16 bit timer mode, increment on +ve edge transition(from L to H), no prescaler
	/* the number to be loaded in TMR0 register (in TMR0L & TMR0H registers) 
		= 2^16 - (15us (the required delay) / 0.25us) = 65476 = 0xFFC4
	*/
	TMR0H = 0xFF;
	TMR0L = 0xC4;
	INTCONbits.TMR0IF = 0; // clear TMR0 interrupt flag
    T0CONbits.TMR0ON = 1; // start timer0
    while(INTCONbits.TMR0IF == 0);
    T0CONbits.TMR0ON = 0; // stop timer0
    INTCONbits.TMR0IF = 0; // clear interrupt flag
}
void delay_us(unsigned int c)
{
    unsigned int i;
    for (i=0; i<c; i++);
}
void vary_D(void)
{
	delay_15us(); // acquisition time 15 us
	ADCON0bits.GO = 1; // start A/D conversion. Conversion time is 48 usec
	while(ADCON0bits.DONE == 1); // wait till the conversion ends
	D_mask = ADRESH;
	D_mask = (D_mask<<8) | ADRESL;
    V = D_mask * SZ; // voltage of potentiometer
	D = V / 5.0;
	if (D <= 0.3)
	{
        D = 0.3;
		CCPR1L = D * TPERIOD;
	}
	else if (D >= 0.8)
	{
        D = 0.8;
		CCPR1L = D * TPERIOD;
	}
	else
	{
		CCPR1L = D * TPERIOD;
	}
	TMR2 = 0x00; // timer2 starts from zero
	T2CONbits.TMR2ON = 1; // timer2 on
}
void read_current(void)
{
	delay_15us(); // wait for acquisition process
    ADCON0bits.GO = 1; // start the conversion process
    while(ADCON0bits.DONE == 1); // wait till the DONE bit cleared identifying that the conversion is done
    current_mask = ADRESH;
    current_mask = (current_mask << 8) | ADRESL;
	CV = (float) ( ( current_mask / 1024.0 ) * 5.0 );
    PV_current = (float) ( ( current_mask / 1024.0 ) * 5.0 ); // voltage reading of zero current of sensor        
    PV_current = PV_current - ACSOFFSET;
    PV_current = PV_current / CS;
}
void read_voltage(void)
{
	delay_15us(); // wait for acquisition process
    ADCON0bits.GO = 1; // start the conversion process
    while(ADCON0bits.DONE == 1); // wait till the DONE bit cleared identifying that the conversion is done
    voltage_mask = ADRESH;
    voltage_mask = (voltage_mask << 8) | ADRESL; // digital output
    PV_voltage = (float) voltage_mask * SZ * VD; // exact input voltage
}