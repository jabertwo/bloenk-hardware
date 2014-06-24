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
 * Created: 07.04.2013 15:57:49 - v0.1
 *			21.04.2013 15:57:49 - v0.2 - Added 12 Mhz code, cleanup
 *			07.05.2013          - v0.4 - size optimization, disable irq
 *			20.05.2013          - v0.5 - Fixed timing bug from size optimization
 *			27.05.2013			- v0.6 - Major update: Changed I/O Port access to byte writes
 *										 instead of bit manipulation. This removes this timing
 *										 discrepancy between standard AVR and reduced core so that
 *										 only one routine is required. This comes at the cost of
 *										 additional register usage.
 *			28.05.2013			- v0.7 - Optimized timing and size of 8 and 12 Mhz routines. 
 *										 All routines are within datasheet specs now, except of
 *										 9.6 Mhz which is marginally off.			
 *			03.06.2013			- v0.8 - 9.6 Mhz implementation now within specifications.
 *								-		 brvs->brcs. Loops terminate correctly
 *  Author: Tim (cpldcpu@gmail.com) 
 */ 

// Tested:
// Attiny 85	4 MHz, 8 MHz, 16 MHz, 16.5 MHz (Little-Wire)
// Attiny 13A	9.6 MHz
// Attiny 10	4 Mhz, 8 Mhz (Reduced core)
// Atmega 8		12 Mhz


#include "light_ws2812.h"
#include <avr/interrupt.h>
#include <avr/io.h>

void ws2812_sendarray_mask(uint8_t *, uint16_t , uint8_t);

void ws2812_sendarray(uint8_t *data,uint16_t datlen)
{
	ws2812_sendarray_mask(data,datlen,_BV(ws2812_pin));
}


/*
	This routine writes an array of bytes with RGB values to the Dataout pin
	using the fast 800kHz clockless WS2811/2812 protocol.
	
	The description of the protocol in the datasheet is somewhat confusing and
	it appears that some timing values have been rounded. 
	
	The order of the color-data is GRB 8:8:8. Serial data transmission begins 
	with the most significant bit in each byte.
	
	The total length of each bit is 1.25µs (20 cycles @ 16Mhz)
	* At 0µs the dataline is pulled high.
	* To send a zero the dataline is pulled low after 0.375µs (6 cycles).
	* To send a one the dataline is pulled low after 0.625µs (10 cycles).
	
	After the entire bitstream has been written, the dataout pin has to remain low
	for at least 50µS (reset condition).
	
	Due to the loop overhead there is a slight timing error: The loop will execute
	in 21 cycles for the last bit write. This does not cause any issues though,
	as only the timing between the rising and the falling edge seems to be critical.
	Some quick experiments have shown that the bitstream has to be delayed by 
	more than 3µs until it cannot be continued (3µs=48 cyles).

*/

void ws2812_sendarray_mask(uint8_t *data,uint16_t datlen,uint8_t maskhi)
{
	uint8_t curbyte,ctr,masklo;
	masklo	=~maskhi&ws2812_port;
	maskhi |=ws2812_port;
	
	while (datlen--) {
		curbyte=*data++;
		
		asm volatile(
		
		"		ldi	%0,8		\n\t"		// 0
		"loop%=:out	%2,	%3		\n\t"		// 1
		"		lsl	%1			\n\t"		// 2
		"		dec	%0			\n\t"		// 3

		"		rjmp .+0		\n\t"		// 5
		
		"		brcs .+2		\n\t"		// 6l / 7h
		"		out	%2,	%4		\n\t"		// 7l / -
		"		nop				\n\t"		// blönk
		"		rjmp .+0		\n\t"		// 9
		"		rjmp .+0		\n\t"		// blönk
		
		"		nop				\n\t"		// 10
		"		out	%2,	%4		\n\t"		// 11
		"		breq end%=		\n\t"		// 12      nt. 13 taken

		"		rjmp .+0		\n\t"		// 14
		//"		rjmp .+0		\n\t"		// 16 (blönk)
		"		rjmp .+0		\n\t"		// 18
		"		rjmp loop%=		\n\t"		// 20
		"end%=:					\n\t"
		:	"=&d" (ctr)
		:	"r" (curbyte), "I" (_SFR_IO_ADDR(ws2812_port)), "r" (maskhi), "r" (masklo)
		);
	}
}
