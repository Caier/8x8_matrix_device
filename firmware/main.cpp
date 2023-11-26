extern "C" {
    #include <avr/io.h>
    #include <avr/wdt.h>
    #include <avr/interrupt.h> 
    #include <util/delay.h>    
    #include <avr/pgmspace.h>  
    #include "usbdrv/usbdrv.h"
    #include "usbdrv/oddebug.h"
}

#define LED_PORT_DDR        DDRB
#define LED_PORT_OUTPUT     PORTB
#define LED_BIT             5

usbMsgLen_t usbFunctionSetup(uchar data[8]) {
    usbRequest_t    *rq = (usbRequest_t*)data;
    static uchar    dataBuffer[4];  /* buffer must stay valid when usbFunctionSetup returns */

    if(rq->bRequest == 0) { /* echo -- used for reliability tests */
        dataBuffer[0] = rq->wValue.bytes[0];
        dataBuffer[1] = rq->wValue.bytes[1];
        dataBuffer[2] = rq->wIndex.bytes[0];
        dataBuffer[3] = rq->wIndex.bytes[1];
        usbMsgPtr = (intptr_t)dataBuffer;         /* tell the driver which data to return */
        return 4;
    } else if(rq->bRequest == 1) {
        if(rq->wValue.bytes[0] & 1) {    /* set LED */
            LED_PORT_OUTPUT |= _BV(LED_BIT);
        } else {                          /* clear LED */
            LED_PORT_OUTPUT &= ~_BV(LED_BIT);
        }
    }

    return 0;   /* default for not implemented requests: return no data back to host */
}

int __attribute__((noreturn)) main(void) {
    uchar   i;

    wdt_enable(WDTO_1S);
    odDebugInit();
    DBG1(0x00, 0, 0);       /* debug output: main starts */
    usbInit();
    usbDeviceDisconnect();  /* enforce re-enumeration, do this while interrupts are disabled! */
    i = 0;
    while(--i){             /* fake USB disconnect for > 250 ms */
        wdt_reset();
        _delay_ms(1);
    }
    usbDeviceConnect();
    LED_PORT_DDR |= _BV(LED_BIT);   /* make the LED bit an output */
    LED_PORT_OUTPUT |= _BV(LED_BIT);
    sei();
    DBG1(0x01, 0, 0);       /* debug output: main loop starts */
    for(;;){                /* main event loop */
        DBG1(0x02, 0, 0);   /* debug output: main loop iterates */
        wdt_reset();
        usbPoll();
    }
}