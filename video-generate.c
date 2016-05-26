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

#define CHARS_PER_TEXT_LINE 10
#define NR_OF_TEXT_LINES 10
#define CHARS_TO_DISPLAY CHARS_PER_TEXT_LINE * NR_OF_TEXT_LINES
#define LINE_SPACE 7
#define FONT_LINES 5
#define FB_NR_LINES (NR_OF_TEXT_LINES * FONT_LINES) + (NR_OF_TEXT_LINES * LINE_SPACE)

#define VSYNC_LOW LATAbits.LATA1 = 0
#define VSYNC_HIGH LATAbits.LATA1 = 1

#define RED_ON LATCbits.LATC5 = 1
#define RED_OFF LATCbits.LATC5 = 0

#define BACKSPACE 8

bool tmr_triggered = false;
unsigned int line_nr = 0;
char text_line = 0;
char fb[FB_NR_LINES][CHARS_PER_TEXT_LINE];

void interrupt timer2_interrupt()
{
    char col = 0;
    if(text_line < FB_NR_LINES)
    {
        while(col < CHARS_PER_TEXT_LINE)
        {
            SSP1BUF = fb[text_line][col];
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

void char_to_fb(char ch)
{
    char font_nr = ch - 32;
    char font_line_nr = 0;
    static char lin = 0;
    static char col = 0;

    if(col > 0 && col % CHARS_PER_TEXT_LINE == 0)
	{
		lin += FONT_LINES + LINE_SPACE; //jump to a new text line
		col = 0;
	}
    
    if(ch == '\r')
    {
        lin += FONT_LINES + LINE_SPACE; //jump to a new text line
		col = 0;
        font_nr = '>' - 32;
    }
    else if(ch == BACKSPACE)
    {
        if(col == 1)
        {
            __delay_us(50);
            return;
        }
        //in case of backspace
        col--; //go back one position
        font_nr = ' ' - 32; //print a space to erase
        for(font_line_nr = 0; font_line_nr < 5; font_line_nr++)
        {
            fb[lin + font_line_nr][col] = fonts[font_nr][font_line_nr];
        }
        return;
    }

    for(font_line_nr = 0; font_line_nr < 5; font_line_nr++)
	{
        fb[lin + font_line_nr][col] = fonts[font_nr][font_line_nr];
	}
    col++;
}

void uart_init(void)
{
    TRISCbits.TRISC7 = 1; // UART RX1
    ANSELCbits.ANSC7 = 0;
    SPBRG1 = 103; // 9600 baud rate
    TXSTA1bits.SYNC = 0;
    RCSTA1bits.SPEN = 1; //enable serial port
    RCSTA1bits.CREN = 1; //enable receiving
}

void main(void)
{
    char i = 0;
/*  const char text[CHARS_TO_DISPLAY];
THE QUICK \
BROWN FOX \
JUMPS OVER\
THE LAZY  \
DOG 123456\
OPQRSTUVWX\
YZ12345678\
90ABCDEFGH\
IJKLMNOPQR\
PORC MARE ";*/
    
    /*OSCCONbits.IRCF = 7;
    OSCTUNEbits.PLLEN = 1;*/
    
    TRISCbits.TRISC5 = 0; // SDO output for color
    TRISAbits.TRISA0 = 0; // HSYNC output
    TRISAbits.TRISA1 = 0; // VSYNC output
    INTCONbits.PEIE = 1;
    PIE1bits.TMR2IE = 1; //enable timer 2 interrupt
    PIR1bits.TMR2IF = 0;
    //SLRCONbits.SLRC = 0;
    
    VSYNC_HIGH;
    
    spi_init();
    pwm_init();
    uart_init();

    memset(fb, 0, FB_NR_LINES * CHARS_PER_TEXT_LINE);

    char_to_fb('>');
    INTCONbits.GIE = 1; //enable interrupts
    
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
            if(PIR1bits.RC1IF == 1)
            {
                char_to_fb(RCREG1); //~56 us
                if(RCREG1 == '\r')
                    __delay_us(5);
                else
                    __delay_us(7);
            }
            else
                __delay_us(63);
            asm("nop"); asm("nop");
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
            text_line = 0;
        }
        else
        {
            __delay_us(5);
            if(line_nr < FB_NR_LINES + 1)
                text_line++;
            line_nr++;
        }
        tmr_triggered = false;
    }
}