//------------------------------------------- main.c CODE STARTS ---------------------------------------------------------------------------
#include <stdio.h>
#include "NUC100Series.h"
#include "lab5_ex2.h"
#define BUZZER_BEEP_TIME 1
#define BUZZER_BEEP_DELAY 20000

void EINT1_IRQHandler(void);
void Buzzer_beep(int beep_time);

unsigned int counter = 0;

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
    
SYS_LockReg(); // Lock protected registers

    
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
    if(counter == 5){
            PC->DOUT ^= 1 << 12; // LED5
            CLK_SysTickDelay(100000);
    }
}
}

// Interrupt Service Rountine of GPIO port B pin 15
void EINT1_IRQHandler(void){

counter = counter + 1;
    
Buzzer_beep(BUZZER_BEEP_TIME);
    
PB->ISRC |= (1ul << 15);
}

void Buzzer_beep(int beep_time){
int i;
for(i=0;i<(beep_time*2);i++){
PB->DOUT ^= (1 << 11);
CLK_SysTickDelay(BUZZER_BEEP_DELAY);
}
}
//------------------------------------------- main.c CODE ENDS ---------------------------------------------------------------------------
