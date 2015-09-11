#include "stdint.h"
#include <SPI.h>
#include <a7105.h> 
#include <a7105_mesh.h> 


/*
This script is a test harness for the 
pro_mini a7105 shield board

It simply joins the mesh on channel 0.

*/


#define RADIO1_SELECT_PIN 7
#define RADIO1_WTR_PIN 8
#define RADIO2_NUM_REGS 1
#define RADIO_IDS 0xdb042679 //taken from devation code, just the ID filter for the current mesh

#define putstring(x) SerialPrint_P(PSTR(x))                             
void SerialPrint_P(PGM_P str) {                                         
  for (uint8_t c; (c = pgm_read_byte(str)); str++) Serial.write(c);     
} 

const char* RADIO2_REG_NAMES[] = {"Radio2_R1"}; //,
//                                "Radio2_R2"};

struct A7105_Mesh radio1;
struct A7105_Mesh_Register reg_buffer;
A7105_Mesh_Register radio2_regs[RADIO2_NUM_REGS];

void setup() {
  Serial.begin(115200);

  Serial.println("Starting...");

  pinMode(MOSI,OUTPUT);
  pinMode(MISO,INPUT);
  pinMode(SCK,OUTPUT);
 
  //Initialize the radios
  A7105_Mesh_Status success = A7105_Mesh_Initialize(&(radio1),
                                                    RADIO1_SELECT_PIN,
                                                    RADIO1_WTR_PIN,
                                                    RADIO_IDS,
                                                    A7105_DATA_RATE_100Kbps,
                                                    0,
                                                    A7105_TXPOWER_10mW,
                                                    0);
  putstring("Radio 1 Init: ");
  Serial.print(success == A7105_Mesh_STATUS_OK);
  putstring(", ID = ");
  Serial.println(radio1.unique_id,HEX);

  
  //A7105_Mesh_Register_Initialize(&(context.reg_buffer),NULL,NULL);

  /*//initialize radio2's registers
  for (int x = 0;x<RADIO2_NUM_REGS;x++)
  {
    A7105_Mesh_Register_Initialize(&(radio2_regs[x]),set_register_called,get_register_called);
    A7105_Mesh_Util_SetRegisterNameStr(&(radio2_regs[x]),RADIO2_REG_NAMES[x]);
    A7105_Mesh_Util_SetRegisterValueU32(&(radio2_regs[x]),x);
  }
  A7105_Mesh_Set_Node_Registers(&(context.radio2),radio2_regs,RADIO2_NUM_REGS);
   */
}



