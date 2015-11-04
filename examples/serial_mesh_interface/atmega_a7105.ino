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


This is the firmware for the A7105 Serial Mesh Interface. This firmware
runs on the mesh node connected to the Halloween 2015 scene controller
and allows the software on that machine to communicate with the effects 
mesh network via a simple serial interface.

Note: All Serial commands return either the specified "Response:" response defined below or 
the universal error response that looks like this: 
  "ERROR,ERROR_ID,MSG\r\n"

Where ERROR_ID is a non-zero integer defined below and MSG is a contextually helpful
plaintext ascii string to aid in identifying the issue.

All responses or interrupting data will be one or more comma delimited lines ending in /r/n.

Serial Commands:
  ECHO: Turns on command echoing
  Response: ECHO,<TOGGLED>
  NOTE: TOGGLED is 1/0 for on/off (respectively) (default is OFF)

  LISTEN: Listen for broadcast events from the mesh.
    Response: N lines of REGISTER_VALUE,REGISTER_NAME,REGISTER_VALUE\r\n
    Possible errors: A7105_Mesh_NOT_ON_MESH
    Note: No other commands may be performed during a LISTEN. To exit LISTEN,
          pass a byte (or less than 120 bytes) and wait for the string
          "READY\r\n" to come back, before issuing any commands.
          When entering LISTEN mode, the Arduino will first send back
          the line "LISTEN\r\n" as an ACK that it is now listening.

    Possible errors: A7105_Mesh_MESH_ALREADY_JOINING

  RESET:    Leave the current mesh (no longer participate), reinitialize
            the A7105 radio and generate a new unique ID.
    Response: INIT,<SUCCESS>,UNIQUE_ID
    Possible errors: A7105_Mesh_NOT_ON_MESH
    NOTE: SUCCESS is 0 or 1 for failure/success respectively and 
          UNIQUE_ID is an ASCII_HEX string for the 2-byte unique ID
          generated for this instance.

  PING: Ping the mesh to get the connected nodes
    Response: PING,NODE_ID_1,NODE_ID_2,.....NODE_ID_N (where node ID's are integers of found nodes on the mesh)
    Possible errors: A7105_Mesh_NOT_ON_MESH, A7105_Mesh_BUSY

  LIST_REGISTERS: Get all register names on the mesh
    Response: LIST_REGISTERS,REGISTER_NAME_1,REGISTER_NAME_2,....,ERRORS
    Possible errors: A7105_Mesh_NOT_ON_MESH, A7105_Mesh_BUSY
    
    NOTE: The ERRORS field in the response is a numeric value (0-255)
          denoting the number of nodes that returned incomplete data
          (either the number of registers or names of registers served)
          so if this value is non-zero, the results should be considered
          incomplete

  GET_REGISTER,<REGISTER_NAME>,<FORMAT>: Get's the value of a single register identified by <REGISTER_NAME>
    Response: GET_REGISTER,REGISTER_NAME,REGISTER_VALUE
    POSSIBLE errors: A7105_Mesh_NOT_ON_MESH, A7105_Mesh_INVALID_REGISTER_INDEX
  
    NOTE: FORMAT can be "STRING" for ASCII data, "BINARY" for binary (output will be
          ASCII hex), or "UINT32" for unsigned integer data.

  SET_REGISTER,<REGISTER_NAME>,<FORMAT>,<REGISTER_VALUE>: Set's the value of a single register identified by <REGISTER_NAME>
    Response: SET_REGISTER_ACK,REGISTER_NAME
    POSSIBLE errors: A7105_Mesh_NOT_ON_MESH, A7105_Mesh_INVALID_REGISTER_INDEX, A7105_Mesh_INVALID_REGISTER_VALUE
    
    NOTE: REGISTER_VALUE is interpreted differently depending on FORMAT. If FORMAT isZ:
      * STRING: REGISTER_VALUE is sent as a string whose value is terminated by /r/n on
                the serial terminal (not included in value sent).
      * BINARY: REGISTER_VALUE is taken to be a byte-alinged ASCII_HEX string whose value is sent
                as a raw binary value.
      * UINT32: REGISTER_VALUE must be an ascii string representing a numerical value between
                0 and UINT32_MAX.

  VALUE_BROADCAST,<REGISTER_NAME>,<FORMAT>,<REGISTER_VALUE>: Broadcasts a value for a register (does not need to actually
                                                             be hosted).
    Response: READY 
    Possible errors: TBD
      
    NOTE: See SET_REGISTER above for possible values of FORMAT.
    
                
 
LISTEN-Mode Data (not directly associated with a command):
  REGISTER_VALUE,<REGISTER_NAME>,<REGISTER_VALUE>
  If a node pushes a REGISTER_VALUE packet out without a node-id, it is considered a value broadcast 
  and will be pushed to the serial terminal at the time of arrival, however it will not preceed the 
  current operation. <HACK: Only the last seen broadcast will be output>

*/


//Constants for the Pro Mini A7105 Radio Shield v0.2
#define RADIO1_SELECT_PIN 7
#define RADIO1_WTR_PIN 8
#define RADIO_IDS 0xdb042679 //The ID filter for the current mesh (TODO: Put this in the library)

//Size of our ascii buffer for input output use
#define ASCII_BUFFER_SIZE 64
#define SERIAL_READ_TIMEOUT 1000 //milliseconds between character reads
#define HEX_CONV_ERR 69 //<bill_and_teds_excellent_adventure>What number are you thinking of? 69 BRO!</bill_and_teds_excellent_adventure>

#define MAX_BROADCAST_CACHE_SIZE 4 //Number of REGISTERS we cache values for during non-LISTEN times
#define putstring(x) SerialPrint_P(PSTR(x))                             
void SerialPrint_P(PGM_P str) {                                         
  for (uint8_t c; (c = pgm_read_byte(str)); str++) Serial.write(c);     
} 

//String constants in PROGMEM to avoid eating RAM
const char ECHO_STR[] PROGMEM = "ECHO";
const char LISTEN_STR[] PROGMEM = "LISTEN";
const char READY_STR[] PROGMEM = "READY";
const char JOIN_STR[] PROGMEM = "JOIN";
const char JOINED_STR[] PROGMEM = "JOINED";
const char RESET_STR[] PROGMEM = "RESET";
const char LEFT_STR[] PROGMEM = "LEFT";
const char PING_STR[] PROGMEM = "PING";
const char LIST_REGISTERS_STR[] PROGMEM = "LIST_REGISTERS";
const char GET_REGISTER_STR[] PROGMEM = "GET_REGISTER";
const char SET_REGISTER_STR[] PROGMEM = "SET_REGISTER";
const char SET_REGISTER_ACK_STR[] PROGMEM = "SET_REGISTER_ACK";
const char REGISTER_VALUE_STR[] PROGMEM = "REGISTER_VALUE";
const char VALUE_BROADCAST_STR[] PROGMEM = "VALUE_BROADCAST";



const char STRING_FMT_STR[] PROGMEM = "STRING";
const char BINARY_FMT_STR[] PROGMEM = "BINARY";
const char UINT32_FMT_STR[] PROGMEM = "UINT32";

struct A7105_Mesh RADIO;
struct A7105_Mesh_Register REG_BUFFER;

byte HOST_LISTENING = 0;
byte ECHO = 0;

//Value broadcast cache
byte BROADCAST_CACHE_END = 0;
byte BROADCAST_CACHE_START = 0;
byte BROADCAST_CACHE_SIZE = 0;
byte BROADCAST_CACHE_OVERFLOW = 0;
struct A7105_Mesh_Register BROADCAST_CACHE[MAX_BROADCAST_CACHE_SIZE];

byte BROADCAST_VALUE_WAITING;
char ASCII_BUFFER[ASCII_BUFFER_SIZE];

enum Reg_Data_Format{
  FMT_ERROR,
  STRING,
  BINARY, //NOTE: Outputted as ascii-hex
  UINT32}; 


int freeRam ()
{
  extern int __heap_start, *__brkval;
  int v; 
  return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval);
}

