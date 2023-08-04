# Bluetooth library for arduino / attiny boards
This library have limited dependencies especially when using a ATTiny. 
Based on work of :
 - Florian Etcher (https://github.com/floe/BTLE)
 - Dmitry Grinberg (https://dmitry.gr/index.php?r=05.Projects&proj=11.%20Bluetooth%20LE%20fakery)

Tested with arduino NANO board, and Attiny 44 

This is a work in progress, and some features are not available yet. 

Feel free to comment / contribute

simple example 

```C
#define CE_PIN 9
#define CSN_PIN 10

BLELite btle(CE_PIN, CSN_PIN);
nrf_service_data buf;

void setup() {
  Serial.begin(9600);
  btle.begin("name");  
}

void loop() { 
  buf.service_uuid = NRF_TEMPERATURE_SERVICE_UUID;  //0x1809
  buf.value = 10;
  
  // just advertise 
  btle.advertise(&buf, sizeof(buf));
  // OR 
  // manually build packet 
  Serial.println(btle.preparePacket());
  btle.addChunk( 0x16,&b,sizeof(b) )
   btle.transmitPacket();

   // change channel 
   btle.hopChannel();
  
  
}
```
