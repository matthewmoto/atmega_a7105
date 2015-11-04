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
#include <a7105_mesh.h> 


/*
  This code is a simple shield harness that serves 3 registers (named with this node's unique ID).
 
  The registers served are UNIQUE_ID_A/B/C, (where UNIQUE_ID is actually the ascii hex id of the node).

  The registers can be all be retrived with GET_REGISTER.
  Only A can be set.
  B will simply ignore updates to it and will return the time since startup in milliseconds (string format)
  C will return an error if anyone tries to set it.

  Hookup Guide:
  This sketch is designed for the A7105 Pro Mini shield (v0.1). 

  See the XL7105 SY schematic at references/schematic_images/thumb-XL7105_SY-PINOUT.jpg for the pin locations
  on the radio.

  The following connections should be made:
    SPI-BUS to the radio (these need to be fed through a 5v<->3.3v logic level convertor if you aren't using the shield):
      SCK: Pin 13 (Arduino IDE pin) -------> sck (radio pin) 
      MISO: Pin 12 (")              -------> gio1 (")
      MOSI: Pin 11 (")              -------> sdio (")
      CS:   Pin 7 (")               -------> scs  (") 

  Other connections:
      GND:  (Any GND)               --------> gnd (any GND on radio)
      WTR: Pin 8 (Arduino IDE pin)  --------> gio2 (radio pin)
        NOTE: This pin isn't required to go through a logic level convertor since it's output only on the radio 
              but definitely don't write to it if you go this route. The radio shield has a logic level convertor 
              here.
      VDD (3.3V): (3.3V source)     --------> vdd (radio pin)
*/


//Constants for the Pro Mini A7105 Radio Shield v0.2
#define RADIO1_SELECT_PIN 7
#define RADIO1_WTR_PIN 8
#define RADIO_IDS 0xdb042679 //The ID filter for the current mesh 

//Size of our ascii buffer for input output use
#define ASCII_BUFFER_SIZE 64

#define putstring(x) SerialPrint_P(PSTR(x))                             
void SerialPrint_P(PGM_P str) {                                         
  for (uint8_t c; (c = pgm_read_byte(str)); str++) Serial.write(c);     
} 

struct A7105_Mesh RADIO;
struct A7105_Mesh_Register regs[3];
struct A7105_Mesh_Register* A = &regs[0];
struct A7105_Mesh_Register* B = &regs[1];
struct A7105_Mesh_Register* C = &regs[2];

char ASCII_BUFFER[ASCII_BUFFER_SIZE];

int freeRam ()
{
  extern int __heap_start, *__brkval;
  int v; 
  return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval);
}

void uint16_to_str(uint16_t src,char* target)
{
  int index =0;
  for (int shift = 12; shift >=0;shift -= 4)
  {
    byte working = (src>>shift) & 0x0F;
    if (working < 10)
      target[index] = working+'0';
    else
      target[index] = working-10+'A';
    index++;
  }
  target[index] = '\0';
}


A7105_Mesh_Status b_set_register_called(A7105_Mesh_Register* reg, A7105_Mesh* node, void* c)
{
  putstring("B SET_REGISTER called\r\n");

  //Output the suggested new value (assume a string) to the serial terminal
  char tmp[64];
  A7105_Mesh_Util_GetRegisterValueStr(&(node->register_cache),tmp,64);
  putstring("Suggested value is: \"");
  Serial.print(tmp);
  putstring("\"\r\n");


  //Don't actually change the register
  return A7105_Mesh_STATUS_OK;
}

A7105_Mesh_Status c_set_register_called(A7105_Mesh_Register* reg, A7105_Mesh* node, void* c)
{
  putstring("C SET_REGISTER called\r\n");

  //DEBUG to see if errors propagate right
  A7105_Mesh_Register_Set_Error(node,"dunno, broke?");
  return A7105_Mesh_INVALID_REGISTER_VALUE;
}

A7105_Mesh_Status b_get_register_called(A7105_Mesh_Register* reg, void* c)
{
  putstring("GET_REGISTER called\r\n");
  return A7105_Mesh_STATUS_OK;
}

void init_radio()
{
  A7105_Mesh_Status success = A7105_Mesh_Initialize(&(RADIO),
                                                    RADIO1_SELECT_PIN,
                                                    RADIO1_WTR_PIN,
                                                    RADIO_IDS,
                                                    A7105_DATA_RATE_125Kbps,
                                                    0,
                                                    A7105_TXPOWER_150mW,
                                                    0);
  putstring("Radio 1 Init: ");
  Serial.print(success == A7105_Mesh_STATUS_OK);
  putstring(", ID = ");
  Serial.println(RADIO.unique_id,HEX);

  //Initialize our registers and get/set callbacks
  
  //A is a non managed GET/SET storage thing
  A7105_Mesh_Register_Initialize(A,NULL,NULL);
  uint16_to_str(RADIO.unique_id,ASCII_BUFFER);
  strcat(ASCII_BUFFER,"_A");
  A7105_Mesh_Util_SetRegisterNameStr(A,ASCII_BUFFER);
  A7105_Mesh_Util_SetRegisterValueStr(A,"A");
  

  //B is a managed GET/SET that refuses to change its value 
  A7105_Mesh_Register_Initialize(B,b_set_register_called,b_get_register_called);
  uint16_to_str(RADIO.unique_id,ASCII_BUFFER);
  strcat(ASCII_BUFFER,"_B");
  A7105_Mesh_Util_SetRegisterNameStr(B,ASCII_BUFFER);
  A7105_Mesh_Util_SetRegisterValueU32(B,666);


  //C is a managed GET/SET that returns an error to the calling node for SET 
  A7105_Mesh_Register_Initialize(C,c_set_register_called,NULL);
  uint16_to_str(RADIO.unique_id,ASCII_BUFFER);
  strcat(ASCII_BUFFER,"_C");
  A7105_Mesh_Util_SetRegisterNameStr(C,ASCII_BUFFER);
  A7105_Mesh_Util_SetRegisterValueStr(C,"C");

  //Register our regs with the radio node
  A7105_Mesh_Set_Node_Registers(&RADIO, regs, 3);
}

void setup() {
  Serial.begin(115200);

  //Serial.println("Starting...");

  pinMode(MOSI,OUTPUT);
  pinMode(MISO,INPUT);
  pinMode(SCK,OUTPUT);
 
  init_radio();
 
  A7105_Mesh_Status join_status = A7105_Mesh_Join(&(RADIO), NULL);
}


// the loop function runs over and over again forever
void loop() {
  //Let the RADIO have some time to push packets around
  A7105_Mesh_Update(&(RADIO));
}
