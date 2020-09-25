//------------------------------------------- main.c CODE STARTS ---------------------------------------------------------------------------
// Assignment 2 - Distance Measurement System - Origin: Ngo Quang Trung code - Ref: Trung's github
#include <stdio.h>
#include "NUC100Series.h"
#include "LCD.h"

#define TIMER0_COUNTS 100000
#define TIMER1_COUNTS 40000
#define TIMER2_COUNTS_3 200000
#define TIMER2_COUNTS_2	150000
#define TIMER2_COUNTS_1 100000
#define TIMER2_COUNTS_0 50000

typedef enum {FAR, MEDIUM, CLOSE, VERY_CLOSE, UNDEFINED} STATES;

//Configurations
void System_Config(void);
void SPI3_Config(void);
void TIMER0_Config(void);
void TIMER1_Config(void);
void TIMER2_Config(void);

//Interrupt handlers
void EINT0_IRQHandler(void);
void TMR2_IRQHandler(void);

//LCD
void LCD_start(void);
void LCD_command(unsigned char temp);
void LCD_data(unsigned char temp);
void LCD_clear(void);
void LCD_SetAddress(uint8_t PageAddr, uint8_t ColumnAddr);

//Operations
void Trigger(void);
uint32_t Calculate_distance(void);
void Print_result(uint32_t distance);
void Set_warning_freq(STATES state);
STATES Get_state(uint32_t distance);

int main(void)
{
	uint32_t distance;
	STATES temp_state, next_state;
	
	System_Config();
	SPI3_Config();
	TIMER0_Config();
	TIMER1_Config();
	TIMER2_Config();
	LCD_start();
	LCD_clear();
	//--------------------------------
	//LCD static content
	//--------------------------------
	printS_5x7(2, 10, "Distance: ");
	temp_state = UNDEFINED;
	while(1)
	{
		Trigger();
		distance = Calculate_distance();
		Print_result(distance);
		CLK_SysTickDelay(50000);
		next_state = Get_state(distance);
		if (next_state != temp_state) {
			temp_state = next_state;
			Set_warning_freq(temp_state);
		}
	}
}

//------------------------------------------------------------------------------------------------------------------------------------
// Functions definition
//------------------------------------------------------------------------------------------------------------------------------------
//Interrupt Service Rountine of GPIO port B pin 14
void EINT0_IRQHandler(void) 
{
	TIMER1->TCSR |= (0x01ul << 26); // reset the counter again after every time the interrupt is triggered

	//start counting
	TIMER1->TCSR |= 1ul << 30;

	while(PB->PIN & (1 << 14)); //wait till GPB.14 is back to 0, while GPB.14 is still 1, do nothing in while loop

	//stop counting
	TIMER1->TCSR &= ~(1ul << 30); // stop counter
	//TDR will not be updated continuously while timer counter is counting
	TIMER1->TCSR &= ~(0x01ul << 16); // stop update new value to TDR
    
	PB->ISRC |= (1ul << 14); // clear pending to GB.14 Interrrupt
}

void TMR2_IRQHandler(void)
{
	TIMER2->TISR |= (0x01ul << 0);
	PC->DOUT ^= (0x01ul << 12);
	PB->DOUT ^= (0x01ul << 11);
}
void LCD_start(void)
{
	LCD_command(0xE2); // Set system reset
	LCD_command(0xA1); // Set Frame rate 100 fps
	LCD_command(0xEB); // Set LCD bias ratio E8~EB for 6~9 (min~max)
	LCD_command(0x81); // Set V BIAS potentiometer
	LCD_command(0xA0); // Set V BIAS potentiometer: A0 ()
	LCD_command(0xC0);
	LCD_command(0xAF); // Set Display Enable
}
void LCD_command(unsigned char temp)
{
	SPI3->SSR |= 1ul << 0;
	SPI3->TX[0] = temp;
	SPI3->CNTRL |= 1ul << 0;
	while(SPI3->CNTRL & (1ul << 0));
	SPI3->SSR &= ~(1ul << 0);
}
void LCD_data(unsigned char temp)
{
	SPI3->SSR |= 1ul << 0;
	SPI3->TX[0] = 0x0100+temp;
	SPI3->CNTRL |= 1ul << 0;
	while(SPI3->CNTRL & (1ul << 0));
	SPI3->SSR &= ~(1ul << 0);
}
void LCD_clear(void)
{
	int16_t i;
	LCD_SetAddress(0x0, 0x0);
	for (i = 0; i < 132 *8; i++)
	{
		LCD_data(0x00);
	}
}
void LCD_SetAddress(uint8_t PageAddr, uint8_t ColumnAddr)
{
LCD_command(0xB0 | PageAddr);
LCD_command(0x10 | ((ColumnAddr>>4)&0xF));
LCD_command(0x00 | (ColumnAddr & 0xF));
}

