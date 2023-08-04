
#include <BLELite.h>
#include <NRF.h>

const uint8_t channel[3] = {37, 38, 39};  // logical BTLE channel number (37-39)
const uint8_t frequency[3] = {2, 26, 80}; // physical frequency (2400+x MHz)
uint8_t address[6] = { 0x11, 0x22, 0x33, 0x44, 0x55, 0x66}; // default address

// change buffer contents to "wire bit order"
void BLELite::swapbuf(uint8_t len)
{

	uint8_t *buf = (uint8_t *)&buffer;

    while (len--)
    {
        uint8_t a = *buf;
        uint8_t v = ((a & 0x01) << 7) |
                    ((a & 0x02) << 5) |
                    ((a & 0x04) << 3) |
                    ((a & 0x08) << 1) |
                    ((a & 0x10) >> 1) |
                    ((a & 0x20) >> 3) |
                    ((a & 0x40) >> 5) |
                    ((a & 0x80) >> 7);
        *(buf++) = v;
    }
}

// constructor
BLELite::BLELite(uint8_t ce_pin, uint8_t csn_pin)
{
	radio.init(ce_pin, csn_pin);
	current = 0;
}

// set BTLE-compatible radio parameters
void BLELite::begin(const char *_name)
{

	name = _name;
	// set standard parameters
	radio.setChannel(frequency[current]);
}

void BLELite::setAddress(const uint8_t *_address)
{
	 memcpy(address, _address, MAC_SIZE);
}

void BLELite::powerOff()
{
	radio.powerDown();
}

void BLELite::powerUp()
{
	radio.powerUp();
}

// hop to the next channel
void BLELite::hopChannel()
{
    current = (current + 1) % 3;
    radio.setChannel(frequency[current]);
}

// Broadcast an advertisement packet with optional payload
// Data type will be 0xFF (Manufacturer Specific Data)
bool BLELite::advertise(void *buf, uint8_t buflen)
{
	return advertise(0xFF, buf, buflen);
}

bool BLELite::addChunk(uint8_t chunk_type, const void *buf, uint8_t buflen)
{
	if (buffer.pl_size + buflen + 2 > MAX_PAYLOAD_SIZE + MAC_SIZE) // (buflen+2) is how much this chunk will take, 21 is payload size without crc and 6 is MAC size
		return false;
	ble_pdu_chunk *chunk = reinterpret_cast<ble_pdu_chunk *>(buffer.payload + buffer.pl_size - MAC_SIZE);
	chunk->type = chunk_type;
	
	for (uint8_t i = 0; i < buflen; i++)
		chunk->data[i] = ((uint8_t *)buf)[i];
	chunk->size = buflen + 1;
	buffer.pl_size += buflen + 2;
	return true;
}

uint8_t BLELite::preparePacket()
{

	// insert pseudo-random MAC address
	memcpy(buffer.mac, address, MAC_SIZE);	

	buffer.pdu_type = 0x42; // PDU type: ADV_NONCONN_IND, TX address is random
	buffer.pl_size = 6;		//including MAC

	// add device descriptor chunk
	uint8_t flags = 0x05;
	addChunk(0x01, &flags, 1);

	// add "complete name" chunk
	uint8_t nameLength = strlen(name);
	if (nameLength > 0)
		addChunk(0x09, name, nameLength);
	
	return buffer.pl_size;
}

void BLELite::transmitPacket()
{
	
	uint8_t pls = buffer.pl_size - 6;
	// calculate CRC over header+MAC+payload, append after payload
	uint8_t *outbuf = (uint8_t *)&buffer;
	crc(pls + 8, outbuf + pls + 8);

	// whiten header+MAC+payload+CRC, swap bit order
	whiten(pls + 11);
	swapbuf(pls + 11);
	radio.clearFlags();	
	radio.write(outbuf, pls + 11);
}

// Broadcast an advertisement packet with a specific data type
// Standardized data types can be seen here:
// https://www.bluetooth.org/en-us/specification/assigned-numbers/generic-access-profile
bool BLELite::advertise(uint8_t data_type, void *buf, uint8_t buflen)
{
	preparePacket();

	// add custom data, if applicable
	if (buflen > 0)
	{
		bool success = addChunk(data_type, buf, buflen);
		if (!success)
		{
			return false;
		}
	}

	transmitPacket();
	return true;
}

// see BT Core Spec 4.0, Section 6.B.3.2
void BLELite::whiten(uint8_t len)
{

	uint8_t *buf = (uint8_t *)&buffer;

	// initialize LFSR with current channel, set bit 6
	uint8_t lfsr = channel[current] | 0x40;

	while (len--)
	{
		uint8_t res = 0;
		// LFSR in "wire bit order"
		for (uint8_t i = 1; i; i <<= 1)
		{
			if (lfsr & 0x01)
			{
				lfsr ^= 0x88;
				res |= i;
			}
			lfsr >>= 1;
		}
		*(buf++) ^= res;
	}
}

// see BT Core Spec 4.0, Section 6.B.3.1.1
void BLELite::crc(uint8_t len, uint8_t *dst)
{

	uint8_t *buf = (uint8_t *)&buffer;

	// initialize 24-bit shift register in "wire bit order"
	// dst[0] = bits 23-16, dst[1] = bits 15-8, dst[2] = bits 7-0
	dst[0] = 0xAA;
	dst[1] = 0xAA;
	dst[2] = 0xAA;

	while (len--)
	{

		uint8_t d = *(buf++);

		for (uint8_t i = 1; i; i <<= 1, d >>= 1)
		{

			// save bit 23 (highest-value), left-shift the entire register by one
			uint8_t t = dst[0] & 0x01;
			dst[0] >>= 1;
			if (dst[1] & 0x01)
				dst[0] |= 0x80;
			dst[1] >>= 1;
			if (dst[2] & 0x01)
				dst[1] |= 0x80;
			dst[2] >>= 1;

			// if the bit just shifted out (former bit 23) and the incoming data
			// bit are not equal (i.e. bit_out ^ bit_in == 1) => toggle tap bits
			if (t != (d & 1))
			{
				// toggle register tap bits (=XOR with 1) according to CRC polynom
				dst[2] ^= 0xDA; // 0b11011010 inv. = 0b01011011 ^= x^6+x^4+x^3+x+1
				dst[1] ^= 0x60; // 0b01100000 inv. = 0b00000110 ^= x^10+x^9
			}
		}
	}
}