void broadcast_cache_init()
{
  BROADCAST_CACHE_END = 0;
  BROADCAST_CACHE_START = 0;
  BROADCAST_CACHE_SIZE = 0;
  BROADCAST_CACHE_OVERFLOW = 0;
}

//Copy the register from the Register buffer (REG_BUFFER)
//to the broadcast cache. 
//NOTE: This method updates values if the same register name 
//      is found and overwrites values in the ring buffer. 
//      WARNING, this doesn't necessarily mean the value
//               overwritten is the oldest either!
void broadcast_cache_append()
{
 
  //If we already have this register name in the cache
  //Just copy the new value in
  for (int x = 0;x<BROADCAST_CACHE_SIZE;x++)
  {

    if (!A7105_Mesh_Util_RegisterNameCmp(&(BROADCAST_CACHE[x]),
                                         &REG_BUFFER))
    {
      A7105_Mesh_Register_Copy(&(BROADCAST_CACHE[x]), &REG_BUFFER);
      return;
    }
  }

  //Check if we're going to overflow (cap at 255 overflows to record)
  if (BROADCAST_CACHE_SIZE == MAX_BROADCAST_CACHE_SIZE &&
      BROADCAST_CACHE_OVERFLOW < 255)
  {
    BROADCAST_CACHE_OVERFLOW += 1;
  } 

  //Stick the next register at the end of the ring buffer
  A7105_Mesh_Register_Copy(&(BROADCAST_CACHE[BROADCAST_CACHE_END]), &REG_BUFFER);
  BROADCAST_CACHE_END += 1;
  BROADCAST_CACHE_END %= MAX_BROADCAST_CACHE_SIZE;
  BROADCAST_CACHE_SIZE++;
  if (BROADCAST_CACHE_SIZE > MAX_BROADCAST_CACHE_SIZE)
    BROADCAST_CACHE_SIZE = MAX_BROADCAST_CACHE_SIZE;
}

