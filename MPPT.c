/*
	main idea:
	=========
	* A timer is designed with a certain sampling rate (100 * 40us) to demonstrate an interrupt request for reading the current input
	* voltage and current values while the main program generate a PWM signal.
	
	description:
	============
	* The program on state is a PWM signal generation with a duty cycle (= 0.5). The TMRxIF is raised regualarly with a certain 
	* sampling rate (100 * 40us). Thus an interrupt is triggered for ADC to read the volatge of pv at channel 0. When the ADIF is
	* raised, another interrupt is triggered for ADC to read current value at channel 1 (interrupt inside interrupt). After reading the 
	* current values, the MPPT algorithm is run and changes the value of CCPR1L according to a certain step size(haven't calculated yet)
*/
#include <stdio.h>
#include <stdlib.h>
#include <pic18f458>

/* configuration bits setup */
#pragma config OSC = HS, OSCS = OFF // XTAL = 16 MHZ
#pragma config PWRT = ON, BOR = ON, BORV = 45
#pragma config WDT = OFF
#pragma config DEBUG = OFF, LVP = OFF, STVR = OFF

/* functions prototypes */
void PWM_init(void);
void ADC_init(void);
float data_update(void);
void TMR0_init(void);
void MPPT(void);

/* preprocessor directives */
#define TPERIOD (16000/(4*4*20)) -1 // for Fosc= 16MHZ, Fpwm=20 kHz, and pre-scaler of 4
#define SZ 5/1024 // ADC step size(SZ)
#define VD 10 // volatge divider(VD) gain (R1+R2 / R2), R2=2k and R1=18k
#define CS 66 // current sensitivity (CS) is 66 mv/A
#define STEP 2 // step size of MPPT

#define PIND0 PORTDbits.RD0
#define TIMER0_ON T0CONbits.TMR0ON
#define PWM_ON T2CONbits.TMR2ON
#define Adj_PWM CCPR1L 

/* Global Variables */
float voltage_now;
float voltage_last = 37; // 17 is the volatge at MPP at 25 temperature degree
float current_now;
float current_last = 8; // 4 is the current at MPP at 25 temperature degree

#pargma code run_isr = 0x0008 // place this code portion at address 0x0008 in ROM
void run_isr(void)
{
	_asm 
	GO TO TMR0_isr 
	_endasm
}
#pragma code

/* interrupt service routine of TMR0IF.
 * read the voltage and then the current
 */
#pragma interrupt AD_isr
void TMR0_isr(void)
{
	TIMER0_ON = 0; // stop timer0
	PIND0 = 0;
	voltage_now = data_update();
	if(PIR1bits.ADIF == 1)
	{
		ADCON0bits.CHS0 = 1; // select ch1 (AN1 pin), at which the current sensor is connected
		current_now = data_update();
		//PIR1bits.ADIF = 0;
		ADCON0bits.CHS0 = 0; // switch to ch0 (AN0 pin), at which the voltage sensor is connected
	}
	INTCON.TMR0IF = 0; // clear TMR0IF which is the source of interrupt
}

/* Main Program */
void main(void)
{
	TRISDbits.TRISD0 = 0; // used for debugging as an indication to the timer0 sampling rate  
	/* Intialization PWM, timer0 and ADC */
	PWM_init();
	ADC_init();
	TMR0_init();
	/* interrupt enable bits */
	INTCONbits.GIE = 1; // enable interrupt globally
	INTCONbits.TMR0IE = 1; // enable TMR0 interrupt source
	
	TIMER0_ON = 1; // start timer0
	PIND0 = 1;
	while(1)
	{
		// check for data update
		if(PIR1bits.ADIF == 1)
		{
			MPPT(); // MPPT algorithm
			PIR1bits.ADIF == 0; // clear ADIF bit
			TMR0_init(); // reload timer0 register again
			TIMER0_ON = 1; // start timer0
			PIND0 = 1;
		}
	}
}

