#include "stdint.h"
#include <SPI.h>
#include <a7105.h> 

//Pin used to activate recoding on my logic analyzer...
#define RECORD 4

#define RADIO_SELECT_PIN 9

A7105 radio;

// the setup function runs once when you press reset or power the board
void setup() {
  Serial.begin(9600);

  Serial.println("Starting...");

  pinMode(RADIO_SELECT_PIN, OUTPUT);
  
  //TODO: Can we nuke this safely?
  pinMode(MOSI,OUTPUT);
  pinMode(MISO,INPUT);
  pinMode(SCK,OUTPUT);
  pinMode(RECORD,OUTPUT);

  digitalWrite(RECORD,LOW);
  //digitalWrite(RADIO_SELECT_PIN,HIGH);
  //delay(5000);


  A7105_Initialize(&radio,RADIO_SELECT_PIN);
  Serial.println("Initialized...");

  A7105_WriteReg(&radio,A7105_0D_CLOCK,(byte)0x05);
  Serial.println("Clock written...");

  byte sanity_check = A7105_ReadReg(&radio,A7105_0D_CLOCK);
  Serial.print("Sanity check: ");
  Serial.println(sanity_check,HEX);

}

// the loop function runs over and over again forever
void loop() {
  delay(100000000);
  //Serial.print("RADIO_CLOCK register: ");
  //Serial.print(read_reg(RADIO_CLOCK),HEX);
  //Serial.println("");
  //digitalWrite(13, HIGH);   // turn the LED on (HIGH is the voltage level)
  //delay(100000000);              // wait for a second
  //digitalWrite(13, LOW);    // turn the LED off by making the voltage LOW
  //delay(100000000);              // wait for a second
}