//Pop register from the ring buffer or NULL if empty
A7105_Mesh_Register* broadcast_cache_pop()
{
  if (BROADCAST_CACHE_SIZE == 0)
    return NULL;

  A7105_Mesh_Register* ret = &(BROADCAST_CACHE[BROADCAST_CACHE_START]);
  BROADCAST_CACHE_START++;
  BROADCAST_CACHE_START %= MAX_BROADCAST_CACHE_SIZE;
  BROADCAST_CACHE_SIZE--;
  return ret;
}


void dump_broadcast_cache()
{
    A7105_Mesh_Register* curr = NULL;
    while ((curr = broadcast_cache_pop()) != NULL)
    {
      SerialPrint_P(REGISTER_VALUE_STR);
      putstring(",");
      A7105_Mesh_Util_GetRegisterNameStr(curr, ASCII_BUFFER, ASCII_BUFFER_SIZE);
      Serial.print(ASCII_BUFFER);
      putstring(",");
      A7105_Mesh_Util_GetRegisterValueStr(curr, ASCII_BUFFER, ASCII_BUFFER_SIZE);
      Serial.println(ASCII_BUFFER);

      //check if the host has told us to stop and break if so
      //NOTE: the state change for HOST_LISTENING takes place in the mainloop
      if (Serial.available() > 0)
        break;
    }
}

void value_broadcast_callback(struct A7105_Mesh* node, void* context){
  //if the host is in listening mode, just push the data to them
 
  if (HOST_LISTENING)
  {
      SerialPrint_P(REGISTER_VALUE_STR);
      putstring(",");
      A7105_Mesh_Util_GetRegisterNameStr(&REG_BUFFER, ASCII_BUFFER, ASCII_BUFFER_SIZE);
      Serial.print(ASCII_BUFFER);
      putstring(",");
      A7105_Mesh_Util_GetRegisterValueStr(&REG_BUFFER, ASCII_BUFFER, ASCII_BUFFER_SIZE);
      Serial.println(ASCII_BUFFER);
  }

  //otherwise cache the broadcast
  else
  {
    broadcast_cache_append(); 
  }
}

void setup() {
  Serial.begin(115200);

  //Serial.println("Starting...");

  pinMode(MOSI,OUTPUT);
  pinMode(MISO,INPUT);
  pinMode(SCK,OUTPUT);
 

  init_radio_and_broadcast_cache();
}

void output_error(A7105_Mesh_Status err_code, PGM_P str)
{
  putstring("ERROR,");
  Serial.print(err_code,DEC);
  putstring(",");
  SerialPrint_P(str);
  putstring("\r\n");
}

void unknown_command()
{
  putstring("ERROR,");
  Serial.print(A7105_Mesh_NO_STATUS,DEC);
  putstring(",Unknown command: \"");
  Serial.print(ASCII_BUFFER);
  putstring("\"\r\n");
}

void toggle_echo()
{
  //Toggle the echo flag
  ECHO = ECHO ? 0 : 1;
  putstring("ECHO,");
  Serial.println(ECHO,DEC);
}

void join_mesh()
{
  //Ensure we are legitimate to join (i.. we aren't already joined or joining)
  if (RADIO.state != A7105_Mesh_NOT_JOINED)
  {
    output_error(A7105_Mesh_MESH_ALREADY_JOINING,PSTR("Node already joined or joining mesh"));
    return;
  }

  //Attempt to join and handle success
  A7105_Mesh_Status join_status = A7105_Mesh_Join(&(RADIO), NULL);
  if (join_status == A7105_Mesh_STATUS_OK)
  {
    SerialPrint_P(JOINED_STR);
    putstring("\r\n");
  }
  
  //Error out of the mesh is full
  else if (join_status == A7105_Mesh_MESH_FULL)
  {
    output_error(A7105_Mesh_MESH_FULL,PSTR("Mesh is full"));
  }
}