void System_Config (void)
{
	SYS_UnlockReg(); // Unlock protected registers
	//System initialization start-------------------
	//enable clock sources
	CLK->PWRCON |= (1 << 0);
	while(!(CLK->CLKSTATUS & (1 << 0)));
	//Select CPU clock
	CLK->CLKSEL0 &= (~(0x07ul << 0));
	//CPU clock frequency divider
	CLK->CLKDIV &= (~(0x0Ful));
	//System initialization end---------------------

	// SPI3 clock enable
	CLK->APBCLK |= 1ul << 15;

	//TM0 Clock selection and configuration
	CLK->CLKSEL1 &= ~(0x07ul << 8);
	CLK->CLKSEL1 |= (0x02ul << 8);
	CLK->APBCLK |= (0x01ul << 2);

	//TM1 Clock selection and configuration
	CLK->CLKSEL1 &= ~(0x07ul << 12); 
	CLK->CLKSEL1 |= (0x02ul << 12);
	CLK->APBCLK |= (0x01ul << 3);

	//TM2 Clock selection configuration
	CLK->CLKSEL1 &= ~(0x07ul << 16); 
	CLK->CLKSEL1 |= (0x02ul << 16);
	CLK->APBCLK |= (0x01ul << 4);

	//GPIO initialization start --------------------
	//GPIOA.5: output push-pull
	PA->PMD &= ~(0x03ul << 10);
	PA->PMD |= (0x01ul << 10);
	PA->DOUT &= ~(0x01ul << 5);
	
	//GPIOC.12 LED: output push-pull
	PC->PMD &= ~(0x03ul << 24);
	PC->PMD |= (0x01ul << 24);
	PC->DOUT |= (0x01ul << 12);
	
	//GPIOB.11 Buzzer: output push-pull
	PB->PMD &= ~(0x03ul << 22);
	PB->PMD |= (0x01ul << 22);
	PB->DOUT |= (0x01ul << 11);
	
	//GPIO initialization end ----------------------
    
	//GPIO Interrupt configuration. GPIO-B14 is the interrupt source
	PB->PMD &= (~(0x03ul << 28)); // input
	PB->IMD &= (~(1ul << 14)); // edge trigger
	PB->IEN |= (1ul << 30); // rising edge

	//NVIC interrupt configuration for GPIO-B14 interrupt source
	NVIC->ISER[0] |= (1ul << 2);
	NVIC->IP[0] &= (~(3ul << 22));

	//TM2 interrupt configuration
	TIMER2->TCSR |= (0x01ul << 29);
	
	//NVIC interrupt configuration for GPIO-B14
	NVIC->ISER[0] |= (1ul << 10);
	NVIC->IP[2] &= (~(3ul << 22));

	SYS_LockReg(); // Lock protected registers
}

void SPI3_Config (void)
{
	SYS->GPD_MFP |= 1ul << 11; //1: PD11 is configured for SPI3
	SYS->GPD_MFP |= 1ul << 9; //1: PD9 is configured for SPI3
	SYS->GPD_MFP |= 1ul << 8; //1: PD8 is configured for SPI3
	SPI3->CNTRL &= ~(1ul << 23); //0: disable variable clock feature
	SPI3->CNTRL &= ~(1ul << 22); //0: disable two bits transfer mode
	SPI3->CNTRL &= ~(1ul << 18); //0: select Master mode
	SPI3->CNTRL &= ~(1ul << 17); //0: disable SPI interrupt
	SPI3->CNTRL |= 1ul << 11; //1: SPI clock idle high
	SPI3->CNTRL &= ~(1ul << 10); //0: MSB is sent first
	SPI3->CNTRL &= ~(3ul << 8); //00: one transmit/receive word will be executed in one data transfer
	SPI3->CNTRL &= ~(31ul << 3); //Transmit/Receive bit length
	SPI3->CNTRL |= 9ul << 3; //9: 9 bits transmitted/received per data transfer
	SPI3->CNTRL |= (1ul << 2); //1: Transmit at negative edge of SPI CLK
	SPI3->DIVIDER = 0; // SPI clock divider. SPI clock = HCLK / ((DIVIDER+1)*2). HCLK = 12 MHz
}

