/* 
 * File:   ICD_Sensors.c
 * Author: Mohammed
 *
 * Created on 19, 2018, 09:55 ?
 */

#include<pic18f458.h>

/* configuration bits setup */
#pragma config OSC = HS, OSCS = OFF // XTAL = 16 MHZ
#pragma config PWRT = ON, BOR = ON, BORV = 45
#pragma config WDT = OFF
#pragma config DEBUG = ON, LVP = OFF, STVR = OFF


#define SZ 5.0/1024.0 // ADC step size(SZ)
#define VD 10 // volatge divider(VD) gain (R1+R2 / R2), R2=2k and R1=18k
#define CS 0.0769 // current sensitivity (CS) is 60 mv/A
#define ACSOFFSET 2.5097656

void delay_15us(void);
void delay_us(unsigned int);

void main (void)
{
    unsigned int voltage_mask;
    unsigned int current_mask;
    float PV_voltage;
    float PV_current;
    float PV_power;
	TRISAbits.TRISA0 = 1; // configure RA0 as input pin for current sensor
    TRISAbits.TRISA1 = 1; // configure RA1 as input pin for voltage sensor
    ADCON0 = 0b10001001; //  ADC ON, select channel 1 with conversion time FOSC/32
    ADCON1 = 0b10000100; // right justification, ADCS2 is 0 for conversion time FOSC/32, 3 analog channels AN0, AN1, AN3 are selected
    while(1)
    {
        delay_15us(); // wait for acquisition process
        ADCON0bits.GO = 1; // start the conversion process
        while(ADCON0bits.DONE == 1); // wait till the DONE bit cleared identifying that the conversion is done
        // read the digital output
        voltage_mask = ADRESH;
        voltage_mask = (voltage_mask << 8) | ADRESL; // digital output
        PV_voltage = (float) voltage_mask * SZ * VD; // exact input voltage
        ADCON0bits.CHS0 = 0; // switch to channel 0 for current sensor reading
        delay_15us();
        delay_15us();
        ADCON0bits.GO = 1; // start the conversion process
        while(ADCON0bits.DONE == 1); // wait till the DONE bit cleared identifying that the conversion is done
        // read the digital output
        current_mask = ADRESH;
        current_mask = (current_mask << 8) | ADRESL;
        PV_current = (float) ( current_mask / 1024.0 ) * 5.0; // voltage reading of zero current of sensor
                
        PV_current = PV_current - ACSOFFSET;
        PV_current = PV_current / CS;
        PV_power = PV_voltage * PV_current;
        
		ADCON0bits.CHS0 = 1; // switch to channel 1 for voltage sensor reading
		//delay_us(4000); // wait for 1 ms
    }
}
/*delay function*/
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