void init_radio_and_broadcast_cache()
{
  A7105_Mesh_Status success = A7105_Mesh_Initialize(&(RADIO),
                                                    RADIO1_SELECT_PIN,
                                                    RADIO1_WTR_PIN,
                                                    RADIO_IDS,
                                                    A7105_DATA_RATE_125Kbps,
                                                    0,
                                                    A7105_TXPOWER_150mW,
                                                    0);
  putstring("INIT,");
  Serial.print(success == A7105_Mesh_STATUS_OK);
  putstring(",");
  byte curr=0;
  for (int shift = 8;shift>=0;shift-=8)
  {
    curr = RADIO.unique_id>>shift & 0xFF;
    //HACK to output properly padded HEX data with AVR
    if (curr < 0x10) putstring("0");
    Serial.print(curr,HEX);
  }
  putstring("\r\n");

  //Initialize our register value broadcast listener
  A7105_Mesh_Broadcast_Listen(&(RADIO),
                              value_broadcast_callback,
                              &(REG_BUFFER));
  //BROADCAST_VALUE_WAITING = 0;
  HOST_LISTENING = 0;
  broadcast_cache_init();
 
}

void ping_mesh()
{
  A7105_Mesh_Status ping_status = A7105_Mesh_Ping(&(RADIO),NULL);

  if (ping_status == A7105_Mesh_NOT_ON_MESH)
  { 
    output_error(A7105_Mesh_NOT_ON_MESH,PSTR("Node not joined to a mesh"));
  }
  else if (ping_status == A7105_Mesh_BUSY)
  {
    output_error(A7105_Mesh_BUSY,PSTR("Node is busy with another operation"));
  }
  else
  { 
    putstring("PING");
    byte current_node = A7105_Get_Next_Present_Node(&(RADIO),0);
    while (current_node)
    {
      Serial.print(',');
      Serial.print(current_node,DEC); 
      current_node = A7105_Get_Next_Present_Node(&(RADIO),current_node);
    }
    putstring("\r\n");
  }

  //putstring("Free RAM [setup()]: ");
  //Serial.println(freeRam());
}

void list_registers()
{
  //Start by pinging mesh
  A7105_Mesh_Status status = A7105_Mesh_Ping(&(RADIO),NULL);

  //bail for any errors (since we can't do anything if we can't ping)
  if (status == A7105_Mesh_NOT_ON_MESH)
  { 
    output_error(A7105_Mesh_NOT_ON_MESH,PSTR("Node not joined to a mesh"));
  }
  else if (status == A7105_Mesh_BUSY)
  {
    output_error(A7105_Mesh_BUSY,PSTR("Node is busy with another operation"));
  }
  else
  {
    byte current_node = A7105_Get_Next_Present_Node(&(RADIO),0);
    uint16_t target_unique_id = 0;
    byte reg_index = 0;
    byte node_error_count = 0;  
    byte error_encountered = 0; //cache for noting an error iterating register names 

    //Output response header
    SerialPrint_P(LIST_REGISTERS_STR);
    putstring(",");
    
    //REMOVEME
    //Serial.println("IGNOREME"); 
    //putstring("starting node is ");
    //Serial.println(current_node,DEC);
    //iterate all nodes on mesh
    while (current_node)
    {

      //Get the number of registers for this node
      status = A7105_Mesh_GetNumRegisters(&RADIO,current_node,NULL); 
      
      //check for errors (move to next node if encountered)
      if (status != A7105_Mesh_STATUS_OK)
      {
        //putstring("Error grabbing num registers: ");
        //Serial.println(status,DEC);
        
        node_error_count++;
        current_node = A7105_Get_Next_Present_Node(&(RADIO),current_node);
        continue;
      }

      //putstring("Iterating regs for node: ");
      //Serial.println(current_node,DEC);
      //putstring("It has this many registers: ");
      //Serial.println(RADIO.num_registers_cache,DEC);
      error_encountered=0;
      //Grab every register from the current node 
      for (reg_index=0;reg_index < RADIO.num_registers_cache;reg_index++)
      {
        status = A7105_Mesh_GetRegisterName(&RADIO,
                                            current_node,
                                            reg_index,
                                            target_unique_id,
                                            NULL);
        //log the error and try the next register name if we fail
        if (status != A7105_Mesh_STATUS_OK)
        {
          error_encountered=1;
          continue;
        }
       
        //putstring("REG: ");
        //Serial.println(reg_index,DEC); 
        //Output the register name
        A7105_Mesh_Util_GetRegisterNameStr(&(RADIO.register_cache), ASCII_BUFFER, ASCII_BUFFER_SIZE);
        Serial.print(ASCII_BUFFER);
        putstring(",");
      }

      if (error_encountered)
        node_error_count++;
      
      current_node = A7105_Get_Next_Present_Node(&(RADIO),current_node);
    }
    
    //Output the number of nodes where errors encountered
    Serial.print(node_error_count,DEC);
    putstring("\r\n");
  }
}

