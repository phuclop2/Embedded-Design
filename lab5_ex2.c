//------------------------------------------- main.c CODE STARTS ---------------------------------------------------------------------------
#include <stdio.h>
#include "NUC100Series.h"
#include "lab5_ex2.h"
#define BUZZER_BEEP_TIME 1
#define BUZZER_BEEP_DELAY 200000

#define TIMER0_COUNTS 5

void EINT1_IRQHandler(void);
void TIMER0_IRQHandler(void);
void Buzzer_beep(int beep_time);

int main(void)
{
//System initialization start-------------------
SYS_UnlockReg(); // Unlock protected registers
//Enable clock sources
//HXT (12 MHz)
CLK->PWRCON |= (1ul << 0);
while(!(CLK->CLKSTATUS & (1ul << 0)));

CLK->CLKSEL0 &= (~(0x07ul << 0));
    
//clock frequency division: 1
CLK->CLKDIV &= ~0x0Ful;
    
//TM0 Clock selection and configuration
CLK->CLKSEL1 &= ~(0x07ul << 8);
CLK->CLKSEL1 |= (0x02ul << 8);
CLK->APBCLK |= (0x01ul << 2);
    
SYS_LockReg(); // Lock protected registers
    
//Timer 0 initialization start--------------
TIMER0->TCSR &= ~(0xFFul << 0);
//set PRESCALE = 0

//reset Timer 0
TIMER0->TCSR |= (0x01ul << 26);
    
TIMER0->TCSR |= (0x01ul << 24);
//CTB bit is set to 1, TDR is 24-bit up counter value 

//TDR to be updated continuously while timer counter is counting
TIMER0->TCSR |= (0x01ul << 16);

//Time = 5
TIMER0->TCMPR = TIMER0_COUNTS;

TIMER0->TCSR |= (0x01ul << 29);
//Timer Interrupt enable

TIMER0->TEXCON |= (0x01ul << 7); //enable de_bounce
TIMER0->TEXCON &= ~(0x01ul << 0); //falling-edge 

//start counting
TIMER0->TCSR |= (0x01ul << 30);
//Timer 0 initialization end----------------

//GPIOA.5: output push-pull
PA->PMD &= ~(0x03ul << 10);
PA->PMD |= (0x01ul << 10);
PA->DOUT &= ~(0x01ul << 5);

PB->PMD &= (~(0x03ul << 16));
SYS->GPB_MFP |= (0x01ul << 8); //Alternative function for GB.8
    
//LED display via GPIO-C12 to indicate main program execution
PC->PMD &= (~(0x03ul << 24));
PC->PMD |= (0x01ul << 24);
//BUZZER to indicate interrupt handling routine
PB->PMD &= (~(0x03ul << 22));
PB->PMD |= (0x01ul << 22);

//GPIO Interrupt configuration. GPIO-B15 is the interrupt source
PB->PMD &= (~(0x03ul << 30));
PB->IMD &= (~(1ul << 15));
PB->IEN |= (1ul << 15);
//NVIC interrupt configuration for GPIO-B15 interrupt source
NVIC->ISER[0] |= 1ul<<3;
NVIC->IP[0] &= (~(3ul<<30));
NVIC->IP[0] |= (1ul<<30);

//NVIC Interrupt configuration for TIMER0 interrupt source
NVIC->ISER[0] |= 1ul<<8;
NVIC->IP[2] &= (~(3ul<<6)); 

while(1){
    //while(TIMER0->TDR == TIMER0_COUNTS){
     //   PC->DOUT ^= 1 << 12;
   // }
}
}

// Interrupt Service Rountine of GPIO port B pin 15
void EINT1_IRQHandler(void){
    
Buzzer_beep(BUZZER_BEEP_TIME);
PA->DOUT = (1 << 5);
CLK_SysTickDelay(20000);
PA->DOUT = (0 << 5);
PB->ISRC |= (1ul << 15);
    
}


void TIMER0_IRQHandler(void){
    
Buzzer_beep(BUZZER_BEEP_TIME);
    
for(int i = 0; i < 6; i++){
    PC->DOUT ^= 1 << 12;
    CLK_SysTickDelay(50000);
    }

//reset Timer 0
TIMER0->TCSR |= (0x01ul << 26);
TIMER0->TISR |= (1ul << 0);
}

void Buzzer_beep(int beep_time){
int i;
for(i=0;i<(beep_time*2);i++){
PB->DOUT ^= (1 << 11);
CLK_SysTickDelay(BUZZER_BEEP_DELAY);
}
}
//------------------------------------------- main.c CODE ENDS ---------------------------------------------------------------------------
