#ifndef _PATTERNS_HPP_
#define _PATTERNS_HPP_

/*
 * The following array defines patterns. 
 * The first column sets which LED's are on or off. 
 * The second column defines the brightness. 0 is full on, 1023 is full off.
 * The third column defines how many miliseconds they should be on for.
 * The array can have many many steps, with short timing between them. Worst case, you'll run out of memory on the chip (160KBytes worth!).
 */


//LED Test Pattern. 
//Standard LED Test Pattern
//This the pattern (on/off flashing) thats used in production to test the pins are soldered correctly etc.
int blinkPattern[][3] = {   {0b00110011,  0,  200 } ,
                          {0b11001100,  0,  200 } };

//Candle Flicker (on one of the LED's)
//The brightness (second column) is changed in each step as well as the time between steps (3rd column)
//Technically, this could be done with a clever randomizer algorithm. Left as an excercise to the reader! ;)

int candlePattern[][3] = {  
                          {0b00000111,  600,  100 } ,
                          {0b00000111,  300,  150 } ,
                          {0b00000111,  100,  200 } ,
                          {0b00000111,  0,  100 } ,
                          {0b00000111,  400,  100 } ,
                          {0b00000111,  800,  50 } ,
                          {0b00000111,  1000,  200 } ,
                          {0b00000111,  1020,  50 } ,
                          {0b00000111,  850,  150 } ,
                          {0b00000111,  900,  100 } ,
                          {0b00000111,  500,  150 } ,
                          {0b00000111,  100,  200 } ,
                          {0b00000111,  0,  100 } ,
                          {0b00000111,  600,  100 } ,
                          {0b00000111,  800,  50 } ,
                          {0b00000111,  100,  200 } ,
                          {0b00000111,  1020,  50 } ,
                          {0b00000111,  850,  150 } ,
                          {0b00000111,  900,  100 } ,
                          {0b00000111,  400,  200 } };


int pattern3[][3] = {   
                          {0b11111111,  0,  500 } ,
                          {0b00000000,  0,  500 } ,
                          {0b11111111,  0,  500} ,
                          {0b00000000,  0,  500 } ,
                          {0b10001000,  0,  250 } ,
                          {0b01000100,  0,  250 } ,
                          {0b00100010,  0,  250 } ,
                          {0b00010001,  0,  250 } ,
                          {0b00000000,  0,  1000 } };



#endif // _PATTERNS_HPP_