//Reads and interprets a REGISTER format (terminated by , or \n)
Reg_Data_Format read_reg_format()
{
  Reg_Data_Format expected_format = FMT_ERROR;
  int name_len = get_serial_cmd();
  if (name_len < 1 || name_len >= ASCII_BUFFER_SIZE-1)
  {
    return FMT_ERROR;
  }
  if (strcmp_P(ASCII_BUFFER,STRING_FMT_STR) == 0)
    expected_format = STRING;
  if (strcmp_P(ASCII_BUFFER,BINARY_FMT_STR) == 0)
    expected_format = BINARY;
  if (strcmp_P(ASCII_BUFFER,UINT32_FMT_STR) == 0)
    expected_format = UINT32;
  return expected_format;
}


void get_register()
{ 
  //Try to read the name of the register to GET
  int name_len = get_serial_cmd(0);
  
  //If we didn't get any command, print an error and bail
  //HACK NOTE: If we filled the buffer, the CMD is too large
  //           for a packet anyway so it's probably bogus data.
  if (name_len < 1 || name_len >= ASCII_BUFFER_SIZE-1)
  {
    putstring("ERROR,");
    Serial.print(A7105_Mesh_NO_STATUS,DEC);
    putstring(",Invalid Register Name: \"");
    Serial.print(ASCII_BUFFER);
    putstring("\"\r\n");
    return;
  }

  //Prep our register buffer with the name of target register
  //NOTE: we're doing this since we're using ASCII_BUFFER again to get the format
  A7105_Mesh_Util_SetRegisterNameStr(&REG_BUFFER,ASCII_BUFFER);

  //Get the format of the command
  Reg_Data_Format expected_format = read_reg_format();
  if (expected_format == FMT_ERROR)
  {
    putstring("ERROR,");
    Serial.print(A7105_Mesh_NO_STATUS,DEC);
    putstring(",Invalid Data Format Specified: \"");
    Serial.print(ASCII_BUFFER);
    putstring("\"\r\n");
    return;
  }


  //Attempt to get the register in question
  A7105_Mesh_Status status = A7105_Mesh_GetRegister(&RADIO, &REG_BUFFER, NULL);
  if (status != A7105_Mesh_STATUS_OK)
  {
    output_error(status,PSTR("Error getting register"));
    return;
  }
  
  //Output the value returned based on the expected data format
  if (expected_format == STRING)
  {
    //Check the value for legit string data
    //NOTE: Do we really need this?
    byte len = A7105_Mesh_Util_GetRegisterValueStr(&(RADIO.register_cache),
                                                    ASCII_BUFFER,
                                                    ASCII_BUFFER_SIZE);
    if (len == 0)
    {
      output_error(A7105_Mesh_INVALID_REGISTER_RETURNED,PSTR("Empty value returned"));
      return;
    }
   
    //Get the name again to output 
    A7105_Mesh_Util_GetRegisterNameStr(&(RADIO.register_cache),
                                       ASCII_BUFFER,
                                       ASCII_BUFFER_SIZE);
     


    //Output the result line for string format
    SerialPrint_P(GET_REGISTER_STR);
    putstring(",");
    Serial.print(ASCII_BUFFER);
    putstring(",");
    A7105_Mesh_Util_GetRegisterValueStr(&(RADIO.register_cache),
                                       ASCII_BUFFER,
                                       ASCII_BUFFER_SIZE);
    Serial.print(ASCII_BUFFER);
    putstring("\r\n");
  }
  
  else if (expected_format == BINARY)
  {
    //Get the name again to output 
    A7105_Mesh_Util_GetRegisterNameStr(&(RADIO.register_cache),
                                       ASCII_BUFFER,
                                       ASCII_BUFFER_SIZE);

     //Just dump the value in HEX (doesn't matter what it is)
    SerialPrint_P(GET_REGISTER_STR);
    putstring(",");
    Serial.print(ASCII_BUFFER);
    putstring(",");

    byte offset = RADIO.register_cache._name_len;
    for (int x = offset;x<offset+RADIO.register_cache._data_len;x++)
    {
      //HACK to output properly padded HEX data with AVR
      if (RADIO.register_cache._data[x] < 0x10) putstring("0");
      Serial.print(RADIO.register_cache._data[x],HEX);
    }
    putstring("\r\n");
  } 
  else if (expected_format == UINT32)
  {
   //Get the name again to output 
    A7105_Mesh_Util_GetRegisterNameStr(&(RADIO.register_cache),
                                       ASCII_BUFFER,
                                       ASCII_BUFFER_SIZE);

      //Check the value for legit uint32_t data
    uint32_t data;
    //NOTE: Do we really need this?
    byte legit = A7105_Mesh_Util_GetRegisterValueU32(&(RADIO.register_cache),
                                                      &data);
    if (!legit)
    {
      output_error(A7105_Mesh_INVALID_REGISTER_RETURNED,PSTR("Invalid U32 value returned"));
      return;
    }
   
    //Output the result line for string format
    SerialPrint_P(GET_REGISTER_STR);
    putstring(",");
    Serial.print(ASCII_BUFFER);
    putstring(",");
    Serial.print(data, DEC);
    putstring("\r\n");
 
  }
}

