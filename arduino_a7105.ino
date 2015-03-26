#include "stdint.h"

//#include <PinChangeInt.h>

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

#define CALIBRATION_TIMEOUT 500 //milliseconds before we call autocalibrating a failure

A7105 radio1;
A7105 radio2;


// the setup function runs once when you press reset or power the board
void setup() {
  Serial.begin(9600);

  Serial.println("Starting...");

  //pinMode(RADIO_SELECT_PIN, OUTPUT);
  
  //TODO: Can we nuke this safely?
  pinMode(MOSI,OUTPUT);
  pinMode(MISO,INPUT);
  pinMode(SCK,OUTPUT);
  pinMode(RECORD,OUTPUT);

  digitalWrite(RECORD,LOW);

  //delay(2000);


  A7105_Status_Code success = A7105_Easy_Setup_Radio(&radio1,RADIO1_SELECT_PIN, RADIO1_WTR_PIN, RADIO_IDS, A7105_DATA_RATE_100Kbps,0,A7105_TXPOWER_100mW);
  A7105_Status_Code success2 = A7105_Easy_Setup_Radio(&radio2,RADIO2_SELECT_PIN, RADIO2_WTR_PIN, RADIO_IDS, A7105_DATA_RATE_100Kbps,0,A7105_TXPOWER_100mW);
  Serial.print("Radio 1 Init: ");
  Serial.println(success == A7105_STATUS_OK);
  Serial.print("Radio 2 Init: ");
  Serial.println(success2 == A7105_STATUS_OK);


}

void send_uint32_packet(struct A7105* radio, uint32_t val)
{
  byte data[8];
  memset(data,0,8);
  data[0] = (val >> 24) & 0xff;
  data[1] = (val >> 16) & 0xff;
  data[2] = (val >> 8) & 0xff;
  data[3] = (val & 0xff);
  A7105_WriteData(radio,data,8);
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


// the loop function runs over and over again forever
void loop() {

  //Put radio2 in RX mode so it will hear the packet we're sending with radio1
  //A7105_Strobe(&radio2,A7105_RX);
  A7105_Easy_Listen_For_Packets(&radio2,8);

  //Write Data from radio 1 to radio 2 (just write the 4-byte long of the millis() counter)
  uint32_t val = millis();
  digitalWrite(RECORD,HIGH);
  send_uint32_packet(&radio1,val);
  digitalWrite(RECORD,LOW);
  Serial.print("Sent from radio1: ");
  Serial.println(val);

  Serial.print("RX ready1: ");
  Serial.println(A7105_CheckRXWaiting(&radio2));
   Serial.print("TX ready1: ");
  Serial.println(A7105_CheckRXWaiting(&radio1));
  //wait for the packet to transmit (since we don't have interrupts working yet)
  delay(100);
  Serial.print("RX ready2: ");
  Serial.println(A7105_CheckRXWaiting(&radio2));
  Serial.print("TX ready2: ");
  Serial.println(A7105_CheckRXWaiting(&radio1));

  //Read the packet from radio2 (hopefully it's there)
  val = 0;
  recv_uint32_packet(&radio2,&val);
  Serial.print("Radio2's fifo had ");
  Serial.println(val);

  Serial.println("done...");
  delay(5000);
  //Serial.print("RADIO_CLOCK register: ");
  //Serial.print(read_reg(RADIO_CLOCK),HEX);
  //Serial.println("");
  //digitalWrite(13, HIGH);   // turn the LED on (HIGH is the voltage level)
  //delay(100000000);              // wait for a second
  //digitalWrite(13, LOW);    // turn the LED off by making the voltage LOW
  //delay(100000000);              // wait for a second
}
