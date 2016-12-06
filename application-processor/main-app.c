#define _XTAL_FREQ 20000000
#include <pic16f877a.h>
#include <xc.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include "keyboard.h"

#pragma config FOSC = HS       // Oscillator Selection bits (HS oscillator)
#pragma config WDTE = OFF       // Watchdog Timer Enable bit (WDT disabled)
#pragma config PWRTE = OFF      // Power-up Timer Enable bit (PWRT disabled)
#pragma config LVP = OFF

#define bit_set(val, bit) (val |= (1<<bit))
#define CR '\r'
#define ESC 0x1b
#define BREAK_KEY 0xF0

char key = 0;

void interrupt intr_RB0(void)
{
    static char temp_key = 0;
	static char cnt = 0;
    
    /* keyboard codes contain 1 Start bit,
     8 Data bits, 1 Parity bit, 1 Stop bit,
     Data bits are transmitted LSB first */
	if(INTCONbits.INTF == 1)
	{
        if(cnt > 0 && cnt < 9)
        {
            if(PORTBbits.RB1 == 1)
                bit_set(temp_key, cnt - 1);
        }
        cnt++;
        if(cnt == 11) // this was the stop bit
        {
            cnt = 0;
            key = temp_key;
            temp_key = 0;
        }
	}
	INTCONbits.INTF = 0;
	return;
}

void uart_init(void)
{
    TXSTA = 0;
    TXSTAbits.BRGH = 1;
	//bit_set(TXSTA,2); //High speed
	SPBRG = 129; //9600 baud rate
	TXSTAbits.TXEN = 1;
    RCSTAbits.SPEN = 1;
}

void uart_send_char(const char byte)
{
    TXREG = byte;
    while(TXSTAbits.TRMT == 0);
}
void uart_send_str(const char *str)
{
    while(*str != 0)
    {
        uart_send_char(*str);
        __delay_ms(50);
        str++;
    }
}

bool input_get_key(char *new_key)
{
    char ascii_key = 0;
    static char prev_key = 0;
    if(key != 0)
    {
        INTCONbits.INTE = 0; /* disable RB0 INT */
        if(prev_key == BREAK_KEY) /* on key release */
        {
            ascii_key = get_ascii_from_key(key);
            uart_send_char(ascii_key);
            *new_key = ascii_key;
            prev_key = key;
            key = 0;
            return true;
        }
        else
        {
            prev_key = key;
            key = 0;
            INTCONbits.INTF = 0;
            INTCONbits.INTE = 1;
            return false;
        }
    }
    return false;
}

bool input_get_nr(int *nr)
{
    int input_nr = 0;
    char curr_key = 0;
    char key_buff[5] = {0};
    int key_count = 0;
    
    while(key_count < 4)
    {
        while(!input_get_key(&curr_key));
        if(curr_key == CR)
        {
            if(key_count == 0)
                return false;
            else
                break;
        }
        else if(curr_key == BACKSPACE)
        {
            if(key_count > 0)
                key_count--;
            key_buff[key_count] = 0;
        }
        else
        {
            if(curr_key > 47 && curr_key < 58) //if it's a digit
            {
                key_buff[key_count] = curr_key;
                key_count++;
            }
            else
            {
                char dbg[4] = {0};
                uart_send_str(dbg);
                return false;
            }
        }
        //__delay_ms(700);
        INTCONbits.INTF = 0;
        INTCONbits.INTE = 1; /* enable RB0 INT */
    }
    
    /*while(key_count >= 0) //check if it is a nr
    {
        int power_of_ten = 1;
        int local_idx = 0;
        
        for(local_idx = key_count; local_idx > 0; local_idx--)
        {
            power_of_ten *= 10;
        }

        input_nr += (key_buff[key_count] - 48) * power_of_ten; 
        key_count--;
    }*/
    input_nr = strtol(&key_buff, NULL, 10);
    *nr = input_nr;
    return true; 
}

void main(void)
{
    TRISCbits.TRISC6 = 0;
    PORTCbits.RC6 = 0;
    TRISCbits.TRISC4 = 0;
    TRISBbits.TRISB0 = 1; /* use for PS2 clock */
    TRISBbits.TRISB1 = 1; /* use for PS2 data line */
    /* INT on falling edge of RB0*/
    OPTION_REGbits.INTEDG = 0;
    //OPTION_REGbits.nRBPU = 1;
    INTCONbits.INTE = 1; /* enable RB0 INT */
    INTCONbits.GIE = 1; /* enable INTs */

    __delay_ms(200);
    const char welcome_screen[] ="\
#########\r\
#PRIME  #\r\
#TESTER #\r\
#########\r\
\r\
ENTER NR:\r\0";
        //char welcome_screen[] = "A\0";  
    uart_init();
    uart_send_char(ESC); // clear screen
    __delay_ms(250);
    uart_send_str(welcome_screen);
    key = 0;
    
    while(1)
    {
        int i;
        int nr_to_test = 0;
        input_get_nr(&nr_to_test);
        for(i = 2; i < nr_to_test; i++)
        {
            if(nr_to_test % i == 0)
            {
                uart_send_str("\r\rNOT PRIME");
                break;
            }
            if(i == nr_to_test - 1)
                uart_send_str("\r\rIS PRIME");
        }
        PORTCbits.RC4 = 1;

    }
}