//NOTE: returns HEX_CONV_ERR on error
byte ascii_hex_2_bin(char nibble)
{
  if (nibble <= '9' && nibble >='0') return (byte)nibble-'0';
  if (nibble >= 'A' && nibble <='F') return ((byte)nibble-'A')+10;
  return HEX_CONV_ERR;
}

void set_register()
{ 
  //Try to read the name of the register to SET
  int name_len = get_serial_cmd(0);
  
  //If we didn't get any command, print an error and bail
  //HACK NOTE: If we filled the buffer, the CMD is too large
  //           for a packet anyway so it's probably bogus data.
  if (name_len < 1 || name_len >= ASCII_BUFFER_SIZE-1)
  {
    putstring("ERROR,");
    Serial.print(A7105_Mesh_NO_STATUS,DEC);
    putstring(",Invalid Register Name: \"");
    Serial.print(ASCII_BUFFER);
    putstring("\"\r\n");
    return;
  }

  //Prep our register buffer with the name of target register
  //NOTE: we're doing this since we're using ASCII_BUFFER again to get the format
  A7105_Mesh_Util_SetRegisterNameStr(&REG_BUFFER,ASCII_BUFFER);

  //Get the format of the command
  Reg_Data_Format expected_format = read_reg_format();
  if (expected_format == FMT_ERROR)
  {
    putstring("ERROR,");
    Serial.print(A7105_Mesh_NO_STATUS,DEC);
    putstring(",Invalid Data Format Specified: \"");
    Serial.print(ASCII_BUFFER);
    putstring("\"\r\n");
    return;
  }

  //Populate the register's value based on the format
  name_len = get_serial_cmd(0);
  if (expected_format == STRING)
  {
    //TODO: error checking?
    if (!A7105_Mesh_Util_SetRegisterValueStr(&REG_BUFFER,ASCII_BUFFER))
    {
      putstring("ERROR,");
      Serial.print(A7105_Mesh_NO_STATUS,DEC);
      putstring(",Invalid Register Value Specified: \"");
      Serial.print(ASCII_BUFFER);
      putstring("\"\r\n");
      return;   
    }

  }
  else if (expected_format == BINARY)
  {
    //Make sure the length of the input is even
    if (name_len & 1)
    {
      putstring("ERROR,");
      Serial.print(A7105_Mesh_NO_STATUS,DEC);
      putstring(",Invalid register value. Binary data must have an even number of characters: \"");
      Serial.print(ASCII_BUFFER);
      putstring("\"\r\n");
      return;   
     }

    //Store the ascii hex string in the reg buffer as binary
    int offset = REG_BUFFER._name_len;
    for (int x = 0;x<name_len-1;x+=2)
    {
      REG_BUFFER._data[offset++] = (ascii_hex_2_bin(ASCII_BUFFER[x])<<4)|ascii_hex_2_bin(ASCII_BUFFER[x+1]);
    }
    REG_BUFFER._data_len = (name_len>>1);
  }
  else if (expected_format == UINT32)
  {
    //HACK should I be using strtoul?
    uint32_t val = (uint32_t)atol(ASCII_BUFFER);
    if (!A7105_Mesh_Util_SetRegisterValueU32(&REG_BUFFER,val))
    {
      putstring("ERROR,");
      Serial.print(A7105_Mesh_NO_STATUS,DEC);
      putstring(",Invalid Register Value Specified: \"");
      Serial.print(ASCII_BUFFER);
      putstring("\"\r\n");
      return;   
    }  
  }

  //Attempt to Set the register in question
  A7105_Mesh_Status status = A7105_Mesh_SetRegister(&RADIO, &REG_BUFFER, NULL);

  //TODO: Pass error string from remote node back if applicable
  if (status != A7105_Mesh_STATUS_OK)
  {
    output_error(status,PSTR("Error setting register"));
    return;
  }
  
  //Output the result line for string format
  SerialPrint_P(SET_REGISTER_ACK_STR);
  putstring(",");
  A7105_Mesh_Util_GetRegisterNameStr(&REG_BUFFER,
                                     ASCII_BUFFER,
                                     ASCII_BUFFER_SIZE);
  Serial.print(ASCII_BUFFER);
  putstring("\r\n");

}

