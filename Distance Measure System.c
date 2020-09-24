//------------------------------------------- main.c CODE STARTS ---------------------------------------------------------------------------
// Assignment 2 - Distance Measurement System
#include <stdio.h>
#include "NUC100Series.h"
#include "LCD.h"

#define TIMER0_COUNTS 100000
#define TIMER1_COUNTS 40000

void System_Config(void);
void SPI3_Config(void);
void TIMER0_Config(void);
void TIMER1_Config(void);
void EINT0_IRQHandler(void);


void LCD_start(void);
void LCD_command(unsigned char temp);
void LCD_data(unsigned char temp);
void LCD_clear(void);
void LCD_SetAddress(uint8_t PageAddr, uint8_t ColumnAddr);

int main(void)
{
uint32_t TDR_val;
char TDR_val_s[4]="0000";
    
System_Config();
SPI3_Config();
TIMER0_Config();
TIMER1_Config();    

//GPIO initialization start --------------------
//GPIOA.5: output push-pull
PA->PMD &= ~(0x03ul << 10);
PA->PMD |= (0x01ul << 10);
PA->DOUT &= ~(0x01ul << 5);
//GPIO initialization end ----------------------
    
//GPIO Interrupt configuration. GPIO-B14 is the interrupt source
PB->PMD &= (~(0x03ul << 28)); // input
PB->IMD &= (~(1ul << 14)); // edge trigger
PB->IEN |= (1ul << 30); // rising edge
    
//NVIC interrupt configuration for GPIO-B14 interrupt source
NVIC->ISER[0] |= (1ul << 2);
NVIC->IP[0] &= (~(3ul << 22));    

LCD_start();
LCD_clear();
//--------------------------------
//LCD static content
//--------------------------------

printS_5x7(2, 10, "TDR value:");

while(1){
    while(!(TIMER0->TDR == TIMER0_COUNTS - 1));
        PA->DOUT = (1 << 5);
        CLK_SysTickDelay(2);
        PA->DOUT = (0 << 5); 
    CLK_SysTickDelay(10000);

    while(TIMER1->TCSR & (1ul << 25)); // wait until counter TIMER1 is stopping (CACT)
    // while(TIMER1->TCSR & (1ul << 30)); // wait until counter TIMER1 is stopping (CEN)
    //while(PB->ISRC & (1ul << 14)); // wait till an interrupt is not generating at GB.14
    
    TDR_val = TIMER1->TDR & 0x0000FFFF;
    CLK_SysTickDelay(50000);
    LCD_clear();
    sprintf(TDR_val_s, "%d", TDR_val);
    printS_5x7(4+5*10, 10, " ");
    printS_5x7(4+5*10, 10, TDR_val_s);
    CLK_SysTickDelay(50000);
}
}

//------------------------------------------------------------------------------------------------------------------------------------
// Functions definition
//------------------------------------------------------------------------------------------------------------------------------------
//Interrupt Service Rountine of GPIO port B pin 14
void EINT0_IRQHandler(void){

TIMER1->TCSR |= (0x01ul << 26); // reset the counter again after every time the interrupt is triggered
    
//start counting
TIMER1->TCSR |= 1ul << 30;

while(PB->PIN & (1ul << 14)); //wait till GPB.14 is back to 0, while GPB.14 is still 1,i.e. not 0, do nothing in while loop

//stop counting
TIMER1->TCSR &= ~(1ul << 30); // stop counter
//TDR will not be updated continuously while timer counter is counting
TIMER1->TCSR &= ~(0x01ul << 16); // stop update new value to TDR
    
PB->ISRC |= (1ul << 14); // clear pending to GB.14 Interrrupt
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

void System_Config (void){
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

SYS_LockReg(); // Lock protected registers
}

void SPI3_Config (void){
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

void TIMER0_Config (void){
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

void TIMER1_Config (void){
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
//------------------------------------------- main.c CODE ENDS ---------------------------------------------------------------------------