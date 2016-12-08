This project is a retro computer made with two PIC microcontrollers:

– a PIC16F877A MCU that acts as the application processor: read PS/2 keyboard input, process input, sends information to video interface

– a PIC18F25K50 MCU that acts as a video interface: takes input from application processor, stores and modifies the framebuffer, generates VGA signals to be displayed on the monitor

The code in the root folder of this repo "video-generate.c" and "fonts.h"  is the code for the PIC18 video processor.

In the folder "application-processor" you can find the code for the PIC16 application processor.

Detailed info and a video demo for the project can be seen at http://hackingbeaver.com/?p=904