void value_broadcast()
{ 
  //Try to read the name of the register to send
  int name_len = get_serial_cmd(0);
  
  //If we didn't get any command, print an error and bail
  //HACK NOTE: If we filled the buffer, the CMD is too large
  //           for a packet anyway so it's probably bogus data.
  if (name_len < 1 || name_len >= ASCII_BUFFER_SIZE-1)
  {
    putstring("ERROR,");
    Serial.print(A7105_Mesh_NO_STATUS,DEC);
    putstring(",Invalid Register Name: \"");
    Serial.print(ASCII_BUFFER);
    putstring("\"\r\n");
    return;
  }

  //Prep our register buffer with the name of target register
  //NOTE: we're doing this since we're using ASCII_BUFFER again to get the format
  A7105_Mesh_Util_SetRegisterNameStr(&REG_BUFFER,ASCII_BUFFER);

  //Get the format of the command
  Reg_Data_Format expected_format = read_reg_format();
  if (expected_format == FMT_ERROR)
  {
    putstring("ERROR,");
    Serial.print(A7105_Mesh_NO_STATUS,DEC);
    putstring(",Invalid Data Format Specified: \"");
    Serial.print(ASCII_BUFFER);
    putstring("\"\r\n");
    return;
  }

  //Populate the register's value based on the format
  name_len = get_serial_cmd(0);
  if (expected_format == STRING)
  {
    //TODO: error checking?
    if (!A7105_Mesh_Util_SetRegisterValueStr(&REG_BUFFER,ASCII_BUFFER))
    {
      putstring("ERROR,");
      Serial.print(A7105_Mesh_NO_STATUS,DEC);
      putstring(",Invalid Register Value Specified: \"");
      Serial.print(ASCII_BUFFER);
      putstring("\"\r\n");
      return;   
    }
  }
  else if (expected_format == BINARY)
  {
    //Make sure the length of the input is even
    if (name_len & 1)
    {
      putstring("ERROR,");
      Serial.print(A7105_Mesh_NO_STATUS,DEC);
      putstring(",Invalid register value. Binary data must have an even number of characters: \"");
      Serial.print(ASCII_BUFFER);
      putstring("\"\r\n");
      return;   
     }

    //Store the ascii hex string in the reg buffer as binary
    int offset = REG_BUFFER._name_len;
    for (int x = 0;x<name_len-1;x+=2)
    {
      REG_BUFFER._data[offset++] = (ascii_hex_2_bin(ASCII_BUFFER[x])<<4)|ascii_hex_2_bin(ASCII_BUFFER[x+1]);
    }
    REG_BUFFER._data_len = (name_len>>1);
  }
  else if (expected_format == UINT32)
  {
    //HACK should I be using strtoul?
    uint32_t val = (uint32_t)atol(ASCII_BUFFER);
    if (!A7105_Mesh_Util_SetRegisterValueU32(&REG_BUFFER,val))
    {
      putstring("ERROR,");
      Serial.print(A7105_Mesh_NO_STATUS,DEC);
      putstring(",Invalid Register Value Specified: \"");
      Serial.print(ASCII_BUFFER);
      putstring("\"\r\n");
      return;   
    }  
  }

  //Attempt to Set the register in question
  A7105_Mesh_Status status = A7105_Mesh_Broadcast(&RADIO, &REG_BUFFER);

  //TODO: Pass error string from remote node back if applicable
  if (status != A7105_Mesh_STATUS_OK)
  {
    output_error(status,PSTR("Error broadcasting value"));
    return;
  }
  
  //Output the result line for string format
  SerialPrint_P(READY_STR);
  putstring("\r\n");

}



void start_listening()
{

  //HACK: This node might also be busy, this error is misleading...
  if (RADIO.state != A7105_Mesh_IDLE)
  {
    output_error(A7105_Mesh_NOT_ON_MESH,PSTR("Node is not on a mesh or not idle"));
  } 
  else
  {
    HOST_LISTENING = 1;
    //Output that we're ready
    SerialPrint_P(LISTEN_STR);
    putstring("\r\n");

    //Dump the broadcast cache contents if there are any
    //to the serial port (anything accumulated from the last LISTEN)
    dump_broadcast_cache();
  }
}
/*
  This function grabs the next command from the serial port
  and stores it in ASCII_BUFFER.
  This should not be called if the host is listening (HOST_LISTENING)
  since we're the ones outputting during that mode.

  Returns: The length of the command if a value was gotten, false otherwise

  Side-Effects: ASCII_BUFFER is null terminated as a zero length string if
                nothing was read. Also, if we didn't find a terminating
                character, we read until the next newline to try to sync
                things back up.
*/
int get_serial_cmd()
{
  return get_serial_cmd(1);
}

