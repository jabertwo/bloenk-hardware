/*
 *   ____  _ _   _       _    
 *  |  _ \| (_) (_)     | |   
 *  | |_) | | ___  _ __ | | __
 *  |  _ <| |/ _ \| '_ \| |/ /
 *  | |_) | | (_) | | | |   < 
 *  |____/|_|\___/|_| |_|_|\_\    
 * 
 *   WS2812 LIB   
 *   v0.1
 *  
 * by DomesticHacks
 * http://www.domestichacks.info/
 * http://www.youtube.com/DomesticHacks
 *
 * Author: Johannes Zinnau (johannes@johnimedia.de)
 * 
 * Based on: light weight WS2812 lib
 *
 * Created: 07.04.2013 15:58:05 - v0.1
 *			06.05.2013          - v0.3 - clean up
 *			27.05.2013			- v0.6 - clean up, removed RC variants, added mask
 *      08.08.2013      - v0.9 - 20 Mhz version added
 *  Author: Tim (cpldcpu@gmail.com) 
 */ 

#include <avr/io.h>

#ifndef LIGHT_WS2812_H_
#define LIGHT_WS2812_H_

// Call with address to led color array (order is Green-Red-Blue)
// Numer of bytes to be transmitted is leds*3

void ws2812_sendarray(uint8_t *ledarray,uint16_t length);
void ws2812_sendarray_mask(uint8_t *ledarray,uint16_t length, uint8_t mask);

///////////////////////////////////////////////////////////////////////
// User defined area: Define I/O pin
///////////////////////////////////////////////////////////////////////

#define ws2812_port PORTB						// Data port register
#define ws2812_pin 0							// Number of the data out pin

///////////////////////////////////////////////////////////////////////
// End user defined area
///////////////////////////////////////////////////////////////////////

#endif /* LIGHT_WS2812_H_ */
