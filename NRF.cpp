#include <NRF.h>

/*
    // desactiver
    data &= ~_BV(MASK_RX_DR);
    // activer
    data |= _BV(MASK_RX_DR)
*/

void NRF::init(uint8_t cePin, uint8_t csnPin)
{
    _cePin = cePin;
    _csnPin = csnPin;
    uint8_t data;

    // Default states for the radio pins.  When CSN is LOW the radio listens to SPI communication,
    // so we operate most of the time with CSN HIGH.
    pinMode(_cePin, OUTPUT);
    pinMode(_csnPin, OUTPUT);
    digitalWrite(_csnPin, HIGH);

// Setup the microcontroller for SPI communication with the radio.
#if defined(__AVR_ATtiny45__) || defined(__AVR_ATtiny85__) || defined(__AVR_ATtiny44__) || defined(__AVR_ATtiny84__)
    pinMode(USI_DI, INPUT);
    digitalWrite(USI_DI, HIGH);
    pinMode(USI_DO, OUTPUT);
    digitalWrite(USI_DO, LOW);
    pinMode(USI_SCK, OUTPUT);
    digitalWrite(USI_SCK, LOW);
    USICR |= (1 << USIWM0) | (1 << USICS1) | (1 << USICLK);
#else

    // Arduino SPI makes SS (D10 on ATmega328) an output and sets it HIGH.  It must remain an output
    // for Master SPI operation, but in case it started as LOW, we'll set it back.
    uint8_t savedSS = digitalRead(SS);
    SPI.begin();
    if (_csnPin != SS)
        digitalWrite(SS, savedSS);

#endif

    // NRF init
    // Now initialize nRF24L01+, setting general parameters
    data =
        _BV(MASK_RX_DR) |  // IRQ interrupt on RX (0 = enabled)
        _BV(MASK_TX_DS) |  // IRQ interrupt on TX (0 = enabled)
        _BV(MASK_MAX_RT) | // IRQ interrupt on auto retransmit counter overflow (0 = enabled)
        _BV(PWR_UP) |      // Power up
        _BV(PRIM_RX);
    // disable CRC and
    data &= ~(_BV(EN_CRC) | _BV(CRC0));
    writeRegister(CONFIG, data); // on, no crc, int on RX/TX done
    _currentConfig = data;

    writeRegister(EN_AA, 0x00);      // no auto-acknowledge
    writeRegister(EN_RXADDR, 0x00);  // no RX
    writeRegister(SETUP_AW, 0x02);   // 4-byte address
    writeRegister(SETUP_RETR, 0x00); // no auto-retransmit
    data = RF_DR_1MBPS | POWER_MAX;

    writeRegister(RF_SETUP, data);  // 1MBps at 0dBm
    writeRegister(STATUS, 0x3E);    // clear various flags
    writeRegister(DYNPD, 0x00);     // no dynamic payloads
    writeRegister(FEATURE, 0x00);   // no features
    writeRegister(RX_PW_P0, 32);    // always RX 32 bytes
    writeRegister(EN_RXADDR, 0x01); // RX on pipe 0

    // Set access addresses (TX address in nRF24L01) to BLE advertising 0x8E89BED6
    // Remember that both bit and byte orders are reversed for BLE packet format
    uint8_t buf[4] = {0x71, 0x91, 0x7D, 0x6B};
    writeRegister(TX_ADDR, buf, 4);
    // set RX address in nRF24L01, doesn't matter because RX is ignored in this case
    writeRegister(RX_ADDR_P0, buf, 4);
}

void NRF::clearFlags()
{
    writeRegister(STATUS, 0x3E); // clear various flags
}

void NRF::setChannel(uint8_t channel)
{
    writeRegister(RF_CH, min(channel, MAX_CHANNEL));
}

void NRF::powerDown()
{
    // Turn off the radio.
    _currentConfig &= ~_BV(PWR_UP);
    writeRegister(CONFIG, _currentConfig);
}

void NRF::powerUp()
{
    // 0001 0010
    _currentConfig |= _BV(PWR_UP);
    writeRegister(CONFIG, _currentConfig);
}

void NRF::write(void *data, uint8_t len)
{
    spiTransfer(FLUSH_RX, NULL, 0); // Clear RX Fifo
    spiTransfer(FLUSH_TX, NULL, 0); // Clear TX Fifo

    spiTransfer(W_TX_PAYLOAD, data, len);

    _currentConfig |= _BV(PWR_UP);
    _currentConfig &= ~_BV(PRIM_RX);
    writeRegister(CONFIG, _currentConfig);

    digitalWrite(_cePin, HIGH);
    delay(10);
    digitalWrite(_cePin, LOW);
}

void NRF::writeRegister(uint8_t reg, uint8_t data)
{
    writeRegister(reg, &data, 1);
}

void NRF::writeRegister(uint8_t reg, void *data, uint8_t length)
{
    spiTransfer((W_REGISTER | (REGISTER_MASK & reg)), data, length);
}

void NRF::spiTransfer(uint8_t regName, void *data, uint8_t length)
{
    cli();

    uint8_t *intData = reinterpret_cast<uint8_t *>(data);

    digitalWrite(_csnPin, LOW); // Signal radio to listen to the SPI bus.
    delayMicroseconds(10);
#if defined(__AVR_ATtiny45__) || defined(__AVR_ATtiny85__) || defined(__AVR_ATtiny44__) || defined(__AVR_ATtiny84__)

    // ATtiny transfer with USI.
    usiTransfer(regName);
    for (uint8_t i = 0; i < length; ++i)
    {
        usiTransfer(intData[i]);
        delayMicroseconds(10);
    }

#else
    // Transfer with the Arduino SPI library.
    SPI.beginTransaction(SPISettings(NRF_SPICLOCK, MSBFIRST, SPI_MODE0));
    SPI.transfer(regName);
    for (uint8_t i = 0; i < length; ++i)
    {
        SPI.transfer(intData[i]);
        delayMicroseconds(10);
    }
    SPI.endTransaction();
#endif

    digitalWrite(_csnPin, HIGH); // Stop radio from listening to the SPI bus.
    sei();
}

#if defined(__AVR_ATtiny45__) || defined(__AVR_ATtiny85__) || defined(__AVR_ATtiny44__) || defined(__AVR_ATtiny84__)

uint8_t NRF::usiTransfer(uint8_t data)
{
    USIDR = data;
    USISR = _BV(USIOIF);

    while ((USISR & (1 << USIOIF)) == 0)
        USICR |= (1 << USITC);

    return USIDR;
}

#endif