int get_serial_cmd(byte uppercase)
{
  int index = 0;
  ASCII_BUFFER[index] = '\0'; //zero the buffer so we don't repeat commands
  unsigned long now = millis();
  
  while(Serial.available() == 0 && millis()-now < SERIAL_READ_TIMEOUT){}
  
  if (Serial.available() > 0)
    while ( index < ASCII_BUFFER_SIZE - 1)
    {
      //Uppercase what we get for the command so we can do easy
      //comparisons
      ASCII_BUFFER[index] = uppercase ? toupper(Serial.read()) : Serial.read();
      //HACK: Echo what we got
      if (ECHO){
        Serial.print(ASCII_BUFFER[index]);
        Serial.flush();
      }

      //Check for \n or ',' to terminate current command
      if (ASCII_BUFFER[index] == '\n' ||
          //ASCII_BUFFER[index] == '\r' ||
          ASCII_BUFFER[index] == ',')
      {
        if (ECHO && ASCII_BUFFER[index] != ',')
          Serial.println("");//HACK: more echo stuff
        ASCII_BUFFER[index] = '\0';
        break;
      }
      //Otherwise, just paste a null at the end for the next char
      //(Ignore/overwrite carraige returns)
      else if (ASCII_BUFFER[index] != '\r')
      {
        index++;
      }
      ASCII_BUFFER[index] = '\0';

      //wait for the next character
      now = millis();
      while (Serial.available() == 0 && millis()-now < SERIAL_READ_TIMEOUT)
      {
        //Let the RADIO have some time to push packets around
        A7105_Mesh_Update(&(RADIO));
      } 
      if (Serial.available() == 0)
        break;
    }

  //DEBUG: NUKEME
  /*if (index > 0)
  {
    Serial.print("DONE: ");
    Serial.println(ASCII_BUFFER);
  }*/

  //Did we fill our whole buffer looking for a command that
  //wasn't there? 
  if (index >= ASCII_BUFFER_SIZE - 1)
  {
    //Read till the next newline to reset for the next legit command
    while(Serial.available() > 0)
    {
      if (Serial.read() == '\n')
        break;     
    }
  }
  return index;
}


// the loop function runs over and over again forever
void loop() {

  //Handle host commands
  if (!HOST_LISTENING)
  {
    get_serial_cmd();

    //Look for legitimate commands

    //ECHO
    if (strcmp_P(ASCII_BUFFER,ECHO_STR) == 0)
    {
      toggle_echo(); 
    }

    //JOIN
    else if (strcmp_P(ASCII_BUFFER,JOIN_STR) == 0)
    {
      join_mesh(); 
    }

    //RESET
    else if (strcmp_P(ASCII_BUFFER,RESET_STR) == 0)
    {
      init_radio_and_broadcast_cache(); 
    }

    //PING
    else if (strcmp_P(ASCII_BUFFER,PING_STR) == 0)
    {
      ping_mesh();
    }

    //LISTEN
    else if (strcmp_P(ASCII_BUFFER,LISTEN_STR) == 0)
    {
      start_listening();
    }

    //LIST REGISTERS (all nodes on mesh)
    else if (strcmp_P(ASCII_BUFFER,LIST_REGISTERS_STR) == 0)
    {
      list_registers();
    }

    //GET_REGISTER
    else if (strcmp_P(ASCII_BUFFER,GET_REGISTER_STR) == 0)
    {
      //NOTE: The rest of the command is read in the function
      get_register();
    }

    //SET_REGISTER
    else if (strcmp_P(ASCII_BUFFER,SET_REGISTER_STR) == 0)
    {
      //NOTE: The rest of the command is read in the function
      set_register();
    }

    //VALUE_BROADCAST
    else if (strcmp_P(ASCII_BUFFER,VALUE_BROADCAST_STR) == 0)
    {
      //NOTE: The rest of the command is read in the function
      value_broadcast();
    }

    //If there was anything in the buffer and it isn't recognized, return an error
    else if (strlen(ASCII_BUFFER))
    {
      unknown_command();
    }
  }

  //If the host is listening look for anything on the serial port
  else if (Serial.available() > 0)
  {
    HOST_LISTENING = 0;

    //purge the inbound buffer
    while (Serial.available()>0)
    {
      Serial.read();
    }
 
    //Output that we're ready
    SerialPrint_P(READY_STR);
    putstring("\r\n");
  }
 
  //Let the RADIO have some time to push packets around
  A7105_Mesh_Update(&(RADIO));
}
