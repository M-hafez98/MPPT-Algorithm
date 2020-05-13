/*
	ADC module has two functions: "ADC_init" and "ADC_on"
*/

/*
	ADC_init function to initialize the ADC module registers
	arguments: N/A
	return values: N/A
*/
void ADC_init(void)
{
	TRISAbits.TRISA0 = 1; // configure bits A0 as input
	ADCON0 = 0x80; // AN0 channel and Fosc/64 clock source (1000,0000)
	ADCON1 = 0xCE; // right justified, Fosc/64 clock source, and AN0 analog input with Vref+ is Vdd and Vref- is Vss (1100,1110)
					// step size is 4.8 mv
}

/*
	ADC_on function to starts A/D conversion
	arguments: N/A
	returns: unsigned integer value
*/

unsigned int ADC_on(void)
{
	unsigned int V;
	ADCON0bits.ADON = 1; // enable ADC
	__delay_us(15); // delay 15 us
	ADCON0bits.GO = 1; // start A/D conversion. Conversion time is 48 usec
	while(ADCON0bits.DONE == 1); // wait till the conversion ends
	V = ADRESH;
	V = (V<<8) | ADRESL;
	return V;
}