/*
	PWM_init function to initialize the PWM mode 
	arguments: N/A
	return value: N/A
*/
void PWM_init(void)
{
	TRISCbits.TRISC2 = 0; // configure RC2 (PWM pin) as an output pin
	PR2 = TPERIOD;
	CCPR1L = 0.5 * TPERIOD; // For a duty cycle 0.5
	CCP1CON = 0x1C; // PWM mode, DC1B1 & DC1B0 values for fraction 0.5 of the value of CCPR1L (0001,1100)
	T2CON = 0x02; // pre-scaler 16 (0000,0010)
	TMR2 = 0x00; // timer2 starts from zero
	PIR1bits.TMR2IF = 0;
	PWM_ON = 1;
}

/*
	ADC_init function to initialize the ADC module registers
	arguments: N/A
	return values: N/A
*/
void ADC_init(void)
{
	TRISAbits.TRISA0 = 1; // configure bit A0 as input for voltage sensor
	TRISAbits.TRISA1 = 1; // configure bit A1 as input for current sensor
	ADCON0 = 0x81; // ADC is on, AN0 channel and Fosc/32 clock source (1000,0001)
	ADCON1 = 0x84; // right justification, Fosc/32 clock source, 3 analog channels AN0, AN1, AN3 with Vref+ is Vdd and Vref- is Vss (1000,0100)
}

/*
	data_update function to starts A/D conversion
	arguments: N/A
	returns: float value, which is the actual PV voltage (or current) value
*/

float data_update(void)
{
	unsigned int V; // used for masking handeling
	float R;
	//__delay_us(15); // delay 15 us for acquisition time
	ADCON0bits.GO = 1; // start A/D conversion. Conversion time is 24 usec
	while(ADCON0bits.DONE == 1); // wait till the conversion ends, at which ADIF set to one
	V = ADRESH;
	V = ((V<<8) | ADRESL);
	if (ADCON0bits.CHS0 == 0)
	{
		R = (float) V * SZ * VD; // get the actual PV voltage value
	}
	else
	{
		R = (float) (R * SZ / CS); // get the actual PV current value
	}
	return R;
}
/*
	TMR0_init function to initialize timer0 operation through T0CON register and reload TMR0 register for specific time delay
	arguments: N/A
	returns: N/A
*/
void TMR0_init(void)
{
	T0CON = 0b00001000; // timer0 off, 16 bit timer mode, increment on +ve edge transition(from L to H), no prescaler
	/* the number to be loaded in TMR0 register (in TMR0L & TMR0H registers) 
		= 2^16 - (100*40us (the required delay) / 0.25us) = 49536 = 0xC180
	*/
	TMR0H = 0xC1;
	TMR0L = 0x80;
	INTCONbits.TMR0IF = 0; // clear TMR0 interrupt flag
}
/*
	MPPT function is used to track the maximum power point using incremental conductance approach
	arguments: N/A
	returns: N/A
*/
void MPPT(void)
{
	float dV = voltage_now - voltage_last;
	float dI = current_now - current_last;
	float power_now = voltage_now * current_now;
	float power_last = voltage_last * current_last;
	float dP = power_now - power_last;
	if(dV == 0)
	{
		if(dI == 0)
		{
			return;
		}
		else if(dI > 0)
		{
			Adj_PWM = Adj_PWM - STEP; // increase the operating voltage
			return;
		}
		else
		{
			Adj_PWM = Adj_PWM + STEP; // decrease the operating voltage
			return;
		}
	}
	else
	{
		if(dP/dV == 0)
		{
			return;
		}
		else if(dP/dV > 0)
		{
			Adj_PWM = Adj_PWM - STEP; // increase the operating voltage
			return;
		}
		else
		{
			Adj_PWM = Adj_PWM + STEP; // decrease the operating voltage
			return;
		}
	}
	voltage_last = voltage_now;
	current_last = current_now;
}