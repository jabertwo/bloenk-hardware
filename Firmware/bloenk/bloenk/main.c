/*
 *   ____  _ _   _       _    
 *  |  _ \| (_) (_)     | |   
 *  | |_) | | ___  _ __ | | __
 *  |  _ <| |/ _ \| '_ \| |/ /
 *  | |_) | | (_) | | | |   < 
 *  |____/|_|\___/|_| |_|_|\_\    
 * 
 *   Blönk Firmware     
 *   v0.2
 *  
 * by DomesticHacks
 * http://www.domestichacks.info/
 * http://www.youtube.com/DomesticHacks
 *
 * Author: Johannes Zinnau (johannes@johnimedia.de)
 * 
 * License:
 * GNU GPL v2
 *
 * Based on: VUSB hid-custom-rq example
 *  Name: main.c
 *  Project: hid-custom-rq example
 *  Author: Christian Starkjohann
 *  Creation Date: 2008-04-07
 *  Tabsize: 4
 *  Copyright: (c) 2008 by OBJECTIVE DEVELOPMENT Software GmbH
 *  License: GNU GPL v2 (see License.txt), GNU GPL v3 or proprietary (CommercialLicense.txt)
 */

#define LED_PORT_DDR        DDRB
#define LED_PORT_OUTPUT     PORTB
#define LED_BIT             0
#define LED_DEFAULT_COUNT	8

#define abs(x) ((x) > 0 ? (x) : (-x))

#include <avr/io.h>
#include <avr/wdt.h>
#include <avr/eeprom.h>
#include <avr/interrupt.h>
#include <util/delay.h>

#include <avr/pgmspace.h>
#include "usbconfig.h"
#include "usbdrv/usbdrv.h"
#include "requests.h"
#include "light_ws2812/light_ws2812.h"

uint8_t eeCalibrationMemory EEMEM = 0;
uint8_t eeLedCountMemory EEMEM = 1;
struct CRGB { uint8_t g; uint8_t r; uint8_t b; };
struct CRGB led[254];
uint8_t currentLed;
uint8_t ledCount;
static uchar replyBuffer[1];

void setLedColor(uint8_t ledNum, uint8_t r, uint8_t g, uint8_t b)
{
	led[ledNum].r = r;
	led[ledNum].g = g;
	led[ledNum].b = b;
}

void writeToLeds()
{
	uint8_t i;

	cli();
	for (i=0; i<ledCount; i++) {
		ws2812_sendarray((uint8_t *)&led[i],3);
	}
	sei();
	
	_delay_us(50);
}

void setLedCount(uint8_t count)
{
	if(count > 0 && count < 254){
		ledCount = count;
		if(currentLed >= count){
			currentLed = 0;
		}
		eeprom_update_byte(&eeLedCountMemory, count);
	}
}

usbMsgLen_t usbFunctionSetup(uchar data[8])
{
    uint8_t returnLen = 0;
	usbRequest_t    *rq = (void *)data;
    if((rq->bmRequestType & USBRQ_TYPE_MASK) == USBRQ_TYPE_VENDOR){
        if(rq->bRequest == CUSTOM_RQ_SET_CURRENT_LED){
			if (rq->wValue.bytes[0] < ledCount) {
				currentLed = rq->wValue.bytes[0];
			}
        }
        
        if(rq->bRequest == CUSTOM_RQ_SET_COLOR_R){
          led[currentLed].r = rq->wValue.bytes[0];
        }
        
        if(rq->bRequest == CUSTOM_RQ_SET_COLOR_G){
          led[currentLed].g = rq->wValue.bytes[0];
        }
        
        if(rq->bRequest == CUSTOM_RQ_SET_COLOR_B){
          led[currentLed].b = rq->wValue.bytes[0];
        }
        
        if(rq->bRequest == CUSTOM_RQ_WRITE_TO_LEDS){
          writeToLeds();
        }
        
        if(rq->bRequest == CUSTOM_RQ_SET_LEDCOUNT){
          setLedCount(rq->wValue.bytes[0]);
        }
        
        if(rq->bRequest == CUSTOM_RQ_GET_LEDCOUNT){
          replyBuffer[0] = ledCount;
          usbMsgPtr = replyBuffer;
          returnLen = 1;
        }
    }
    return returnLen;
}

static void calibrateOscillator(void)
{
	uchar       step = 128;
	uchar       trialValue = 0, optimumValue;
	int         x, optimumDev, targetValue = (unsigned)(1499 * (double)F_CPU / 10.5e6 + 0.5);

    /* do a binary search: */
    do{
        OSCCAL = trialValue + step;
        x = usbMeasureFrameLength();    /* proportional to current real frequency */
        if(x < targetValue)             /* frequency still too low */
            trialValue += step;
        step >>= 1;
    }while(step > 0);
    /* We have a precision of +/- 1 for optimum OSCCAL here */
    /* now do a neighborhood search for optimum value */
    optimumValue = trialValue;
    optimumDev = x; /* this is certainly far away from optimum */
    for(OSCCAL = trialValue - 1; OSCCAL <= trialValue + 1; OSCCAL++){
        x = usbMeasureFrameLength() - targetValue;
        if(x < 0)
            x = -x;
        if(x < optimumDev){
            optimumDev = x;
            optimumValue = OSCCAL;
        }
    }
    OSCCAL = optimumValue;
}

void hadUsbReset(void)
{
    cli(); // usbMeasureFrameLength() counts CPU cycles, so disable interrupts.
    calibrateOscillator();
    sei();
    eeprom_update_byte(&eeCalibrationMemory, OSCCAL);   /* store the calibrated value in EEPROM */
}

int __attribute__((noreturn)) main(void)
{
	uchar   i;

	// Read oscillator calibration
	uchar   calibrationValue;
	calibrationValue = eeprom_read_byte(&eeCalibrationMemory); /* calibration value from last time */
    if(calibrationValue != 0xff){
        OSCCAL = calibrationValue;
    }
	
    wdt_enable(WDTO_1S);
	
	// Init USB
    usbInit();
    usbDeviceDisconnect();  /* enforce re-enumeration, do this while interrupts are disabled! */
    i = 0;
    while(--i){             /* fake USB disconnect for > 250 ms */
        wdt_reset();
        _delay_ms(1);
    }
    usbDeviceConnect();
    sei();
	
	// Read led count configuration
	ledCount = eeprom_read_byte(&eeLedCountMemory);
	if(ledCount == 0xff){
		ledCount = LED_DEFAULT_COUNT;
	}
	
	// Init leds
	currentLed = 0;
	LED_PORT_DDR |= 1 << LED_BIT;
	for (i=0; i<ledCount; i++) {
		setLedColor(i, 0, 30, 0);
	}
	writeToLeds();
	
	// Main loop
    for(;;){
        wdt_reset();
        usbPoll();
    }
}
