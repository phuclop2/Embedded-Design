//------------------------------------------- main.c CODE STARTS ---------------------------------------------------------------------------
#include <stdio.h>
#include "NUC100Series.h"
#include "LCD.h"

#define BUZZER_BEEP_TIME 3
#define BUZZER_BEEP_DELAY 40000

#define TIMER1_COUNTS 500000
#define TIMER2_COUNTS 400


void System_Config(void);
void Timer0_Config(void);
void Timer1_Config(void);
void Timer2_Config(void);
void GPIO_Config(void);
void SPI3_Config(void);
void LCD_start(void);
void LCD_command(unsigned char temp);
void LCD_data(unsigned char temp);
void LCD_clear(void);
void LCD_SetAddress(uint8_t PageAddr, uint8_t ColumnAddr);

unsigned int counter = 0;

int main(void)
{
char str[2] = "00";

System_Config();
Timer0_Config();
Timer1_Config();
Timer2_Config();
GPIO_Config();
SPI3_Config();

//--------------------------------
//LCD initialization
//--------------------------------
LCD_start();
LCD_clear();
//LCD static content
//--------------------------------
printS_5x7(2, 0, "EEET2481 Final Test");
printS_5x7(2, 40, "Count value:");
//--------------------------------
//LCD dynamic content
//--------------------------------
while(1){

    if ((TIMER0->TEXISR & (1ul)) == (1ul)){
        counter = counter + 1;
        TIMER0->TEXISR |= (0x01ul << 0); // clear this bit to set the interrupt again
    }

    if (counter == 3) {
        while(!(TIMER1->TDR == TIMER1_COUNTS - 1));
        PC->DOUT ^= 1 << 12; // LED5 toogles every 0.5s
    }

    if (counter == 5){
        while(!(TIMER2->TDR == TIMER2_COUNTS - 1));
        PB->DOUT ^= 1 << 11; //BUZZER at frequency of 2.5kHz
    }

    if (counter == 8){
        PC->DOUT |= (0x01ul << 12); //off LED5
        PB->DOUT |= (0x01ul << 11); //off BUZZER
    }

sprintf(str, "%d", counter);
printS_5x7(4+5*15, 40, "  ");
printS_5x7(4+5*15, 40, str);
CLK_SysTickDelay(2000000);
}
}

//------------------------------------------------------------------------------------------------------------------------------------
// Functions definition
//------------------------------------------------------------------------------------------------------------------------------------


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


