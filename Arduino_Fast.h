#ifndef __Arduino_Fast_h_
#define __Arduino_Fast_h_

#include <avr/io.h>
#include <util/delay.h>
#include <digitalWriteFast.h>

#define HIGH 0x1
#define LOW  0x0

#define INPUT 0x0
#define OUTPUT 0x1
#define INPUT_PULLUP 0x2

#define min(a,b) ((a)<(b)?(a):(b))
#define max(a,b) ((a)>(b)?(a):(b))

#define digitalRead(P) ( digitalReadFast((P)) )
#define pinMode(P,V) pinModeFast(P, V)
#define digitalWrite(P, V) digitalWriteFast(P, V)

#define delayMicroseconds(us) _delay_us(us)
#define delay(ms) _delay_ms(ms)

#endif