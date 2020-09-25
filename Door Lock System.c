//------------------------------------------- main.c CODE STARTS ---------------------------------------------------------------------------
#include <stdio.h>
#include "NUC100Series.h"
#include "MCU_init.h"
#include "SYS_init.h"
#include "LCD.h"

void System_Config(void);
void SPI3_Config(void);
void LCD_start(void);
void LCD_command(unsigned char temp);
void LCD_data(unsigned char temp);
void LCD_clear(void);
void LCD_SetAddress(uint8_t PageAddr, uint8_t ColumnAddr);
void KeyPadEnable(void);
uint8_t KeyPadScanning(void);
void flash_passord(char digit[2], int16_t i);
int check_valid_password(char input[6], char password[6]);
void clear_password(char password[6]);
void update_password(char new_password[6], char system_pasword[6]);

typedef enum {START, LOGIN, UNLOCK, UPDATE1, UPDATE2, WRONG_PASS, SUCCESS_PASS_CHANGE} STATES;

int main(void)
{
STATES screen_state; 
uint8_t i;
uint8_t pressed_key = 0, pressed_password = 0;
char tmp_key[2] = "0";
char pressed_password_str[6] = "------";
char new_password_str[6] = "------";
char system_password_str[6] = "742774"; // 3748017 (Phuc), 3742774 (Trung), 3748575 (Long), 3757281 (Quang)
//--------------------------------
//System initialization
//--------------------------------
System_Config();
//--------------------------------
//SPI3 initialization
//--------------------------------
SPI3_Config();
//--------------------------------
//LCD initialization
//--------------------------------
LCD_start();
LCD_clear();
//--------------------------------
//LCD dynamic content
//--------------------------------

screen_state = START;

// Loading 4 ids or passwords into SRAM here, maybe put in a function

while(1){
	switch(screen_state){
		case START:
			// start of user interface
			//--------------------------------
			//LCD static content
			//--------------------------------
			printS_5x7(2, 0, "EEET2481-Design Door Lock");
			printS_5x7(2, 16, "1. Login");
			printS_5x7(2, 24, "2. Change key");
		
			while(pressed_key==0){ 
				pressed_key = KeyPadScanning();
			}
			if (pressed_key == 1){
				LCD_clear();
				screen_state = LOGIN;
			}
			else if (pressed_key == 2){
				LCD_clear();
				screen_state = UPDATE1;
			}
			else{
				screen_state = START;
			}
			pressed_key = 0;
			CLK_SysTickDelay(200000);
			break;
		
		case LOGIN:
			// login starts here
			printS_5x7(2,0,"Input password:");
			printS_5x7(2,35,"- - - - - -");
			for(i = 0;i < 6; i++){
				while(pressed_password == 0){
					pressed_password = KeyPadScanning();
				}
				sprintf(tmp_key, "%d", pressed_password);
				pressed_password_str[i] = tmp_key[0];
                flash_passord(tmp_key, i);
				pressed_password = 0;
				CLK_SysTickDelay(2000000);
			}	
			// check valid password
			if (check_valid_password(pressed_password_str, system_password_str)) {
				// correct password
				LCD_clear();
				screen_state = UNLOCK;
			} else {
				// wrong password
				LCD_clear();
				screen_state = WRONG_PASS;
			}
			// clear pressed_password_str, put it in a function later
			for(i = 0;i < 6; i++){
				pressed_password_str[i] = '-'; 
			}
			break;
			
		case UNLOCK:
			// Welcome home!!!
			print_Line(1,"WELCOME HOME");
			break;
		
		case UPDATE1:
			// input old password
			printS_5x7(2,0,"Input old password:");
			printS_5x7(2,35,"- - - - - -");
			for(i = 0;i < 6; i++){
				while(pressed_password == 0){
					pressed_password = KeyPadScanning();
				}
				sprintf(tmp_key, "%d", pressed_password);
				pressed_password_str[i] = tmp_key[0];
                flash_passord(tmp_key, i);
				pressed_password = 0;
				CLK_SysTickDelay(2000000);
			}	
			// check valid password
			if (check_valid_password(pressed_password_str, system_password_str)) {
				// correct password
				CLK_SysTickDelay(2000000);
				LCD_clear();
				screen_state = UPDATE2;
			} else {
				// wrong password
				LCD_clear();
				screen_state = WRONG_PASS;
			}
			// clear pressed_password_str
			clear_password(pressed_password_str);
			break;
			
		case UPDATE2:
			// input new password
			printS_5x7(2,0,"Input new password:");
			printS_5x7(2,35,"- - - - - -");
		  for(i = 0;i < 6; i++){
				while(pressed_password == 0){
					pressed_password = KeyPadScanning();
				}
				sprintf(tmp_key, "%d", pressed_password);
				new_password_str[i] = tmp_key[0];
				flash_passord(tmp_key, i);
				pressed_password = 0;
				CLK_SysTickDelay(2000000);
			}	
			// save in register ??? 
            update_password(new_password_str, system_password_str);
            clear_password(new_password_str);
			LCD_clear();
			screen_state = SUCCESS_PASS_CHANGE;
			break;
			
		case SUCCESS_PASS_CHANGE:
			// successful password change
			printS_5x7(2,20,"Your key has been changed");
			printS_5x7(17,30,"* * * * * *");
			printS_5x7(35,50,"Thank you!");
			for (i = 0; i < 6; i++){
				CLK_SysTickDelay(2000000);
			}
			// clear pressed_password_str
			clear_password(pressed_password_str);
			LCD_clear();
			screen_state = START;
			break;
		
		case WRONG_PASS:
			// wrong password input
			printS_5x7(20,20,"The key is wrong!");
			printS_5x7(2,30,"System will restart in 1 second");
			printS_5x7(35,35,"Thank you!");
			for (i = 0; i < 6; i++){
				CLK_SysTickDelay(2000000);
			}
			LCD_clear();
			screen_state = START;
			break;
		
	}
}
}

