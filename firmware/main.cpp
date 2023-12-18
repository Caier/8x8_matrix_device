extern "C" {
    #include <avr/io.h>
    #include <avr/wdt.h>
    #include <avr/interrupt.h> 
    #include <util/delay.h>    
    #include <avr/pgmspace.h>  
    #include "usbdrv/usbdrv.h"
    #include "usbdrv/oddebug.h"
}

#include "screen.hpp"

#define LED_PORT_DDR        DDRB
#define LED_PORT_OUTPUT     PORTB
#define LED_BIT             5

static Screen screen;

static volatile uint64_t state = screen.state;
static uint8_t remainingBytes = 0;
static uint64_t tempScreen = 0;

usbMsgLen_t usbFunctionSetup(uchar data[8]) {
    usbRequest_t    *rq = (usbRequest_t*)data;
    
    static uchar    dataBuffer[4];
    if(rq->bRequest == 0) {
        dataBuffer[0] = rq->wValue.bytes[0];
        dataBuffer[1] = rq->wValue.bytes[1];
        dataBuffer[2] = rq->wIndex.bytes[0];
        dataBuffer[3] = rq->wIndex.bytes[1];
        usbMsgPtr = (intptr_t)dataBuffer;
        return 4;
    } else if(rq->bRequest == MATRIX_IMAGE_RQ) {
        remainingBytes = rq->wLength.bytes[0];
        return USB_NO_MSG;
    } else if(rq->bRequest == MATRIX_ENABLE_RQ) {
        screen.enabled = (rq->wValue.word & 1);
    }

    return 0;
}

uchar usbFunctionWrite(uchar* data, uchar len) {
    if(remainingBytes > 0) {
        for(uchar i = 0; i < len; i++) {
            *(((uchar*)&tempScreen) + remainingBytes - 1) = data[i];
            remainingBytes--;
        }
    }

    if(remainingBytes == 0)
        state = tempScreen;

    return remainingBytes == 0;
}

int __attribute__((noreturn)) main(void) {
    uchar   i;

    wdt_enable(WDTO_1S);
    usbInit();
    usbDeviceDisconnect();
    i = 0;
    while(--i) {
        wdt_reset();
        _delay_ms(1);
    }
    usbDeviceConnect();
    sei();
    for(;;){
        wdt_reset();
        screen.render();
        screen.state = state;
    }
}