void LCD_command(unsigned char temp){
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


void LCD_clear(void) {
int16_t i;
LCD_SetAddress(0x0, 0x0);
for (i = 0; i < 132 *8; i++) {
LCD_data(0x00);
}
}


void LCD_SetAddress(uint8_t PageAddr, uint8_t ColumnAddr) {
LCD_command(0xB0 | PageAddr);
LCD_command(0x10 | ((ColumnAddr>>4)&0xF));
LCD_command(0x00 | (ColumnAddr & 0xF)); 
}



void System_Config(void){
//System initialization start-------------------
SYS_UnlockReg(); // Unlock protected registers
//Enable clock sources
//HXT (12 MHz)
CLK->PWRCON |= (1ul << 0);
while(!(CLK->CLKSTATUS & (1ul << 0)));

CLK->CLKSEL0 &= (~(0x07ul << 0));
    
//clock frequency division: 1
CLK->CLKDIV &= (~(0x0Ful));

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

//TM2 Clock selection and configuration
CLK->CLKSEL1 &= ~(0x07ul << 16); 
CLK->CLKSEL1 |= (0x02ul << 16);
CLK->APBCLK |= (0x01ul << 4);

SYS_LockReg(); // Lock protected registers
}



void Timer0_Config(void){
//Timer 0 initialization start--------------
TIMER0->TCSR &= ~(0xFFul << 0);
//set PRESCALE = 0

//reset Timer 0
TIMER0->TCSR |= (0x01ul << 26);

//define Timer 0 operation mode
TIMER0->TCSR &= ~(0x03ul << 27);
TIMER0->TCSR |= (0x01ul << 27);
//set periodic mode
    
TIMER0->TCSR &= ~(0x01ul << 24);
//CTB bit is set to 0, TDR is 24-bit up timer value 

//TDR to be updated continuously while timer counter is counting
TIMER0->TCSR |= (0x01ul << 16); 

TIMER0->TCSR &= ~(0x01 << 29); // no interrupt    
    
TIMER0->TCMPR |= (0x0FFFFFFul << 0);

//TEXCON initialization
TIMER0->TEXCON |= (0x01ul << 7);
TIMER0->TEXCON |= (0x01ul << 6);
TIMER0->TEXCON &= ~(0x01ul << 5);
TIMER0->TEXCON &= ~(0x01ul << 4);
TIMER0->TEXCON &= ~(0x03ul << 1);
TIMER0->TEXCON &= ~(0x01ul << 0);

TIMER0->TEXCON |= (0x01 << 3);

TIMER0->TCSR  |= (0x01 << 30);

//Timer 0 initialization end----------------
}


void Timer1_Config(void){
//Timer 1 initialization start--------------

// set PRESCALE = 12
TIMER1->TCSR &= ~(0xFFul << 0);
TIMER1->TCSR |= (11ul << 0);

//reset Timer 1
TIMER1->TCSR |= (0x01ul << 26);

//define Timer 1 operation mode: periodic
TIMER1->TCSR &= ~(0x03ul << 27);
TIMER1->TCSR |= (0x01ul << 27);
TIMER1->TCSR &= ~(0x01ul << 24);

//TDR to be updated continuously while timer counter is counting
TIMER1->TCSR |= (0x01ul << 16);

//TimeOut =  0.5s  --> Counter's TCMPR = 0.5s / (1/(1 MHz) = 500000
TIMER1->TCMPR = TIMER1_COUNTS - 1;

//start counting
TIMER1->TCSR |= (0x01ul<<30);

//Timer 1 initialization end----------------
}


void Timer2_Config(void){
//Timer 2 initialization start--------------

// set PRESCALE = 12
TIMER2->TCSR &= ~(0xFFul << 0);
TIMER2->TCSR |= (11ul << 0);

//reset Timer 1
TIMER2->TCSR |= (0x01ul << 26);

//define Timer 1 operation mode: periodic
TIMER2->TCSR &= ~(0x03ul << 27);
TIMER2->TCSR |= (0x01ul << 27);
TIMER2->TCSR &= ~(0x01ul << 24);

//TDR to be updated continuously while timer counter is counting
TIMER2->TCSR |= (0x01ul << 16);

//TimeOut = 0.4 ms (2.5k Hz) --> Counter's TCMPR = 0.4 ms / (1/(1 MHz) = 400
TIMER2->TCMPR = TIMER2_COUNTS - 1;

//start counting
TIMER2->TCSR |= (0x01ul<<30);
//Timer 2 initialization end----------------
}


void GPIO_Config(void){
//GPIO configuration
//LED5 display via GPIO-C12 
PC->PMD &= (~(0x03ul << 24));
PC->PMD |= (0x01ul << 24);
PC->DOUT |= (0x01ul << 12); 

//BUZZER to indicate interrupt handling routine
PB->PMD &= (~(0x03ul << 22));
PB->PMD |= (0x01ul << 22);
PB->DOUT |= (0x01ul << 11);

//GPIO Interrupt configuration. GPIO-B15 is the triggering source
PB->PMD &= (~(0x03ul << 30));
SYS->GPB_MFP |= (0x01ul << 15);
SYS->ALT_MFP |= (0x01ul << 24);    

}


void SPI3_Config (void){
SYS->GPD_MFP |= 1ul << 11; //1: PD11 is configured for alternative function
SYS->GPD_MFP |= 1ul << 9; //1: PD9 is configured for alternative function
SYS->GPD_MFP |= 1ul << 8; //1: PD8 is configured for alternative function
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
//------------------------------------------- main.c CODE ENDS -------------------------------------------------------------------------//
