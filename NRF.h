/* arduino wiring 
            d12
    d11     d13
    d10     d9
    3v3     gnd

    attiny wiring 

    10      7
    3v3     gnd
*/



#ifndef _NRF_h_
#define _NRF_h_

#if defined(__AVR_ATtiny45__) || defined(__AVR_ATtiny85__) || defined(__AVR_ATtiny44__) || defined(__AVR_ATtiny84__)
#include <Arduino_Fast.h>
#include <stddef.h>
#include <avr/interrupt.h>
#else
#include <Arduino.h>
#endif

#include <nFR24L01.h>


#if defined(__AVR_ATtiny44__) || defined(__AVR_ATtiny84__)
#define USI_DI 6  // PA6
#define USI_DO 5  // PA5
#define USI_SCK 4 // PA4
#elif defined(__AVR_ATtiny45__) || defined(__AVR_ATtiny85__)
#define USI_DI 0  // PB0
#define USI_DO 1  // PB1
#define USI_SCK 2 // PB2
#else
#include <SPI.h> // Use the normal Arduino hardware SPI library.
#endif

#define MAX_CHANNEL 125

class NRF {
    public:
    NRF() {}

    void init(uint8_t cePin, uint8_t csnPin);    
    void powerDown();
    void powerUp();

    void setChannel(uint8_t channel);
    void write(void*, uint8_t);
    void clearFlags();

private:

    const static uint32_t NRF_SPICLOCK = 4000000; // Speed to use for SPI communication with the transceiver.   
    uint8_t _cePin, _csnPin;
    uint8_t _currentConfig;
    
    void writeRegister(uint8_t regName, uint8_t data);
    void writeRegister(uint8_t regName, void* data, uint8_t length);    
    void spiTransfer(uint8_t regName, void* data, uint8_t length);
#if defined(__AVR_ATtiny45__) || defined(__AVR_ATtiny85__) || defined(__AVR_ATtiny44__) || defined(__AVR_ATtiny84__)
    uint8_t usiTransfer(uint8_t data);
#endif
 
};
#endif 