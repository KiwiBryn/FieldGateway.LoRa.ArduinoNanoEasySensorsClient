/*
  Copyright ® 2018 November devMobile Software, All Rights Reserved

  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
  PURPOSE.

  You can do what you want with this code, acknowledgment would be nice.

  http://www.devmobile.co.nz

*/
#include <stdlib.h>
#include <LoRa.h>
#include <sha204_library.h>
#include <TH02_dev.h>

//#define DEBUG
//#define DEBUG_TELEMETRY
//#define DEBUG_LORA

// LoRa field gateway configuration (these settings must match your field gateway)
const char FieldGatewayAddress[] = {"LoRaIoT1"};
const float FieldGatewayFrequency =  915000000.0;
const byte FieldGatewaySyncWord = 0x12 ;

// Payload configuration
const int ChipSelectPin = 10;
const int ResetPin = 9;

// LoRa radio payload configuration
const byte SensorIdValueSeperator = ' ' ;
const byte SensorReadingSeperator = ',' ;
const int LoopSleepDelaySeconds = 60 ;

// ATSHA204 secure authentication, validation with crypto and hashing (currently only using for unique serial number)
const byte Atsha204Port = A3;
atsha204Class sha204(Atsha204Port);
const byte DeviceSerialNumberLength = 9 ;
byte deviceSerialNumber[DeviceSerialNumberLength] = {""};

const byte PayloadSizeMaximum = 64 ;
byte payload[PayloadSizeMaximum];
byte payloadLength = 0 ;


void setup()
{
  Serial.begin(9600);

#if DEBUG
  while (!Serial);
#endif
 
  Serial.println("Setup called");

  Serial.print("Field gateway:");
  Serial.print( FieldGatewayAddress ) ;
  Serial.print(" Frequency:");
  Serial.print( FieldGatewayFrequency,0 ) ;
  Serial.print("MHz SyncWord:");
  Serial.print( FieldGatewaySyncWord ) ;
  Serial.println();
  
   // Retrieve the serial number then display it nicely
  if(sha204.getSerialNumber(deviceSerialNumber))
  {
    Serial.println("sha204.getSerialNumber failed");
    while (true); // Drop into endless loop requiring restart
  }

  Serial.print("SNo:");
  for (int i = 0; i < sizeof( deviceSerialNumber) ; i++)
  {
    // Add a leading zero
    if ( deviceSerialNumber[i] < 16)
    {
      Serial.print("0");
    }
    Serial.print(deviceSerialNumber[i], HEX);
    Serial.print(" ");
  }
  Serial.println();

  Serial.println("LoRa setup start");

  // override the default chip select and reset pins
  LoRa.setPins(ChipSelectPin, ResetPin);
  if (!LoRa.begin(FieldGatewayFrequency))
  {
    Serial.println("LoRa begin failed");
    while (true); // Drop into endless loop requiring restart
  }

  // Need to do this so field gateway pays attention to messsages from this device
  LoRa.enableCrc();
  LoRa.setSyncWord(FieldGatewaySyncWord);

#ifdef DEBUG_LORA
  LoRa.dumpRegisters(Serial);
#endif
  Serial.println("LoRa Setup done.");

  // Configure the Seeedstudio TH02 temperature & humidity sensor
  Serial.println("TH02 setup start");
  TH02.begin();
  delay(100);
  Serial.println("TH02 setup done");

  PayloadHeader(FieldGatewayAddress,strlen(FieldGatewayAddress), deviceSerialNumber, DeviceSerialNumberLength);

  Serial.println("Setup done");
  Serial.println();
}


void loop()
{
  float temperature ;
  float humidity ;

  Serial.println("Loop called");

  PayloadReset();

  // Read the temperature & humidity values then display nicely
  temperature = TH02.ReadTemperature();
  Serial.print("T:");
  Serial.print(temperature, 1) ;
  Serial.println("C ") ;

  PayloadAdd( "T", temperature, 1);

  humidity = TH02.ReadHumidity();
  Serial.print("H:" );
  Serial.print(humidity, 0) ;
  Serial.println("% ") ;

  PayloadAdd( "H", humidity, 0) ;

#ifdef DEBUG_TELEMETRY
  Serial.println();
  Serial.print("RFM9X/SX127X Payload length:");
  Serial.print(payloadLength);
  Serial.println(" bytes");
#endif

  LoRa.beginPacket();
  LoRa.write(payload, payloadLength);
  LoRa.endPacket();

  Serial.println("Loop done");
  Serial.println();
  delay(LoopSleepDelaySeconds * 1000l);
}


void PayloadHeader(byte *to, byte toAddressLength, byte *from, byte fromAddressLength)
{
  byte addressesLength = toAddressLength + fromAddressLength ;

  payloadLength = 0 ;

  // prepare the payload header with "To" Address length (top nibble) and "From" address length (bottom nibble)
  
  payload[payloadLength] = (toAddressLength << 4) | fromAddressLength ;
  payloadLength += 1;

  // Copy the "To" address into payload
  memcpy(&payload[payloadLength], to, toAddressLength);
  payloadLength += toAddressLength ;

  // Copy the "From" into payload
  memcpy(&payload[payloadLength], from, fromAddressLength);
  payloadLength += fromAddressLength ;
}


void PayloadAdd( char *sensorId, float value, byte decimalPlaces)
{
  byte sensorIdLength = strlen( sensorId ) ;

  memcpy( &payload[payloadLength], sensorId,  sensorIdLength) ;
  payloadLength += sensorIdLength ;
  payload[ payloadLength] = SensorIdValueSeperator;
  payloadLength += 1 ;
  payloadLength += strlen( dtostrf(value, -1, decimalPlaces, (char *)&payload[payloadLength]));
  payload[ payloadLength] = SensorReadingSeperator;
  payloadLength += 1 ;
  
#ifdef DEBUG_TELEMETRY
  Serial.print("PayloadAdd float-payloadLength:");
  Serial.print( payloadLength);
  Serial.println( );
#endif
}


void PayloadAdd( char *sensorId, int value )
{
  byte sensorIdLength = strlen(sensorId) ;

  memcpy(&payload[payloadLength], sensorId,  sensorIdLength) ;
  payloadLength += sensorIdLength ;
  payload[ payloadLength] = SensorIdValueSeperator;
  payloadLength += 1 ;
  payloadLength += strlen(itoa( value,(char *)&payload[payloadLength],10));
  payload[ payloadLength] = SensorReadingSeperator;
  payloadLength += 1 ;
  
#ifdef DEBUG_TELEMETRY
  Serial.print("PayloadAdd int-payloadLength:" );
  Serial.print(payloadLength);
  Serial.println( );
#endif
}


void PayloadAdd( char *sensorId, unsigned int value )
{
  byte sensorIdLength = strlen(sensorId) ;

  memcpy(&payload[payloadLength], sensorId,  sensorIdLength) ;
  payloadLength += sensorIdLength ;
  payload[ payloadLength] = SensorIdValueSeperator;
  payloadLength += 1 ;
  payloadLength += strlen(utoa( value,(char *)&payload[payloadLength],10));
  payload[ payloadLength] = SensorReadingSeperator;
  payloadLength += 1 ;

#ifdef DEBUG_TELEMETRY
  Serial.print("PayloadAdd uint-payloadLength:");
  Serial.print(payloadLength);
  Serial.println( );
#endif
}


void PayloadReset()
{
  byte fromAddressLength = payload[0] & 0xf ;
  byte toAddressLength = payload[0] >> 4 ;
  
  payloadLength = toAddressLength + fromAddressLength + 1;
}
