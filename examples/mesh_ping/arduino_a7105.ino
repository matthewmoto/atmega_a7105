#include "stdint.h"
#include <SPI.h>
#include <a7105.h> 
#include <a7105_mesh.h> 


//Pin used to activate recoding on my logic analyzer...
#define RECORD 4

#define RADIO1_SELECT_PIN 9
#define RADIO1_WTR_PIN 6

#define RADIO2_SELECT_PIN 8
#define RADIO2_WTR_PIN 5

#define RADIO_IDS 0xdb042679 //taken from devation code

#define RADIO1_JOIN_DELAY 0
#define RADIO2_JOIN_DELAY 1500
#define RADIO2_NUM_REGS 2

#define putstring(x) SerialPrint_P(PSTR(x))                             
void SerialPrint_P(PGM_P str) {                                         
  for (uint8_t c; (c = pgm_read_byte(str)); str++) Serial.write(c);     
} 

const char* RADIO2_REG_NAMES[] = {"Radio2_R1",
                                  "Radio2_R2"};

struct A7105_Mesh radio1;
struct A7105_Mesh_Register reg_buffer;

A7105_Mesh_Register radio2_regs[RADIO2_NUM_REGS];

int freeRam () 
{
  extern int __heap_start, *__brkval; 
  int v; 
  return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval); 
}


A7105_Mesh_Status get_register_called(A7105_Mesh_Register* reg, void* c)
{
  struct TestHarnessContext* ctx = (struct TestHarnessContext*)c;
  putstring("GET_REGISTER called\r\n");
  return A7105_Mesh_STATUS_OK;
}

A7105_Mesh_Status set_register_called(A7105_Mesh_Register* reg, A7105_Mesh* node, void* c)
{
  putstring("SET_REGISTER called\r\n");
  //return A7105_Mesh_AUTO_SET_REGISTER;

  //DEBUG to see if errors propagate right
  A7105_Mesh_Register_Set_Error(node,"dunno, broke?");
  return A7105_Mesh_INVALID_REGISTER_VALUE;
}

// the setup function runs once when you press reset or power the board
void setup() {
  Serial.begin(115200);

  Serial.println("Starting...");

  //TODO: Can we nuke this safely?
  //pinMode(MOSI,OUTPUT);
  //pinMode(MISO,INPUT);
  //pinMode(SCK,OUTPUT);

  //DEBUG (for logic probe operations)
  pinMode(RECORD,OUTPUT);
  digitalWrite(RECORD,LOW);

  //DEBUG (print free RAM for monitoring)
  putstring("Free RAM [setup()]: ");
  Serial.println(freeRam());

  //Initialize the radios
  A7105_Mesh_Status success = A7105_Mesh_Initialize(&(radio1),
                                                    RADIO1_SELECT_PIN,
                                                    RADIO1_WTR_PIN,
                                                    RADIO_IDS,
                                                    A7105_DATA_RATE_250Kbps,
                                                    0,
                                                    A7105_TXPOWER_150mW,
                                                    0);
  putstring("Radio 1 Init: ");
  Serial.print(success == A7105_Mesh_STATUS_OK);
  putstring(", ID = ");
  Serial.println(radio1.unique_id,HEX);
  


  //Initialize the context structure
  A7105_Mesh_Register_Initialize(&(reg_buffer),NULL,NULL);

  //Set the context for both radios
  //A7105_Mesh_Set_Context(&(context.radio1),&context);

  //initialize radio2's registers
  for (int x = 0;x<RADIO2_NUM_REGS;x++)
  {
    A7105_Mesh_Register_Initialize(&(radio2_regs[x]),set_register_called,get_register_called);
    A7105_Mesh_Util_SetRegisterNameStr(&(radio2_regs[x]),RADIO2_REG_NAMES[x]);
    A7105_Mesh_Util_SetRegisterValueU32(&(radio2_regs[x]),x);
  }
  A7105_Mesh_Set_Node_Registers(&(radio1),radio2_regs,RADIO2_NUM_REGS);


  //Join the mesh
  A7105_Mesh_Join(&(radio1), NULL);
   
}

// the loop function runs over and over again forever
void loop() {
  putstring("Free RAM [loop()]: ");
  Serial.println(freeRam());


  A7105_Mesh_Status success = A7105_Mesh_Ping(&(radio1), NULL);


  putstring("Status of mesh ping: ");
  Serial.println(success);
  
  delay(3000);
  //A7105_Mesh_Update(&(radio1));
  //delay(10);
}
