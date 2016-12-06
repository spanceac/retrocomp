#include "keyboard.h"

char get_ascii_from_key(const char key)
{
    switch(key)
    {
        case 0x16:
            return '1';
        case 0x1e:
            return '2';
        case 0x26:
            return '3';
        case 0x25:
            return '4';
        case 0x2e:
            return '5';
        case 0x36:
            return '6';
        case 0x3d:
            return '7';
        case 0x3e:
            return '8';
        case 0x46:
            return '9';
        case 0x45:
            return '0';
        case 0x15:
            return 'Q';
        case 0x1d:
            return 'W';
        case 0x24:
            return 'R';
        case 0x2c:
            return 'T';
        case 0x35:
            return 'Y';
        case 0x3c:
            return 'U';
        case 0x43:
            return 'I';
        case 0x44:
            return 'O';
        case 0x4d:
            return 'P';
        case 0x5a:
            return '\r';
        case 0x1c:
            return 'A';
        case 0x1b:
            return 'S';
        case 0x66:
            return BACKSPACE; /* backspace */
        default:
            return 0;
    }
    return 0;
}
