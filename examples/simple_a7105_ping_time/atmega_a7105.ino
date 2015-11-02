/*Copyright (C) 2015 Matthew Meno

This file is part of ATmega A7105.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:

(1) Redistributions of source code must retain the above copyright
notice, this list of conditions and the following disclaimer. 

(2) Redistributions in binary form must reproduce the above copyright
notice, this list of conditions and the following disclaimer in
the documentation and/or other materials provided with the
distribution.  

(3)The name of the author may not be used to
endorse or promote products derived from this software without
specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
POSSIBILITY OF SUCH DAMAGE.
*/

#include "stdint.h"
#include <SPI.h>
#include <a7105.h> 


/*
This script is a test harness for the 
a7105 library (just the physical bits)

It sets up radios on a specific channel
and bounces data between them using a single
arduino (pro mini in the original case).


TODO: 

  * Update A7105 library to include status errors and such
    for the relevant functions as well as implement (optional)
    pin interrupt notifications for data being ready

Then, after we successfully pass data, add 
an interrupt to notify us of pending data to read.

After everything works, we can start to library-ify the
results into the mesh library.

*/


//Pin used to activate recoding on my logic analyzer...
#define RECORD 4

#define RADIO1_SELECT_PIN 9
#define RADIO1_WTR_PIN 6

#define RADIO2_SELECT_PIN 8
#define RADIO2_WTR_PIN 5

#define RADIO_IDS 0xdb042679 //taken from devation code

#define PING_TIMEOUT 1500 //time we wait between starting a TX and receiving notification of RX

A7105 radio1;
A7105 radio2;

// the setup function runs once when you press reset or power the board
void setup() {
  Serial.begin(115200);

  Serial.println("Starting...");

  //TODO: Can we nuke this safely?
  pinMode(MOSI,OUTPUT);
  pinMode(MISO,INPUT);
  pinMode(SCK,OUTPUT);
  pinMode(RECORD,OUTPUT);

  digitalWrite(RECORD,LOW);


  A7105_Status_Code success = A7105_Easy_Setup_Radio(&radio1,RADIO1_SELECT_PIN, RADIO1_WTR_PIN, RADIO_IDS, A7105_DATA_RATE_250Kbps,0,A7105_TXPOWER_150mW,1,1);
  A7105_Status_Code success2 = A7105_Easy_Setup_Radio(&radio2,RADIO2_SELECT_PIN, RADIO2_WTR_PIN, RADIO_IDS, A7105_DATA_RATE_250Kbps,0,A7105_TXPOWER_150mW,1,1);

  Serial.print("Radio 1 Init: ");
  Serial.println(success == A7105_STATUS_OK);
  Serial.print("Radio 2 Init: ");
  Serial.println(success2 == A7105_STATUS_OK);


}

void send_uint32_packet(struct A7105* radio, uint32_t val)
{
  byte data[64];
  memset(data,0,64);
  data[0] = (val >> 24) & 0xff;
  data[1] = (val >> 16) & 0xff;
  data[2] = (val >> 8) & 0xff;
  data[3] = (val & 0xff);
  A7105_WriteData(radio,data,64);
}

void recv_uint32_packet(struct A7105* radio, uint32_t* val)
{
  byte data[4];
  A7105_ReadData(radio,data,4);
  *val = ((uint32_t)(data[0]) << 24) |
        ((uint32_t)(data[1]) << 16) |
        ((uint32_t)(data[2]) << 8) |
        (uint32_t)(data[3]);
        
}

//returns success or error
int bounce_packet(struct A7105* src, struct A7105* dest, uint32_t value)
{
  //Put dest in RX mode so it will hear the packet we're sending with src (8 byte packets since it's the smallest that will
  //fit a 4 byte uint32_t we're pinging around)
  A7105_Easy_Listen_For_Packets(dest,64);

  if (A7105_CheckRXWaiting(dest) == A7105_RX_DATA_WAITING)
  {
    Serial.println("int malfunction...");
  }

  //Write Data from radio 1 to radio 2 (just write the 4-byte long of the millis() counter)
  send_uint32_packet(src,value);
  Serial.print("Sent from Source Radio: ");
  Serial.println(value);

  uint32_t sent_time  = millis();
  uint32_t recv_val = 0;
  while (A7105_CheckRXWaiting(dest) != A7105_RX_DATA_WAITING &&
         millis() - sent_time < PING_TIMEOUT)
  {
    recv_val++;
    //delay(1);
  }

  
  if (A7105_CheckRXWaiting(dest) == A7105_RX_DATA_WAITING)
  {
    uint32_t received_time = millis();
    recv_uint32_packet(dest,&recv_val);
    Serial.print("Received data after ");
    Serial.print(received_time - sent_time);
    Serial.print("ms: ");
    Serial.println(recv_val);
    return 1;
  }
  else
  {
    Serial.print("Error waiting for RX data: ");
    if (millis() - sent_time >= PING_TIMEOUT)
      Serial.println("(timeout)");
    else
      Serial.println(A7105_CheckRXWaiting(dest)); 
  }
  return 0;
}

int success = 1;
// the loop function runs over and over again forever
void loop() {

  if (success) 
  {
    success = bounce_packet(&radio1,&radio2,millis());
    if (success)
    {
      Serial.println("Boucing a packet back...");
      success = bounce_packet(&radio2,&radio1,millis());
    }

    Serial.println("done...");
  }

  delay(5000);
}