/*void join_finished(struct A7105_Mesh* node, A7105_Mesh_Status status,void* context)
{
  putstring("Node ");
  Serial.print(node->unique_id,HEX);
  putstring(" finished JOIN as ");
  Serial.print(node->node_id);
  putstring(" Status: ");
  Serial.println(status);

  TestHarnessContext* c = (TestHarnessContext*)context;

  //Update our test harness state
  if (node->unique_id == c->radio1.unique_id)
    c->radio1_joined = 2;
  if (node->unique_id == c->radio2.unique_id)
    c->radio2_joined = 2;

  putstring("Radio1 joined: ");
  Serial.println(c->radio1_joined);
  putstring("Radio2 joined: ");
  Serial.println(c->radio2_joined);

  if (c->radio1_joined + c->radio2_joined == 4)
    c->curr_state = RADIOS_JOINED;
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

  TestHarnessContext* c = (TestHarnessContext*)context;

  //Kick off a GET_NUM_REGISTERS for the first found node
  if (status == A7105_Mesh_STATUS_OK)
  {
    c->curr_target_node = 0;
    c->curr_state = R1_PING_DONE;
  }
}
void get_num_registers_finished(struct A7105_Mesh* node, A7105_Mesh_Status status,void* context)
{
  putstring("Node ");
  Serial.print(node->unique_id,HEX);
  putstring(" finished GET_NUM_REGISTERS");
  putstring(" Status: ");
  Serial.println(status);

  putstring("Free RAM after get_num_registers finshed: ");
  Serial.println(freeRam());

  TestHarnessContext* c = (TestHarnessContext*)context;
  if (status == A7105_Mesh_STATUS_OK)
  {
    putstring("Response from node: ");
    Serial.print(node->responder_unique_id,HEX);
    putstring(" (");
    Serial.print(node->responder_node_id);
    putstring("): ");
    Serial.println(node->num_registers_cache);

    c->curr_target_reg_index = 0;
    c->curr_target_num_registers = node->num_registers_cache;
    c->curr_state = R1_GET_NUM_REGS_DONE;
  }
  else
    c->curr_state = IDLE;
}

void get_register_name_finished(struct A7105_Mesh* node, A7105_Mesh_Status status,void* context)
{
  char buffer[64];
  
  putstring("Node ");
  Serial.print(node->unique_id,HEX);
  putstring(" finished GET_REGISTER_NAME");
  putstring(" Status: ");
  Serial.println(status);

  putstring("Free RAM after get_register_name finshed: ");
  Serial.println(freeRam());

  TestHarnessContext* c = (TestHarnessContext*)context;

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

    //Save the register in our cache
    A7105_Mesh_Register_Copy(&(c->reg_buffer),&(node->register_cache));

    c->curr_state = R1_GET_REG_NAME_DONE;
  }
}

void get_register_finished(struct A7105_Mesh* node, A7105_Mesh_Status status,void* context)
{
  uint32_t buffer; 

  putstring("Node ");
  Serial.print(node->unique_id,HEX);
  putstring(" finished GET_REGISTER");
  putstring(" Status: ");
  Serial.println(status);

  putstring("Free RAM after get_register finshed: ");
  Serial.println(freeRam());

  TestHarnessContext* c = (TestHarnessContext*)context;

  if (status == A7105_Mesh_STATUS_OK)
  {
    putstring("Response from node: ");
    Serial.print(node->responder_unique_id,HEX);
    putstring(" (");
    Serial.print(node->responder_node_id);
    putstring("): \"");

    A7105_Mesh_Util_GetRegisterValueU32(&(node->register_cache),&buffer);

    Serial.print(buffer);
    putstring("\"\r\n");

    //Put the returned value in our cache so we can increment
    A7105_Mesh_Register_Copy(&(c->reg_buffer),&(node->register_cache));
    
    c->curr_state = R1_GET_REG_VALUE_DONE;
  }
}

void set_register_finished(struct A7105_Mesh* node, A7105_Mesh_Status status,void* context)
{

  putstring("Node ");
  Serial.print(node->unique_id,HEX);
  putstring(" finished SET_REGISTER");
  putstring(" Status: ");
  Serial.println(status);

  putstring("Free RAM after set_register finshed: ");
  Serial.println(freeRam());

  TestHarnessContext* c = (TestHarnessContext*)context;

  putstring("Response from node: ");
  Serial.print(node->responder_unique_id,HEX);
  putstring(" (");
  Serial.print(node->responder_node_id);
  putstring("): ");
  Serial.println(status);



  if (status == A7105_Mesh_STATUS_OK)
  {
    putstring("Setting state to start querying next available register\r\n");
    
    c->curr_state = R1_GET_NUM_REGS_DONE;
    c->curr_target_reg_index += 1;
  }

  else if (status == A7105_Mesh_INVALID_REGISTER_VALUE)
  {
    putstring("remote node returned an error: (");
    Serial.print((const char *)A7105_Mesh_Register_Get_Error(node));
    putstring(")\r\n");
  }
}
*/

