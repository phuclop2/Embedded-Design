#include <stdio.h>
#include "NUC100Series.h"
#include "LCD.h"

#define BUZZER_BEEP_TIME 3
#define BUZZER_BEEP_DELAY 200000

void System_Config(void);
void SPI3_Config(void);
void ADC7_Config(void);
void LCD_start(void);
void LCD_command(unsigned char temp);
void LCD_data(unsigned char temp);
void LCD_clear(void);
void LCD_SetAddress(uint8_t PageAddr, uint8_t ColumnAddr);
void Buzzer_beep(int beep_time);
void Buzzer_off(void);

int main(void)
{
uint32_t adc7_val;
char adc7_val_s[4]="0000";
System_Config();
SPI3_Config();
ADC7_Config();
LCD_start();
LCD_clear();
//--------------------------------
//LCD static content
//--------------------------------
printS_5x7(2, 0, "EEET2481 Lab 7 ADC");
printS_5x7(2, 8, "ADC7 conversion test");
printS_5x7(2, 16, "Reference voltage: 5 V");
printS_5x7(2, 24, "A/D resolution: 1.221 mV");
printS_5x7(2, 40, "A/D value:");

ADC->ADCR |= (0x01ul << 11); // start ADC channel 7 conversion
while(1){
while(!(ADC->ADSR & (0x01ul << 0))); // wait until conversion is completed (ADF=1)
ADC->ADSR |= (0x01ul << 0); // write 1 to clear ADF
adc7_val = ADC->ADDR[7] & 0x0000FFFF;
sprintf(adc7_val_s, "%d", adc7_val);
printS_5x7(4+5*10, 40, " ");
printS_5x7(4+5*10, 40, adc7_val_s);
	if (adc7_val > 2458){
		Buzzer_beep(BUZZER_BEEP_TIME);
	}
	else{
		Buzzer_off();
	}
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
LCD_command(0x10 | (ColumnAddr>>4)&0xF);
LCD_command(0x00 | (ColumnAddr & 0xF));
}
void System_Config (void){
SYS_UnlockReg(); // Unlock protected registers
CLK->PWRCON |= (0x01ul << 0);
while(!(CLK->CLKSTATUS & (1ul << 0)));


//PLL configuration starts
CLK->PLLCON &= ~(1ul<<19); //0: PLL input is HXT
CLK->PLLCON &= ~(1ul<<16); //PLL in normal mode
CLK->PLLCON &= (~(0x01FFul << 0));
CLK->PLLCON |= 48;
CLK->PLLCON &= ~(1ul<<18); //0: enable PLLOUT
while(!(CLK->CLKSTATUS & (0x01ul << 2)));
//PLL configuration ends
//clock source selection
CLK->CLKSEL0 &= (~(0x07ul << 0));
CLK->CLKSEL0 |= (0x02ul << 0);
//clock frequency division
CLK->CLKDIV &= (~0x0Ful << 0);
// SPI3 clock enable
CLK->APBCLK |= 1ul << 15;
//ADC Clock selection and configuration
CLK->CLKSEL1 &= ~(0x03ul << 2); // ADC clock source is 12 MHz
CLK->CLKDIV &= ~(0x0FFul << 16);
CLK->CLKDIV |= (0x0Bul << 16); // ADC clock divider is (11+1) --> ADC clock is 12/12 = 1 MHz
CLK->APBCLK |= (0x01ul << 28); // enable ADC clock
SYS_LockReg(); // Lock protected registers

//BUZZER 
PB->PMD &= (~(0x03ul << 22));
PB->PMD |= (0x01ul << 22);
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
SPI3->DIVIDER = 0; // SPI clock divider. SPI clock = HCLK / ((DIVIDER+1)*2). HCLK = 50 MHz
}
void ADC7_Config(void) {
PA->PMD &= ~(0x03ul << 14);
PA->PMD |= (0x00ul << 14); // PA.7 is input pin
PA->OFFD |= (0x01ul << 23); // PA.7 digital input path is disabled
SYS->GPA_MFP |= (0x01ul << 7); // GPA_MFP[7] = 1 for ADC7
SYS->ALT_MFP &= ~(0x01ul << 11); //ALT_MFP[11] = 0 for ADC7
//ADC operation configuration
ADC->ADCR |= (0x03ul << 2); // continuous scan mode
ADC->ADCR &= ~(0x01ul <<1); // ADC interrupt is disabled
ADC->ADCR |= (0x01ul <<0); // ADC is enabled
ADC->ADCHER &= ~(0x03ul << 8); // ADC7 input source is external pin
ADC->ADCHER |= (0x01ul << 7); // ADC channel 7 is enabled.
}
void Buzzer_beep(int beep_time){
int i;
for(i=0;i<(beep_time*2);i++){
PB->DOUT ^= (1 << 11);
CLK_SysTickDelay(BUZZER_BEEP_DELAY);
}
}
void Buzzer_off(void){
PB->DOUT = (1 << 11);
}