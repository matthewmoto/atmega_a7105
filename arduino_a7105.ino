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

#define putstring(x) SerialPrint_P(PSTR(x))                             
void SerialPrint_P(PGM_P str) {                                         
  for (uint8_t c; (c = pgm_read_byte(str)); str++) Serial.write(c);     
} 


void join_finished(struct A7105_Mesh* node, A7105_Mesh_Status status,void* context);

A7105_Mesh radio1;
A7105_Mesh radio2;

#define RADIO2_NUMREGISTERS 1
int state = 0;
byte node_id = 255;
byte reg_index = 0;
byte num_registers = 0;

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

  putstring("Radio 1 Init: ");
  Serial.print(success == A7105_Mesh_STATUS_OK);
  putstring(", ID = ");
  Serial.println(radio1.unique_id,HEX);
  
  putstring("Radio 2 Init: ");
  Serial.print(success2 == A7105_Mesh_STATUS_OK);
  putstring(", ID = ");
  Serial.println(radio2.unique_id,HEX);


  //Try to make radio1 join the Mesh
  A7105_Mesh_Join(&radio1, join_finished);

  //initialize radio2's registers
  radio2.registers = (struct A7105_Mesh_Register*)malloc(sizeof(struct A7105_Mesh_Register)*1);

  radio2.registers[0]._name_len = 1;
  radio2.registers[0]._data_len = 1;
  radio2.registers[0]._data[0] = 'a';
  radio2.registers[0]._data[1] = 'A';
  
  radio2.num_registers = 1;
  //radio2.num_registers = RADIO2_NUMREGISTERS;
   
}

int freeRam () 
{
  extern int __heap_start, *__brkval; 
  int v; 
  return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval); 
}

void join_finished(struct A7105_Mesh* node, A7105_Mesh_Status status,void* context)
{
  putstring("Node ");
  Serial.print(node->unique_id,HEX);
  putstring(" finished JOIN as ");
  Serial.print(node->node_id);
  putstring(" Status: ");
  Serial.println(status);

  if (node->unique_id == radio2.unique_id)
    state = 2;
}

void get_num_registers_finished(struct A7105_Mesh* node, A7105_Mesh_Status status,void* context)
{
  putstring("Node ");
  Serial.print(node->unique_id,HEX);
  putstring(" finished GET_NUM_REGISTERS");
  putstring(" Status: ");
  Serial.println(status);

  if (status == A7105_Mesh_STATUS_OK)
  {
    putstring("Response from node: ");
    Serial.print(node->responder_unique_id,HEX);
    putstring(" (");
    Serial.print(node->responder_node_id);
    putstring("): ");
    Serial.println(node->num_registers_cache);
    state = 6;
    num_registers = radio1.num_registers_cache;
    reg_index = 0;
  }
}

void get_register_name_finished(struct A7105_Mesh* node, A7105_Mesh_Status status,void* context)
{
  char buffer[64];
  
  putstring("Node ");
  Serial.print(node->unique_id,HEX);
  putstring(" finished GET_REGISTER_NAME");
  putstring(" Status: ");
  Serial.println(status);

  if (status == A7105_Mesh_STATUS_OK)
  {
    putstring("Response from node: ");
    Serial.print(node->responder_unique_id,HEX);
    putstring(" (");
    Serial.print(node->responder_node_id);
    putstring("): \"");

    A7105_Mesh_Util_GetRegisterNameStr(&(node->register_cache),
                                       buffer,
                                       63);

    Serial.print(buffer);
    putstring("\"\r\n");
    state = 6;
  }
}

void get_register_finished(struct A7105_Mesh* node, A7105_Mesh_Status status,void* context)
{
  char buffer[64];
  
  putstring("Node ");
  Serial.print(node->unique_id,HEX);
  putstring(" finished GET_REGISTER");
  putstring(" Status: ");
  Serial.println(status);

  if (status == A7105_Mesh_STATUS_OK)
  {
    putstring("Response from node: ");
    Serial.print(node->responder_unique_id,HEX);
    putstring(" (");
    Serial.print(node->responder_node_id);
    putstring("): \"");

    A7105_Mesh_Util_GetRegisterValueStr(&(node->register_cache),
                                       buffer,
                                       63);

    Serial.print(buffer);
    putstring("\"\r\n");
    state = 10;
  }
}


void ping_finished(struct A7105_Mesh* node, A7105_Mesh_Status status,void* context)
{
  putstring("Node ");
  Serial.print(node->unique_id,HEX);
  putstring(" finished PING");
  putstring(" Status: ");
  Serial.println(status);

  putstring("Free RAM after ping finshed: ");
  Serial.println(freeRam());

  //Kick off a GET_NUM_REGISTERS for the first found node
  if (status == A7105_Mesh_STATUS_OK)
  {
    state = 4;
  }
}

int success = 1;
int radio2_joined = 0;
int ping_sent = 0;

//0 = radio1 joining, nothing else
//1 = radio2 joining
//2 = radio2 joined
//3 = radio1 pings
//4 = radio1 ping finished
//5 = radio1 get_num_registers from first ping response (radio2)
//6 = radio1 get_num_registers finshed
//7 = radio1 getting register name 
//8 = radio1 done getting register name
//9 = radio1 getting register value for last register retrieved
//10 = radio1 done getting register value

// the loop function runs over and over again forever
void loop() {

  //Join radio 2
  if (state == 0 && millis() > 1500)
  {
    state = 1;
    A7105_Mesh_Join(&radio2, join_finished);
  }

  //Send out a ping from radio 1
  if (state == 2 && millis() > 3000)
  {
    state = 3;
    A7105_Mesh_Ping(&radio1, ping_finished);
  }

  //Get the registers from the first ping response
  if (state == 4)
  {
    state = 5;
    node_id = A7105_Get_Next_Present_Node(radio1.presence_table,0);
    if (node_id != 0)
      A7105_Mesh_GetNumRegisters(&radio1, node_id, get_num_registers_finished);
  }

  if (state == 6 )
  {
      if (reg_index < num_registers)
      {
        putstring("Get register name for index: ");
        Serial.println(reg_index);
        state = 7;
        A7105_Mesh_GetRegisterName(&radio1, node_id,reg_index,get_register_name_finished);
        reg_index++;
      }
      else
        state = 8;
  }

  if (state == 8)
  {
    putstring("Getting value of register 'a'...\r\n");
    A7105_Mesh_Register reg;
    reg._name_len = 1;
    reg._data_len = 1;
    reg._data[0] = 'a';    
    A7105_Mesh_GetRegister(&radio1, &reg, get_register_finished);
    state = 9;
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