// the loop function runs over and over again forever
unsigned long second = 0;
void loop() {
  if (millis() - (second * 1000) > 1000)
  {
    if (second == 0)
      A7105_Mesh_Join(&(radio1), NULL);

    if (second == 5)
    {
      byte clock_wanger = A7105_ReadReg(&(radio1.radio),A7105_0D_CLOCK);
      Serial.print("CLOCK is: ");
      Serial.println(clock_wanger);
    }

    second = millis() / 1000;
    //Serial.println("TICK");
  }

  /*
  //Join the radios if they haven't done so yet
  if (millis() > context.radio1_join_delay && context.radio1_joined == 0)
  {
    context.radio1_joined = 1;
  }
  if (millis() > context.radio2_join_delay && context.radio2_joined == 0)
  {
    context.radio2_joined = 1;
    A7105_Mesh_Join(&(context.radio2), join_finished);
  }

  //Send a ping with radio1 once all radios are joined to the mesh
  if (context.curr_state == RADIOS_JOINED)
  {
    putstring("Radio1 Pinging...\r\n");
    context.curr_state = R1_PINGING;
    A7105_Mesh_Ping(&(context.radio1), ping_finished);
  }

  if (context.curr_state == R1_PING_DONE)
  {
    putstring("Getting next node after id: ");
    Serial.println(context.curr_target_node);

    //Get the next node to query
    context.curr_target_node = A7105_Get_Next_Present_Node(&(context.radio1),context.curr_target_node);
    
    putstring("New node is id: ");
    Serial.println(context.curr_target_node);

    //If we're done querying nodes
    if (context.curr_target_node == 0)
    {
      putstring("Done iterating nodes...\r\n");
      //NOTE: uncomment this to just keep incrementing registers forever
      context.curr_state = IDLE;
    }

    //else, kick off a get num regs for the next node
    else
    {
      context.curr_state = R1_GET_NUM_REGS;
      putstring("Radio1 GetNumRegisters for: ");
      Serial.println(context.curr_target_node);
      A7105_Mesh_GetNumRegisters(&(context.radio1), context.curr_target_node, get_num_registers_finished);
    }
  }

  if (context.curr_state == R1_GET_NUM_REGS_DONE)
  {
    //If we've exhausted all the registers on the current node
    if (context.curr_target_reg_index >= context.curr_target_num_registers)
    {
      putstring("Done iterating all registers for node: ");
      Serial.println(context.curr_target_node);
      //Go back to intereate the next node (if any are found)
      context.curr_state = R1_PING_DONE;
    }

    else
    {
      context.curr_state = R1_GET_REG_NAME;
      //Start getting the names of the registers
      putstring("Radio1 GetRegisterName for index: ");
      Serial.print(context.curr_target_reg_index);
      putstring(" on node ");
      Serial.println(context.curr_target_node);

      A7105_Mesh_GetRegisterName(&(context.radio1),
                                 context.curr_target_node,
                                 context.curr_target_reg_index,
                                 get_register_name_finished);
    }
  }

  if (context.curr_state == R1_GET_REG_NAME_DONE)
  {
    //Get the value of the current register
    context.curr_state = R1_GET_REG_VALUE;
    putstring("Radio1 GetRegister for register name: ");
    char name_buff[64];
    A7105_Mesh_Util_GetRegisterNameStr(&(context.reg_buffer),name_buff,64);
    Serial.println(name_buff);

    A7105_Mesh_GetRegister(&(context.radio1),
                           &(context.reg_buffer),
                           get_register_finished);
 }

 if (context.curr_state == R1_GET_REG_VALUE_DONE)
 {
    context.curr_state = R1_SET_REG_VALUE;
    putstring("Radio1 SetRegister for register name: ");
    char name_buff[64];
    A7105_Mesh_Util_GetRegisterNameStr(&(context.reg_buffer),name_buff,64);

    Serial.println(name_buff);
    putstring("Incrementing value by 1\r\n");

    //increment the value in the register object
    uint32_t val;
    A7105_Mesh_Util_GetRegisterValueU32(&(context.reg_buffer),&val);
    val++;
    A7105_Mesh_Util_SetRegisterValueU32(&(context.reg_buffer),val);

    A7105_Mesh_SetRegister(&(context.radio1),
                           &(context.reg_buffer),
                           set_register_finished);
 }
  */
  A7105_Mesh_Update(&(radio1));
  //A7105_Mesh_Update(&(context.radio2));
  //delay(10);
}