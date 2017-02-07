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

#define CHARS_PER_TEXT_LINE 14
#define NR_OF_TEXT_LINES 14
#define CHARS_TO_DISPLAY ((CHARS_PER_TEXT_LINE) * (NR_OF_TEXT_LINES))
#define LINE_SPACE 7
#define FB_NR_LINES (((NR_OF_TEXT_LINES) * (FONT_LINES)))

#define VSYNC_LOW LATAbits.LATA1 = 0
#define VSYNC_HIGH LATAbits.LATA1 = 1

#define RED_ON LATCbits.LATC5 = 1
#define RED_OFF LATCbits.LATC5 = 0

#define BACKSPACE 8
#define ESC 0x1B

#define TEST_MODE 0

bool tmr_triggered = false;
unsigned int line_nr = 0;

char fb[FB_NR_LINES * CHARS_PER_TEXT_LINE];
char *fb_pos = fb;

#if TEST_MODE == 1
const char test_text[CHARS_TO_DISPLAY] ="\
THE QUICKDICKY\
BROWN FOXSHO\r\
JUMPS ON LE\r\
THE LAZY NO\r\
PASSI TAI MEI\r\
GAOI GAOI HOI\r\
IJKLMNOPQRSTU\r\
PORC MARES DRG\r\
12345678901234\
BEBE MEME TEST\
BEBE VEVE TEST\
BEBE HEHE TEST\
THIS IS SPARTA\
NO, IT AIN'T S";
#endif

void interrupt timer2_interrupt()
{
    char cnt = 0;
    while(cnt < CHARS_PER_TEXT_LINE)
    {
        SSP1BUF = *(fb_pos + cnt);
        asm("nop");asm("nop");asm("nop");
        cnt++;
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
    static char *fb_p= fb;
    static char count = 0;

    /* check for out of bounds access */
    if(fb_p >= (fb + FB_NR_LINES * CHARS_PER_TEXT_LINE))
    {
        __delay_us(28);
        return;
    }

    if(count == CHARS_PER_TEXT_LINE &&
            (ch != '\r') && (ch != BACKSPACE))
	{
		count = 0;
		fb_p += CHARS_PER_TEXT_LINE * FONT_LINES;
	}
    
    if(ch == '\r')
    {
        //jump to a new text line
        fb_p += CHARS_PER_TEXT_LINE * FONT_LINES;
		count = 0;
        __delay_us(28);
        return;
    }
    else if(ch == BACKSPACE)
    {
        if(count == 0)
        {
            __delay_us(28);
            return;
        }
        //in case of backspace
        count--; //go back one position
        font_nr = ' ' - 32; //print a space to erase
        for(font_line_nr = 0; font_line_nr < FONT_LINES; font_line_nr++)
        {
            *(fb_p + (font_line_nr * CHARS_PER_TEXT_LINE) + count) = fonts[font_nr][font_line_nr];
        }
        return;
    }
    else if(ch == ESC)
    {
        /* clear the screen */
        count = 0;
        fb_p= fb;
        memset(fb, 0, FB_NR_LINES * CHARS_PER_TEXT_LINE);
        __delay_us(28);
        return;
    }

    for(font_line_nr = 0; font_line_nr < FONT_LINES; font_line_nr++)
	{
        *(fb_p + (font_line_nr * CHARS_PER_TEXT_LINE) + count) = fonts[font_nr][font_line_nr];
	}
    count++;
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
    char rx_char = 0;
    char local_lin_count = 0;
    char line_is_empty = 0;
    char empty_line[CHARS_PER_TEXT_LINE] = {0};
    char *temp_fb_p;
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

#if TEST_MODE == 1
    for(i = 0; i < 196; i++)
        char_to_fb(test_text[i]);
#endif

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
                rx_char = RCREG1;
                char_to_fb(rx_char); //~57 us
                __delay_us(39);

                asm("nop"); asm("nop"); asm("nop"); asm("nop");
                asm("nop"); asm("nop"); asm("nop"); asm("nop");
                asm("nop"); asm("nop");
            }
            else
            {
                __delay_us(63);
                asm("nop"); asm("nop");
                asm("nop");
            }
            VSYNC_HIGH;

            line_nr += 3;
            INTCONbits.GIE = 1;
        }

        if(line_nr == 524)
        {
            __delay_us(5);
            line_nr = 0;
            fb_pos = fb;
        }
        else
        {
            __delay_us(5);
            /*test to see if current line is space between consecutive text lines */
            if(local_lin_count == FONT_LINES - 1)
            {
                line_is_empty = 1;
                temp_fb_p = fb_pos + CHARS_PER_TEXT_LINE;
                fb_pos = empty_line;
            }

            if(!line_is_empty && line_nr < (FB_NR_LINES + NR_OF_TEXT_LINES * LINE_SPACE) - 1)
            {
                fb_pos += CHARS_PER_TEXT_LINE;
            }

            line_nr++;

            if(line_nr < (FB_NR_LINES + NR_OF_TEXT_LINES * LINE_SPACE))
            {
                local_lin_count++;
            }
            else
            {
                local_lin_count = 0;
                fb_pos = empty_line;
                line_is_empty = 0;
            }
            /* if a text line and the space following the line were painted*/
            if(local_lin_count == FONT_LINES + LINE_SPACE)
            {
                line_is_empty = 0;
                local_lin_count = 0;
                fb_pos = temp_fb_p;
            }
        }
        tmr_triggered = false;
    }
}