/* 16 MHz Crystal * 4 PLL = 64 MHz osc -> 16 MHz Fcy */
#pragma config FOSC=HSHP, PLLCFG=ON, PRICLKEN = ON, WDTEN = OFF, LVP = OFF
#pragma config IESO = OFF

#define _XTAL_FREQ 64000000

#include <p18f24k22.h>
#include <xc.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "fonts.h"

#define CHARS_TO_DISPLAY 12
#define CHARS_PER_TEXT_LINE 4
#define LINE_SPACE 3

#define VSYNC_LOW LATAbits.LATA1 = 0
#define VSYNC_HIGH LATAbits.LATA1 = 1

#define RED_ON LATCbits.LATC5 = 1
#define RED_OFF LATCbits.LATC5 = 0

bool tmr_triggered = false;
unsigned int line_nr = 0;
char fb[24][CHARS_PER_TEXT_LINE];

void interrupt timer2_interrupt()
{
    char col = 0;
    if(line_nr < 24)
    {
        while(col < CHARS_PER_TEXT_LINE)
        {
            SSP1BUF = fb[line_nr][col];
            col++;
        }
    }
    tmr_triggered = true;
    PIR1bits.TMR2IF = 0;
}

void spi_init(void)
{
    //SPI Master mode, clock = FOSC /(4 * (SSPxADD+1))
    SSP1ADD = 1; // set SPI speed to 64MHz/((SSP1ADD + 1) * 4) = 8MHz
    SSP1CON1 = 10;
    SSP1BUF = 0;
    SSP1STATbits.CKE = 1; //SDO low when not transmitting
    SSP1CON1bits.SSPEN1 = 1;
}

void pwm_init(void)
{
    // CCP4 -> simple pwm for p18f25k22
    CCPTMRS1 = 0; //use TMR2 for CCP4 PWM
    PR2 = 126; //32 KHz pwm
    CCP4CON = 0x0C; // CCP4 -> PWM mode
    CCPR4L = 448 >> 2; //duty cycle
    CCP4CON |= (448 & 3) << 4;
    PIR1bits.TMR2IF = 0;
    T2CON = 5; //start timer 2, 1:4 prescaler
    TRISBbits.RB0 = 0; //set CCP4 as output
}

void main(void)
{
    char i = 0;
    char text[12] = "SEBITEST1234";
    char lin = 0, col = 0, displayed_chars;
	char font_line_nr = 0;
    
    /*OSCCONbits.IRCF = 7;
    OSCTUNEbits.PLLEN = 1;*/
    
    TRISCbits.TRISC5 = 0; // SDO output for color
    TRISAbits.TRISA0 = 0; // HSYNC output
    TRISAbits.TRISA1 = 0; // VSYNC output
    INTCONbits.GIE = 1; //enable interrupts
    INTCONbits.PEIE = 1;
    PIE1bits.TMR2IE = 1; //enable timer 2 interrupt
    PIR1bits.TMR2IF = 0;
    SLRCONbits.SLRC = 0;
    
    VSYNC_HIGH;
    
    spi_init();
    pwm_init();

    memset(fb, 0, 24 * CHARS_PER_TEXT_LINE);
	for(displayed_chars = 0; displayed_chars < CHARS_TO_DISPLAY; displayed_chars++)
	{
		int font_nr = text[displayed_chars] - 32;
		
		if(displayed_chars > 0 && displayed_chars % CHARS_PER_TEXT_LINE == 0)
		{
			lin += 5 + LINE_SPACE; //jump to a new text line
			col = 0;
		}
		
		//paint font in framebuffer
		for(font_line_nr = 0; font_line_nr < 5; font_line_nr++)
		{
            fb[lin + font_line_nr][col] = fonts[font_nr][font_line_nr];
		}
		col++; // go to next font placement
	}
    col = 0;
    
    while(1)
    {
        while(!tmr_triggered); // wait for a timer overflow

        if(line_nr == 479)
        {
            INTCONbits.GIE = 0; //disable interrupts
            __delay_us(26);
            asm("nop"); asm("nop"); asm("nop"); asm("nop");
            asm("nop"); asm("nop"); asm("nop"); asm("nop");
            asm("nop"); asm("nop"); asm("nop");
            VSYNC_LOW;
            __delay_us(63);
            asm("nop"); asm("nop"); asm("nop"); asm("nop");
            asm("nop"); asm("nop"); asm("nop"); asm("nop");
            asm("nop");
            VSYNC_HIGH;

            line_nr += 3;
            
            INTCONbits.GIE = 1;
        }

        if(line_nr == 524)
        {
            __delay_us(5);
            line_nr = 0;
        }
        else
        {
            __delay_us(5);
            line_nr++;
        }
        tmr_triggered = false;
    }
}