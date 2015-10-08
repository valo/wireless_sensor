/*
 Copyright (C) 2015 Valentin Mihov <valentin.mihov@gmail.com>

 This program is free software; you can redistribute it and/or
 modify it under the terms of the MIT license.
 */

/**
 * This is a program, which reads measurements from a DHT11
 * sensor, encrypts them using an AES128 cypher and transmits them
 * using an nRF24l01 radio.
 */

#include <SPI.h>
#include "nRF24L01.h"
#include "RF24.h"
#include "printf.h"
#include "aes_key.h"
#include <AESLib.h>
#include "DHT.h"

#define DHTPIN 3     // what pin we're connected to
#define DHTTYPE DHT11   // DHT 11

uint8_t data[16];

typedef enum { TEMPERATURE = 1, RELAY } message_type_t;

//
// Hardware configuration
//

// Initialize DHT sensor.
// Note that older versions of this library took an optional third parameter to
// tweak the timings for faster processors.  This parameter is no longer needed
// as the current DHT reading algorithm adjusts itself to work on faster procs.
DHT dht(DHTPIN, DHTTYPE);

// Set up nRF24L01 radio on SPI bus plus pins 9 & 10

RF24 radio(9,10);

//
// Topology
//

// Radio pipe addresses for the 2 nodes to communicate.
const uint64_t sensor_address = 0xF0F0F0F0E1LL;

void setup(void)
{
  //
  // Print preamble
  //

  Serial.begin(57600);
  printf_begin();

  printf("Setting up the DHT sensor\n");
  dht.begin();

  //
  // Setup and configure rf radio
  //

  printf("Setting up the nRF24L01 radio\n");

  radio.begin();                          // Start up the radio
  radio.setChannel(0x4e);
  radio.setAutoAck(false);   // Ensure autoACK is disabled
  radio.enableDynamicPayloads();

  //
  // Open write pipe to other nodes for communication
  //
  radio.openWritingPipe(sensor_address);

  //
  // Dump the configuration of the rf unit for debugging
  //

  radio.printDetails();                   // Dump the configuration of the rf unit for debugging
}

void loop(void)
{
  float h = dht.readHumidity();
  float t = dht.readTemperature();
  // Check if any reads failed and exit early (to try again).
  if (isnan(h) || isnan(t)) {
    Serial.println("Failed to read from DHT sensor!");
    delay(5000);
    return;
  }
  // Compute heat index in Fahrenheit (the default)
  float hif = dht.computeHeatIndex(t, h, false);

  Serial.print("Humidity: ");
  Serial.println(h);
  Serial.print("Temperature: ");
  Serial.println(t);

  // Take the temperature and send it
  memset(data, 0, sizeof(data));
  data[0] = TEMPERATURE;
  memcpy(data + 1, &h, sizeof(h));
  memcpy(data + 1 + sizeof(h), &t, sizeof(t));
  aes128_enc_single(key, data);

  Serial.println("Sending:");
  for (int i = 0;i < sizeof(data); i++) {
    Serial.print("\\x");
    Serial.print(data[i], HEX);
  }
  Serial.println("");

  bool ok = radio.write(data, sizeof(data) );

  if (ok)
    Serial.println("ok...");
  else
    Serial.println("failed.");


  delay(10000);
}
// vim:cin:ai:sts=2 sw=2 ft=cpp