//------------------------------------------------------------------------------------------------------------------------------------
// User Defined Functions
//------------------------------------------------------------------------------------------------------------------------------------
void flash_passord(char digit[2], int16_t i) {
    printS_5x7(2 + i*10,35,digit);
    CLK_SysTickDelay(200000);
    printS_5x7(2 + i*10,35,"*");
}

int check_valid_password(char input[6], char password[6]) {
    for (int i = 0; i < 6; i++) {
        if (input[i] != password[i]) return 0;
    }
    return 1;
}

void clear_password(char password[6]) {
	for(int i = 0;i < 6; i++){
		password[i] = '-'; 
	}
}

void update_password(char new_password[6], char system_pasword[6]) {
    for (int i = 0; i < 6; i++) {
        system_pasword[i] = new_password[i];
    }
}
//------------------------------------------------------------------------------------------------------------------------------------
// Functions definition of System Initialization
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


void KeyPadEnable(void){
GPIO_SetMode(PA, BIT0, GPIO_MODE_QUASI);
GPIO_SetMode(PA, BIT1, GPIO_MODE_QUASI);
GPIO_SetMode(PA, BIT2, GPIO_MODE_QUASI);
GPIO_SetMode(PA, BIT3, GPIO_MODE_QUASI);
GPIO_SetMode(PA, BIT4, GPIO_MODE_QUASI);
GPIO_SetMode(PA, BIT5, GPIO_MODE_QUASI);
}


uint8_t KeyPadScanning(void){
PA0=1; PA1=1; PA2=0; PA3=1; PA4=1; PA5=1;
if (PA3==0) return 1;
if (PA4==0) return 4;
if (PA5==0) return 7;
PA0=1; PA1=0; PA2=1; PA3=1; PA4=1; PA5=1;
if (PA3==0) return 2;
if (PA4==0) return 5;
if (PA5==0) return 8;
PA0=0; PA1=1; PA2=1; PA3=1; PA4=1; PA5=1;
if (PA3==0) return 3;
if (PA4==0) return 6;
if (PA5==0) return 9;
return 0;
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
//enable clock of SPI3
CLK->APBCLK |= 1ul << 15;
SYS_LockReg(); // Lock protected registers
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
SPI3->DIVIDER = 0; // SPI clock divider. SPI clock = HCLK / ((DIVIDER+1)*2). HCLK = 50 MHz
}
//------------------------------------------- main.c CODE ENDS ---------------------------------------------------------------------------