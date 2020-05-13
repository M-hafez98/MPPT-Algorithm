/*
	Date: 11/2/2018
	Auother: MH
	
	PWM module consists of two functions: "PWM_init", which makes initilaization of PWM and "PWM_on" which starts the PWM signal. 
*/

#define TPERIOD (16000/(4*4*20)) -1 // for Fosc= 16MHZ, Fpwm=20 kHz, and pre-scaler of 4
/*
	PWM_init function to initialize the PWM mode 
	arguments: N/A
	return value: N/A
*/
void PWM_init(void)
{
	TRISCbits.TRISC2 = 0; // configure RC2 (PWM pin) as an output pin
	PR2 = TPERIOD; // The periodic time value at 16 MHz crystal, period frequency 25 KHz, and pre-scaler 16
	CCPR1L = 0.5 * TPERIOD; // For a duty cycle 0.5
	CCP1CON = 0x1C; // PWM mode, DC1B1 & DC1B0 values for fraction 0.5 of the value of CCPR1L register (0001,1100)
	T2CON = 0x01; // pre-scaler 4 (0000,0001)
	TMR2 = 0x00; // timer2 starts from zero
	PIR1bits.TMR2IF = 0;
}

/*
	PWM_on function to starts the PWM signal
	arguments: N/A
	return value: N/A
*/
void PWM_on(void)
{
	T2CONbits.TMR2ON = 1; // starts timer2
}