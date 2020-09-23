//------------------------------------------- main.c CODE STARTS ---------------------------------------------------------------------------
// if debounce, then it is good, if not the buzzer will ring 
#include <stdio.h>
#include "NUC100Series.h"
#include "lab5_ex3.h"

#define BUZZER_BEEP_TIME 3
#define BUZZER_BEEP_DELAY 20000
#define TIMER0_COUNTS 8000000

void Buzzer_beep(int beep_time);
void EINT1_IRQHandler(void);

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
CLK->CLKDIV &= (~(0x0Ful));
    
//TM0 Clock selection and configuration
CLK->CLKSEL1 &= ~(0x07ul << 8);
CLK->CLKSEL1 |= (0x02ul << 8);
CLK->APBCLK |= (0x01ul << 2);
    
SYS_LockReg(); // Lock protected registers
    
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
//CTB bit is set to 0, TDR is 24-bit up timer value 

//TDR to be updated continuously while timer counter is counting
TIMER0->TCSR |= (0x01ul << 16);

//TimeOut = 50ms --> Counter's TCMPR = 50ms / (1/(1M Hz) = 50000
TIMER0->TCMPR = TIMER0_COUNTS - 1;

//Timer 0 initialization end----------------

    
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

while(1){

PC->DOUT ^= 1ul << 12;
    
if(TIMER0->TCSR & (1ul << 25)){
//stop counting
TIMER0->TCSR &= ~(0x01ul << 30);
Buzzer_beep(BUZZER_BEEP_TIME);
}

}
}

// Interrupt Service Rountine of GPIO port B pin 15
void EINT1_IRQHandler(void){
//reset Timer 0
TIMER0->TCSR |= (0x01ul << 26);
//start counting
TIMER0->TCSR |= (0x01ul << 30);
PB->ISRC |= (1ul << 15);
}

void Buzzer_beep(int beep_time){
int i;
for(i=0;i<(beep_time);i++){
PB->DOUT ^= (1 << 11);
CLK_SysTickDelay(BUZZER_BEEP_DELAY);
}
}
//------------------------------------------- main.c CODE ENDS ---------------------------------------------------------------------------