void TIMER0_Config (void)
{
	//Timer 0 initialization start--------------
	TIMER0->TCSR &= ~(0xFFul << 0);
	TIMER0->TCSR |= (11ul << 0);
	//set PRESCALE = 11

	//reset Timer 0
	TIMER0->TCSR |= (0x01ul << 26);

	//define Timer 0 operation mode
	TIMER0->TCSR &= ~(0x03ul << 27);
	TIMER0->TCSR |= (0x01ul << 27);
	//set periodic mode 
	TIMER0->TCSR &= ~(0x01ul << 24);

	//TDR to be updated continuously while timer counter is counting
	TIMER0->TCSR |= (0x01ul << 16);

	//TimeOut = 100ms --> Counter's TCMPR = 100ms / (1/(1M Hz) = 100000
	TIMER0->TCMPR = TIMER0_COUNTS - 1;

	//start counting
	TIMER0->TCSR |= (0x01ul << 30);

	//Timer 0 initialization end----------------
}

void TIMER1_Config (void)
{
	//Timer 1 initialization start--------------

	// set PRESCALE = 12
	TIMER1->TCSR &= ~(0xFFul << 0);
	TIMER1->TCSR |= (11ul << 0);

	//reset Timer 1
	TIMER1->TCSR |= (0x01ul << 26);

	//define Timer 1 operation mode: one-shot
	TIMER1->TCSR &= ~(0x03ul << 27);
	TIMER1->TCSR |= (0x00ul << 27);
	TIMER1->TCSR &= ~(0x01ul << 24);

	//TDR to be updated continuously while timer counter is counting
	TIMER1->TCSR |= (0x01ul << 16);

	//TimeOut = 40 ms (25 Hz) --> Counter's TCMPR = 40 ms / (1/(1 MHz) = 40000
	TIMER1->TCMPR = TIMER1_COUNTS - 1;

	//Timer 1 initialization end----------------
}

void TIMER2_Config(void)
{
	//Timer 2 initialization start--------------
	// set PRESCALE = 12
	TIMER2->TCSR &= ~(0xFFul << 0);
	TIMER2->TCSR |= (11ul << 0);
	
	//reset Timer 2
	TIMER2->TCSR |= (0x01ul << 26);
	
	//define Timer 2 operation mode
	TIMER2->TCSR &= ~(0x03ul << 27);
	TIMER2->TCSR |= (0x01ul << 27); //periodic
	TIMER2->TCSR &= ~(0x01ul << 24); 
	
	//TDR to be updated continuously while timer counter is counting
	TIMER2->TCSR |= (0x01ul << 16);
	
	
}
void Trigger(void)
{
	while(!(TIMER0->TDR == TIMER0_COUNTS - 1));
	PA->DOUT = (1 << 5);
	CLK_SysTickDelay(2);
	PA->DOUT = (0 << 5);
	CLK_SysTickDelay(2000);
}	

uint32_t Calculate_distance(void)
{
	uint32_t TDR_val;
	while(TIMER1->TCSR & (1ul << 30)); // wait until counter TIMER1 is stopping 
	TDR_val = TIMER1->TDR & 0x00FFFFFF;
	return TDR_val / (2.0 * 29.412);
}
void Print_result(uint32_t distance)
{
	char TDR_val_s[4]="0000";
	printS_5x7(4+5*10, 8, "     ");
	sprintf(TDR_val_s, "%d", distance);
  printS_5x7(4+5*10, 8, TDR_val_s);
	printS_5x7(4+7*10, 8, "cm");
}

void Set_warning_freq(STATES state)
{
	TIMER2->TCSR &= ~(0x01ul << 30); // stop counting
	TIMER2->TCSR |= (0x01ul << 26); // reset
	switch (state){
		case VERY_CLOSE: 
			TIMER2->TCMPR = TIMER2_COUNTS_0 - 1;
			break;
		case CLOSE:
			TIMER2->TCMPR = TIMER2_COUNTS_1 - 1;
			break;
		case MEDIUM:
			TIMER2->TCMPR = TIMER2_COUNTS_2 - 1;
			break;
		case FAR:
			TIMER2->TCMPR = TIMER2_COUNTS_3 - 1;
			break;
		case UNDEFINED:
			return;
	}
	TIMER2->TCSR |= (0x01ul << 30);
}


STATES Get_state(uint32_t distance){
	if (distance < 2) return UNDEFINED;
	if (distance > 2 && distance < 10) return VERY_CLOSE;
	if (distance >= 10 && distance < 25) return CLOSE;
	if (distance >= 25 && distance < 40) return MEDIUM;
	return FAR;
}

//------------------------------------------- main.c CODE ENDS ---------------------------------------------------------------------------