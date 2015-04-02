#include "stdint.h"
#include <SPI.h>
#include <a7105.h> 
#include <a7105_mesh.h> 


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

void join_finished(struct A7105_Mesh* node, A7105_Mesh_Status status);

A7105_Mesh radio1;
A7105_Mesh radio2;

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

  A7105_Mesh_Status success = A7105_Mesh_Initialize(&radio1,
                                                    RADIO1_SELECT_PIN,
                                                    RADIO1_WTR_PIN,
                                                    RADIO_IDS,
                                                    A7105_DATA_RATE_250Kbps,
                                                    0,
                                                    A7105_TXPOWER_150mW,
                                                    0);
  A7105_Mesh_Status success2 = A7105_Mesh_Initialize(&radio2,
                                                     RADIO2_SELECT_PIN,
                                                     RADIO2_WTR_PIN,
                                                     RADIO_IDS,
                                                     A7105_DATA_RATE_250Kbps,
                                                     0,
                                                     A7105_TXPOWER_150mW,
                                                     0);
  //A7105_Status_Code success = A7105_Easy_Setup_Radio(&radio1,RADIO1_SELECT_PIN, RADIO1_WTR_PIN, RADIO_IDS, A7105_DATA_RATE_250Kbps,0,A7105_TXPOWER_150mW,1,1);
  //A7105_Status_Code success2 = A7105_Easy_Setup_Radio(&radio2,RADIO2_SELECT_PIN, RADIO2_WTR_PIN, RADIO_IDS, A7105_DATA_RATE_250Kbps,0,A7105_TXPOWER_150mW,1,1);

  Serial.print("Radio 1 Init: ");
  Serial.print(success == A7105_Mesh_STATUS_OK);
  Serial.print(", ID = ");
  Serial.println(radio1.unique_id,HEX);
  
  Serial.print("Radio 2 Init: ");
  Serial.print(success2 == A7105_Mesh_STATUS_OK);
  Serial.print(", ID = ");
  Serial.println(radio2.unique_id,HEX);


  //Try to make radio1 join the Mesh
  A7105_Mesh_Join(&radio1, join_finished);
}

int freeRam () 
{
  extern int __heap_start, *__brkval; 
  int v; 
  return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval); 
}

void join_finished(struct A7105_Mesh* node, A7105_Mesh_Status status)
{
  Serial.print("Node ");
  Serial.print(node->unique_id,HEX);
  Serial.print(" finished JOIN as ");
  Serial.print(node->node_id);
  Serial.print(" Status: ");
  Serial.println(status);
}

void get_num_registers_finished(struct A7105_Mesh* node, A7105_Mesh_Status status)
{
  Serial.print("Node ");
  Serial.print(node->unique_id,HEX);
  Serial.print(" finished GET_NUM_REGISTERS");
  Serial.print(" Status: ");
  Serial.println(status);

  if (status == A7105_Mesh_STATUS_OK)
  {
    Serial.print("Response from node: ");
    Serial.print(node->responder_unique_id,HEX);
    Serial.print(" (");
    Serial.print(node->responder_node_id);
    Serial.print("): ");
    Serial.println(node->num_registers_cache);
  }
}

void ping_finished(struct A7105_Mesh* node, A7105_Mesh_Status status)
{
  Serial.print("Node ");
  Serial.print(node->unique_id,HEX);
  Serial.print(" finished PING");
  Serial.print(" Status: ");
  Serial.println(status);

  //Kick off a GET_NUM_REGISTERS for the first found node
  if (status == A7105_Mesh_STATUS_OK)
  {
    byte node_id = A7105_Get_Next_Present_Node(node->presence_table,0);
    if (node_id != 0)
      A7105_Mesh_GetNumRegisters(node, node_id, get_num_registers_finished);
  }
}

int success = 1;
int radio2_joined = 0;
int ping_sent = 0;
// the loop function runs over and over again forever
void loop() {

  if (radio2_joined ==0 && millis() > 1500)
  {
    A7105_Mesh_Join(&radio2, join_finished);
    radio2_joined = 1;
  }

  if (ping_sent == 0 && millis() > 3000)
  {
    A7105_Mesh_Ping(&radio1, ping_finished);
    ping_sent = 1;
  }



  A7105_Mesh_Update(&radio1);
  A7105_Mesh_Update(&radio2);
  //Serial.println("Looping...");
  //Serial.println(freeRam()); 
  /*
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
  */
  delay(10);
}
