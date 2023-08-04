#ifndef _BLELite_h
#define _BLELite_h

#if defined(__AVR_ATtiny45__) || defined(__AVR_ATtiny85__) || defined(__AVR_ATtiny44__) || defined(__AVR_ATtiny84__)
#include <Arduino_Fast.h>
#include <string.h>
#else
#include <Arduino.h>
#endif 


#include <NRF.h>

// float as used on the nRF8001 and nRF51822 platforms
// and on the "nRF Master Control Panel" and "nRF Temp 2.0" apps.
// This float representation has the first 8 bits as a base-10 exponent
// and the last 24 bits as the mantissa.

#define MAX_PAYLOAD_SIZE 21
#define MAC_SIZE 6


// Service UUIDs used on the nRF8001 and nRF51822 platforms
#define NRF_TEMPERATURE_SERVICE_UUID		0x1809
#define NRF_BATTERY_SERVICE_UUID			0x180F
#define NRF_DEVICE_INFORMATION_SERVICE_UUID 0x180A


// helper struct for sending temperature as BT service data
struct nrf_service_data {
	int16_t   service_uuid;
	int32_t value;
};
// advertisement PDU
struct ble_adv_pdu {

	// packet header
	uint8_t pdu_type; // PDU type
	uint8_t pl_size;  // payload size

	// MAC address
	uint8_t mac[6];

	// payload (including 3 bytes for CRC)
	uint8_t payload[24];
};

// payload chunk in advertisement PDU payload
struct ble_pdu_chunk {
	uint8_t size;
	uint8_t type;
	uint8_t data[];
};



class BLELite {

	public:

		BLELite(uint8_t ce_pin, uint8_t csn_pin);

		void setAddress(const uint8_t *_address);
		void begin( const char* _name ); // set BTLE-compatible radio parameters & name
		
		void hopChannel();              // hop to the next channel

		// Broadcast an advertisement packet with a specific data type
		// Standardized data types can be seen here: 
		// https://www.bluetooth.org/en-us/specification/assigned-numbers/generic-access-profile
		bool advertise( uint8_t data_type, void* buf, uint8_t len ); 

		// Broadcast an advertisement packet with optional payload
		// Data type will be 0xFF (Manufacturer Specific Data)
		bool advertise( void* buf, uint8_t len ); 
		
		ble_adv_pdu buffer;  // buffer for received BTLE packet (also used for outgoing!)
		uint8_t preparePacket();
		bool addChunk(uint8_t chunk_type, const void* buf, uint8_t buflen);
		void transmitPacket();

		void powerOff();
		void powerUp();

	private:

		void whiten( uint8_t len );
		void swapbuf( uint8_t len );
		void crc( uint8_t len, uint8_t* dst );
        NRF radio;
		uint8_t current;   // current channel index
    	const char* name;  // name of local device		
};

#endif