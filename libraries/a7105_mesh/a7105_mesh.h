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

#ifndef _A7105_Mesh_MESH_H_
#define _A7105_Mesh_MESH_H_

#include <a7105.h>
#include "a7105_mesh_packet.h"

//Packet size (64 bytes is the Maximum supported in normal FIFO mode by the A7105)
//NOTE: This must be a multiple of 8 and at least 16 bytes (so 16, 32 or 64)
#define A7105_MESH_PACKET_SIZE 64

//Total available length for register name + value in a packet 
#define A7105_MESH_MAX_REGISTER_ARRAY_SIZE A7105_MESH_PACKET_SIZE - 7 //64 bytes - 7 needed for headers and sizes
#define A7105_MESH_MAX_REGISTER_PART_SIZE  A7105_MESH_MAX_REGISTER_ARRAY_SIZE - 2 //minimum size is max - 2 (one content byte and one length)

//Maximum number of packets to cache for repeating 
//(kept small to prevent using too much RAM)
#define A7105_MESH_MAX_REPEAT_CACHE_SIZE 2

//Maximum number of packet header/sequence/unique sets 
//to keep for avoiding reacting to previously seen (and reacted-to) packets
//IMPORTANT: if this ever goes above 16, make the sequence number a whole byte!

#define A7105_MESH_HANDLED_PACKET_CACHE_LENGTH 8 

//if we see a packet with a sequence number 3 or less below the 
//one in the cache, we can "expire" that cache item so we don't 
//ignore legitimate packets that have occurred since the sequence 
//number rolled over
#define A7105_MESH_HANDLED_PACKET_CACHE_SEQ_EXPIRE 3 

//Maximum number of times a packet can be repeated on the mesh
#define A7105_MESH_MAX_HOP_COUNT 6

/////////Join process constants (milliseconds)///////////

#define A7105_MESH_JOIN_ACCEPT_DELAY 5000 //Time from sending the first JOIN packet to believing we're OK to join

//These define the window for the randomly selected frequency
//at which we broadcast JOIN packets during the joining process
#define A7105_MESH_MIN_JOIN_RETRANSMIT_DELAY 100 
#define A7105_MESH_MAX_JOIN_RETRANSMIT_DELAY 600

//Time after which we will assume we've heard from everybody
//on the mesh
#define A7105_MESH_PING_TIMEOUT 4000 

//Time after which directed requests will be considered timed-out
#define A7105_MESH_REQUEST_TIMEOUT 2000 

//Max number of response repeats we have going on at once
//#define A7105_MESH_RESPONSE_REPEAT_CACHE_SIZE 4
#define A7105_MESH_RESPONSE_CACHE_SIZE 4

//Bytes in a response repeat cache item (don't change)
#define A7105_MESH_RESPONSE_REPEAT_ITEM_SIZE 4

//Max number of times to repeat a particular response
#define A7105_MESH_RESPONSE_MAX_REPEAT 6 

//Max number of times to repeat the current request
#define A7105_MESH_REQUEST_MAX_REPEAT 4

//Indexes for items in the response repeat cache
#define A7105_MESH_RESPONSE_REPEAT_OP 0
#define A7105_MESH_RESPONSE_REPEAT_REP_COUNT 1
#define A7105_MESH_RESPONSE_REPEAT_SEQ 2
#define A7105_MESH_RESPONSE_REPEAT_DATA 3

//Debug stuff
//#define A7105_MESH_DEBUG

#ifdef A7105_MESH_DEBUG
#define A7105_Mesh_SerialDump(x) SerialDump_P(PSTR(x))
#endif

//Error messages to include with SET_REGISTER packets
//when the callback doesn't work correctly
//(The PROGMEM stuff and #defines are a bunch of
//shenanegans to avoid eating up extra RAM with 
//strings. The memory is tight on these little AVR chips...)
#define SET_REG_CB_NO_ERROR_SET 0
#define SET_REG_CB_BOGUS_RETURN 1
#define SET_REG_INVALID_VALUE_SIZE 2
const char ERR_STR1[] PROGMEM = "Unknown Error (not set)";
const char ERR_STR2[] PROGMEM = "Remote Callback Error";
const char ERR_STR3[] PROGMEM = "Invalid Value Size";

const char* const A7105_ERROR_STRINGS[] PROGMEM = {ERR_STR1, ERR_STR2, ERR_STR3};


enum A7105_Mesh_State{
  A7105_Mesh_NOT_JOINED,
  A7105_Mesh_JOINING,
  A7105_Mesh_IDLE,
  A7105_Mesh_PING,
  A7105_Mesh_GET_NUM_REGISTERS,
  A7105_Mesh_GET_REGISTER_NAME,
  A7105_Mesh_GET_REGISTER,
  A7105_Mesh_SET_REGISTER,
};

enum A7105_Mesh_Status{
  A7105_Mesh_NO_STATUS,
  A7105_Mesh_STATUS_OK,
  A7105_Mesh_TIMEOUT,
  A7105_Mesh_INVALID_REGISTER_INDEX,
  A7105_Mesh_INVALID_REGISTER_LENGTH,
  A7105_Mesh_INVALID_REGISTER_RETURNED,
  A7105_Mesh_INVALID_REGISTER_VALUE, //returned by remote node
  A7105_Mesh_NOT_ON_MESH,
  A7105_Mesh_BUSY,
  A7105_Mesh_MESH_FULL,
  A7105_Mesh_MESH_ALREADY_JOINING,
  A7105_Mesh_RADIO_INIT_ERROR,
  A7105_Mesh_RADIO_CALIBRATION_ERROR,
  A7105_Mesh_RADIO_INVALID_CHANNEL,
  A7105_Mesh_AUTO_SET_REGISTER,
};

struct A7105_Mesh_Register
{
  //Array used to store register name and data. Total must be <= MAX_ARRAY_SIZE;
  byte _data[A7105_MESH_MAX_REGISTER_ARRAY_SIZE];
  byte _name_len; 
  byte _data_len;
  
  //Flag that indicates _data is a null terminated string instead of the segmented name/data pair
  byte _error_set; //0 = false, 1 = _data is an ascii error string 

  /*
    Callback for when a node receives a "SET_REGISTER" request for this register.
    If this is NULL, the value passed in a SET_REGISTER request will be assigned
    verbatim.

    This callback should return:
      * A7105_Mesh_STATUS_OK if the register was updated successfully.
      * A7105_Mesh_INVALID_REGISTER_VALUE if there was an error setting the register
        NOTE: If this is returned, A7105_Mesh_Register_Set_Error() MUST be called 
              to populate the register with an error.
      * A7105_Mesh_AUTO_SET_REGISTER If the callback would just like the register to
                                     to be set automatically (i.e. callback is just
                                     a notifier).
  */
  A7105_Mesh_Status (*set_callback)(struct A7105_Mesh_Register*,struct A7105_Mesh* ,void*);

  /*
    Callback for when a node receives a "GET_REGISTER" request for this register.
    If this is NULL, the value passed in a GET_REGISTER request will be assigned
    verbatim.

    This callback should return:
      * A7105_Mesh_STATUS_OK if the register value in the first parameter is acceptable
                             to return (i.e. it has been set if it is dynamic).
      * (other returns for future versions)
  */
  A7105_Mesh_Status (*get_callback)(struct A7105_Mesh_Register*,void*);
};

/*
  A7105_Mesh_Register_Initialize:
    * reg: pointer to an existing A7105_Mesh_Register object
    * set_callback: A function to call whenever the register in question 
                    has a SET_REGISTER request made to it via the containing A7105_Mesh node 
                    or NULL if the register should be set verbatim via mesh requests.
    * get_callback:

    Side-Effects/Notes: See the descriptions above for get/set callbacks to 
                        understand how they should behave and what they should return
                        if you implement your own.

    This function initializes a A7105_Mesh_Register structure. It zero's the data
    portions and sets get/set callbacks (optionally).
*/
void A7105_Mesh_Register_Initialize(struct A7105_Mesh_Register* reg,
                                    A7105_Mesh_Status (*set_callback)(struct A7105_Mesh_Register*,struct A7105_Mesh*,void*),
                                    A7105_Mesh_Status (*get_callback)(struct A7105_Mesh_Register*,void*));

void A7105_Mesh_Register_Set_Error(struct A7105_Mesh* node, const char* error_msg);

const char* A7105_Mesh_Register_Get_Error(struct A7105_Mesh* node);

void _A7105_Mesh_Register_Clear_Error(struct A7105_Mesh_Register* reg);

//Returns the length (in bytes) avialable for the register value or 0 on error.
byte A7105_Mesh_Util_SetRegisterNameStr(struct A7105_Mesh_Register* reg,
                                        const char* name);

//Returns true or false based on success
byte A7105_Mesh_Util_SetRegisterValueStr(struct A7105_Mesh_Register* reg,
                                        const char* value);

//Returns true or false based on success
byte A7105_Mesh_Util_SetRegisterValueU32(struct A7105_Mesh_Register* reg,
                                         const uint32_t value);


//Returns length of register name
//Will return either the whole name or buffer_len -1 bytes (needs to pad a trailing /0)
byte A7105_Mesh_Util_GetRegisterNameStr(struct A7105_Mesh_Register* reg,char* buffer,int buffer_len);
byte A7105_Mesh_Util_GetRegisterValueStr(struct A7105_Mesh_Register* reg,char* buffer,int buffer_len);
byte A7105_Mesh_Util_GetRegisterValueU32(struct A7105_Mesh_Register* reg,uint32_t* dest);

int A7105_Mesh_Util_RegisterNameCmp(struct A7105_Mesh_Register* a, struct A7105_Mesh_Register* b);

void A7105_Mesh_Register_Copy(struct A7105_Mesh_Register* dest, struct A7105_Mesh_Register* src);

struct A7105_Mesh
{
  struct A7105 radio;
  A7105_Mesh_State state; //track what we're doing  

  uint16_t unique_id; //unique id salt for this node (generated at init-time)
  byte node_id;
  
  uint16_t random_delay; //used for repeating operations
  
  struct A7105_Mesh_Register* registers; //registers we serve
  byte num_registers; //number of registers

  //////// Request Tracking //////////
  byte packet_cache[A7105_MESH_PACKET_SIZE];
  unsigned long request_sent_time;
  byte target_node_id;
  uint16_t target_unique_id;
  byte target_register_index;

  //////// Response Tracking /////////
  //byte last_request_handled[A7105_MESH_PACKET_SIZE];
  //unsigned long last_request_handled_time;
  byte handled_packet_cache[A7105_MESH_HANDLED_PACKET_CACHE_LENGTH][A7105_MESH_HANDLED_PACKET_HEADER_SIZE];
  byte last_handled_packet_index;
  byte sequence_num; //Send by requestors to identify their packets

  //////// Join State Tracking ////////
  unsigned long join_first_tx_time;
  uint16_t join_retransmit_delay;

  /////// Presence Table /////////
  byte presence_table[32]; //bit mask for mesh node-id state

  ////// Repeater State Tracking ///////
  byte repeat_cache[A7105_MESH_MAX_REPEAT_CACHE_SIZE][A7105_MESH_PACKET_SIZE];  
  byte repeat_cache_start;
  byte repeat_cache_end;
  byte repeat_cache_size;
  unsigned long last_repeat_sent_time;

  ///// Client Data Storage Cache //////
  void* client_context_obj; 
  byte num_registers_cache; 
  A7105_Mesh_Register register_cache; //HACK: used as cache for GET_REGISTER/SET_REGISTER requests
  byte responder_node_id; 
  uint16_t responder_unique_id; 
  void (*operation_callback)(struct A7105_Mesh*,A7105_Mesh_Status,void*);
  A7105_Mesh_Status blocking_operation_status; //Used to store the return status during blocking interface usage

  ///// Register Value Broadcast Handler ///////
  void (*register_value_broadcast_callback)(struct A7105_Mesh*,void*);
  A7105_Mesh_Register* broadcast_cache; //HACK: maintained by client so we don't have to waste 150bytes of ram 
                                        //      if broadcasting isn't used
  

  ///// Auto Rejoin State Cache //////
  void (*pending_operation_callback)(struct A7105_Mesh*,A7105_Mesh_Status,void*);
  A7105_Mesh_State pending_state;
  byte pending_target_node_id;
  byte pending_target_unique_id;
  
  //HACK: We dual purpose 'pending_request_cache' for the request repeater
  //      It's kinda ugly, but necessary to avoid having yet-another-cache
  //      eating more RAM.  
  //      The original purpose of this cache is to store the current running request
  ///     while we have to re-join the mesh as a different ID.
  byte pending_request_cache[A7105_MESH_PACKET_SIZE];
  byte pending_operation;  //true means the packet_cache contains a queued operation to repeat 
                           //due to a re-join from a node-id conflict


  ///// Request Repeating State ///// (repeats request packets a few times to overcome packet collision/loss)
  unsigned long last_request_repeat_sent_time;
  byte request_repeat_count;

  ///// Response Repeating State ///// (repeats responses directed to us a few times to overcome packet collision/loss)
  //Response repeater cache layout
  //Byte 0     |    Byte 1    |    Byte 2 | Byte 3
  //OPERATION  | REPEAT-COUNT |     SEQ   |    DATA (specific to operation)

  //DATA byte meanings:
  // NUM_REGISTERS: NULL
  // PONG: NULL
  // REGISTER_NAME: Register index
  // REGISTER_VALUE: Register index
  // SET_REGISTER_ACK: target node ID 
  byte response_repeat_cache[A7105_MESH_RESPONSE_CACHE_SIZE][A7105_MESH_RESPONSE_REPEAT_ITEM_SIZE];
  byte response_repeat_cache_start;
  byte response_repeat_cache_end;
  byte response_repeat_cache_size;
  unsigned long last_response_repeat_sent_time;
 };


void A7105_Mesh_Set_Context(struct A7105_Mesh* node,void* context_obj);

/*
  A7105_Mesh_Initialize:
    See notes for A7105_Easy_Setup_Radio for details on 
    parameters not covered here.
    
    * unconnected_analog_pin: Pin number of unconnected ADC pin for random-seed generation.


 This function just set's up the hardware and initial structures for
 a A7105 radio to be used on a mesh, no meshes are joined.
 

  Returns:
    * A7105_Mesh_STATUS_OK: If the radio is successfully initialized
    * A7105_Mesh_RADIO_INIT_ERROR: If we can't get any data back from the radio.
                        This usually means a SPI error since we just
                        read back the clock settings as a sanity check 
                        to make sure the radio isn't just a black hole.
    * A7105_Mesh_RADIO_CALIBRATION_ERROR: If the auto-calibration fails for the 
                               chip. A7105's have to be auto-calibrated
                               every time they're reset or powered on
                               and that is done in this function.
    * A7105_Mesh_RADIO_INVALID_CHANNEL: If the specified channel is outside
                             the allowable range of 0-A8 (given our
                             pre-set and datasheet recommended channel
                             steps).
 
*/
A7105_Mesh_Status A7105_Mesh_Initialize(struct A7105_Mesh* node, 
                                        int chip_select_pin,
                                        int wtr_pin,
                                        uint32_t mesh_id,
                                        A7105_DataRate data_rate,
                                        byte channel,
                                        A7105_TxPower power,
                                        int unconnected_analog_pin);


/*
    A7105_Mesh_Status A7105_Mesh_Join(struct A7105_Mesh* node, void (*join_finished_callback)(A7105_Mesh_Status))
      * node: Must be a node successfully initialized with A7105_Mesh_Initialize()
      * join_finished_callback: The function called when the JOIN operation completes or times out. If this is NULL,                             
                                A7105_Mesh_Join will use an internal callback and block until the operation completes 
                                or times out. 

    Returns
      * A7105_Mesh_STATUS_OK on success or if ping_finished_callback is NULL.
      * A7105_Mesh_MESH_FULL if the mesh already has 255 nodes.     
*/
A7105_Mesh_Status A7105_Mesh_Join(struct A7105_Mesh* node, 
                                  void (*join_finished_callback)(struct A7105_Mesh*,A7105_Mesh_Status,void*));

A7105_Mesh_Status A7105_Mesh_Join(struct A7105_Mesh* node, 
                                  byte node_id,
                                  void (*join_finished_callback)(struct A7105_Mesh*,A7105_Mesh_Status,void*));

A7105_Mesh_Status A7105_Mesh_Join(struct A7105_Mesh* node, 
                                  byte node_id,
                                  void (*join_finished_callback)(struct A7105_Mesh*,A7105_Mesh_Status,void*),
                                  byte interrupting);

void _A7105_Mesh_Update_Join(struct A7105_Mesh* node);

//Called when we get a CONFLICT_NAME packet. Packet data is
//in packet_cache.
void _A7105_Mesh_Handle_Conflict_Name(struct A7105_Mesh* node);

//Called when we're joining and we see another node with a higher
//unique ID doing the same
void _A7105_Mesh_Handle_Deferred_Joining(struct A7105_Mesh* node);

//Called for all incoming packets to be processed to send out 
//CONFLICT_NAME packets if any node-ID conflicts are seen that
//defer to us as the node that should stay
//Returns: True if we sent a CONFLICT_NAME packet, false otherwise
byte _A7105_Mesh_Check_For_Node_ID_Conflicts(struct A7105_Mesh* node);
/*
  TODO: Describeme
*/
A7105_Mesh_Status A7105_Mesh_Update(struct A7105_Mesh* node);


/*
    A7105_Mesh_Status A7105_Mesh_Ping(struct A7105_Mesh* node, void (*ping_finished_callback)(A7105_Mesh_Status))
      * node: Must be a node successfully initialized with A7105_Mesh_Initialize() and the node must be successfully
              joined to a mesh.
      * ping_finished_callback: The function called when the PING operation completes or times out. If this is NULL,
                                A7105_Mesh_Ping will use an internal callback and block until the operation completes
                                or times out.

      Side-Effects/Notes: If this method completes, the contents of node->presence_table will
                          be up to date (this is a 256 bit bitmask of presence bits for every node ID on the mesh).
                          Otherwise, an error status will be returned or sent to the callback (if specified).

      Returns:
        * A7105_Mesh_STATUS_OK on success or if ping_finished_callback is NULL.
        * A7105_Mesh_NOT_ON_MESH if 'node' is not on a mesh
        * A7105_Mesh_BUSY if 'node' is currently performing a different operation
*/
A7105_Mesh_Status A7105_Mesh_Ping(struct A7105_Mesh* node, 
                                  void (*ping_finished_callback)(struct A7105_Mesh*,A7105_Mesh_Status,void*));

void _A7105_Mesh_Handle_Ping(struct A7105_Mesh* node);
void _A7105_Mesh_Handle_Pong(struct A7105_Mesh* node);
void _A7105_Mesh_Update_Ping(struct A7105_Mesh* node);

/*
    A7105_Mesh_Status A7105_Mesh_GetNumRegisters(struct A7105_Mesh* node, byte node_id, void (*get_num_registers_callback)(A7105_Mesh_Status))
      * node: Must be a node successfully initialized with A7105_Mesh_Initialize() and the node must be successfully
              joined to a mesh.
      * node_id: the node ID to query
      * target_unique_id: (Optional) filter responses with unique ID to ensure we don't get 
                                     responses from a node that usurped node_id from the last
                                     request to now.
      * get_num_registers_callback: The function called when the GET_NUM_REGISTERS operation completes or times out. If this is NULL,
                                    A7105_Mesh_GetNumRegisters will use an internal callback and block until the operation completes
                                    or times out.

      Side-Effects/Notes: If this method completes successfully, the number of registers will be stored in node->num_registers_cache.
                          Otherwise, an error status will be returned or sent to the callback (if specified).
                          Also, responder node-ID and unique-ID will be stored in node->responder_node_id and 
                          resonder->responder_unique_id respectively. Also, the num_registers_cache will remain usable by the client
                          until the next call to GetNumRegisters.

      Returns:
        * A7105_Mesh_STATUS_OK on success or if get_num_registers_callback is NULL.
        * A7105_Mesh_TIMEOUT if the operation finished without getting any response from the target node.
        * A7105_Mesh_BUSY if 'node' is currently performing a different operation
        
*/
A7105_Mesh_Status A7105_Mesh_GetNumRegisters(struct A7105_Mesh* node, 
                                             byte node_id, 
                                             void (*get_num_registers_finished_callback)(struct A7105_Mesh*,A7105_Mesh_Status,void*)); 
A7105_Mesh_Status A7105_Mesh_GetNumRegisters(struct A7105_Mesh* node, 
                                             byte node_id, 
                                             uint16_t target_unique_id,
                                             void (*get_num_registers_finished_callback)(struct A7105_Mesh*,A7105_Mesh_Status,void*)); 

void _A7105_Mesh_Handle_GetNumRegisters(struct A7105_Mesh* node);
void _A7105_Mesh_Handle_NumRegisters(struct A7105_Mesh* node);

/*
    A7105_Mesh_Status A7105_Mesh_GetRegisterName(struct A7105_Mesh* node, byte node_id, byte reg_index, uint16_t filter_unique_id, void (*get_register_name_callback)(A7105_Mesh_Status))
      * node: Must be a node successfully initialized with A7105_Mesh_Initialize() and the node must be successfully
              joined to a mesh.
      * node-id: 0-255 node ID of the target node.
      * reg_index: Index of the register to get. This value must be 0-(len-1) where len is the number of registers served by the target node.
      * filter_unique_id: If non-zero, this value is used to ensure we're getting responses from a particular unique_id. The method
                          will work without this value. However, specifying a unique ID will ensure we don't get names from
                          a different node by the same name (this is a good idea to specify for nodes that move a bunch and have
                          connectivity problems).
      * get_register_name_callback: The function called when the GET_REGISTER_NAME operation completes or times out. If this is NULL,
                                    A7105_Mesh_GetRegisterName will use an internal callback and block until the operation completes
                                    or times out.

      Side-Effects/Notes: If this method completes successfully, the register name returned can be retrived by
                          calling A7105_Mesh_Util_GetRegisterNameBytes() or A7105_Mesh_Util_GetRegisterNameStr().
                          with node->register_cache.
                          NOTE: These values are stored internally in the A7105_Node structure and will be reset at the next
                          register operation.
                          Otherwise, an error status will be returned or sent to the callback (if specified).

      Returns:
        * A7105_Mesh_STATUS_OK on success or if get_num_registers_callback is NULL.
        * A7105_Mesh_INVALID_REGISTER_INDEX if the target register doesn't serve a register with the specified index.
        * A7105_Mesh_TIMEOUT if the operation finished without getting any response from the target node.
        * A7105_Mesh_BUSY if 'node' is currently performing a different operation
*/
A7105_Mesh_Status A7105_Mesh_GetRegisterName(struct A7105_Mesh* node, 
                                             byte node_id, 
                                             byte reg_index, 
                                             void (*get_register_name_finished_callback)(struct A7105_Mesh*,A7105_Mesh_Status,void*)); 

A7105_Mesh_Status A7105_Mesh_GetRegisterName(struct A7105_Mesh* node, 
                                             byte node_id, 
                                             byte reg_index, 
                                             uint16_t filter_unique_id,
                                             void (*get_register_name_finished_callback)(struct A7105_Mesh*,A7105_Mesh_Status,void*)); 

void _A7105_Mesh_Handle_GetRegisterName(struct A7105_Mesh* node);
void _A7105_Mesh_Handle_RegisterName(struct A7105_Mesh* node);


/*
    A7105_Mesh_Status A7105_Mesh_GetRegister(struct A7105_Mesh* node, struct A7105_Mesh_Register* reg, void (*get_register_callback)(A7105_Mesh_Status))
      * node: Must be a node successfully initialized with A7105_Mesh_Initialize() and the node must be successfully
              joined to a mesh.
      * reg: Points to a register whose name is the name for which we're searching. This can be set using the utility method A7105_Mesh_Util_SetRegisterNameStr().
      * get_register_callback: The function called when the GET_REGISTER operation completes or times out. If this is NULL,
                               A7105_Mesh_GetRegister will use an internal callback and block until the operation completes
                               or times out.

      Side-Effects/Notes: If this method completes successfully, the register name/value returned can be retrived by
                          calling A7105_Mesh_Util_GetRegisterNameBytes() or A7105_Mesh_Util_GetRegisterNameStr() for the name and
                          A7105_Mesh_UtilGetRegisterValueBytes() or A7105_Mesh_Util_GetRegisterValueStr() for the value.
                          NOTE: These values are stored internally in the A7105_Node structure and will be reset at the next
                          register operation.
                          Otherwise, an error status will be returned or sent to the callback (if specified).

      Returns:
        * A7105_Mesh_STATUS_OK on success or if get_num_registers_callback is NULL.
        * A7105_Mesh_TIMEOUT if the operation finished without getting any response from the target node.
        * A7105_Mesh_BUSY if 'node' is currently performing a different operation
        * A7105_Mesh_INVALID_REGISTER_LENGTH if the specified register name is either too long (or zero length).
*/
A7105_Mesh_Status A7105_Mesh_GetRegister(struct A7105_Mesh* node, 
                                         struct A7105_Mesh_Register* reg, 
                                         void (*get_register_finished_callback)(struct A7105_Mesh*,A7105_Mesh_Status,void*)); 

void _A7105_Mesh_Handle_GetRegister(struct A7105_Mesh* node);
void _A7105_Mesh_Handle_RegisterValue(struct A7105_Mesh* node);

/*
  DESCRIBEME

  Generic function to handle operation timeouts for GET_*,SET_* and puts us back at IDLE
  and calls the operation callback
*/
void _A7105_Mesh_Check_For_Timeout(struct A7105_Mesh* node,
                                   A7105_Mesh_State operation,
                                   int timeout);


/*
    A7105_Mesh_Status A7105_Mesh_SetRegister(struct A7105_Mesh* node, byte* register_name, byte register_name_len,
                                                                      byte* register_value, byte register_value_len,
                                                                      void (*set_register_callback)(A7105_Mesh_Status))
      * node: Must be a node successfully initialized with A7105_Mesh_Initialize() and the node must be successfully
              joined to a mesh.
      * register_name: Byte array of length register_name_len).
      * register_name_len: Length of the array passed as 'register_name' in bytes.
      * register_value: Byte array of length register_value_len).
      * register_value_len: Length of the array passed as 'register_value' in bytes.
      * set_register_callback: The function called when the GET_REGISTER operation completes or times out. If this is NULL,
                               A7105_Mesh_GetRegister will use an internal callback and block until the operation completes
                               or times out.

      Side-Effects/Notes: If this method completes successfully, the register specified will have the value specified.
                          This does not garantee it will be that value during the next read, but does gaurantee that
                          it has been set successfully.
                          Otherwise, an error status will be returned or sent to the callback (if specified).
    
                          'reg' is not accessed again after this function (writing to non-cache memory is too risky
                          since it could crash weirdly if we reference something that went out of scope).

      Returns:
        * A7105_Mesh_STATUS_OK on success or if get_num_registers_callback is NULL.
        * A7105_Mesh_TIMEOUT if the operation finished without getting any response from the target node.
        * A7105_Mesh_BUSY if 'node' is currently performing a different operation
        * A7105_Mesh_INVALID_REGISTER_LENGTH if the specified register name is either too long (or zero length).
        * A7105_Mesh_INVALID_REGISTER_VALUE if the node managing the register would not except the value sent
*/
A7105_Mesh_Status A7105_Mesh_SetRegister(struct A7105_Mesh* node, 
                                         struct A7105_Mesh_Register* reg,
                                         void (*set_register_finished_callback)(struct A7105_Mesh*,A7105_Mesh_Status,void*)); 

void _A7105_Mesh_Handle_SetRegister(struct A7105_Mesh* node);
void _A7105_Mesh_Handle_SetRegisterAck(struct A7105_Mesh* node);

/*
  A7105_Mesh_Broadcast_Listen:
    * node: A node that has been previously initialized with A7105_Mesh_Initialize()
    * callback: The function to call whenever this node receives a Register Value broadcast.
    * reg: The register to use as a cache for broadcast packets

  Side-Effects/Notes:
    * The value being broadcast is stored in node->broadcast_cache when 'callback' is called
    * The node *may* be involved in other operations when 'callback' is called so 
      it's best to not depend on sending requests from 'callback'
    * Important! The callback specified may be called multiple times
      if repeated packets come in with the same value. 
    * 'reg' must remain in scope as long as the node is listening since 
      node->broadcast_cache is simply a pointer to it (they can be used interchangeably).

    * To stop listening, just call with NULL,NULL for callback,reg


  This function registers a callback to call when a REGISTER_VALUE packet with a target of 0
  is seen on the mesh (that is a broadcast value). 
    
*/
void A7105_Mesh_Broadcast_Listen(struct A7105_Mesh* node,
                                void (*callback)(struct A7105_Mesh*,void*),
                                struct A7105_Mesh_Register* reg);

void _A7105_Mesh_Handle_RegisterValue_Broadcast(struct A7105_Mesh* node);

/*
  A7105_Mesh_Broadcast:
    * node: A node that has been previously initialized with A7105_Mesh_Initialize()
    * reg: A pointer to a register structure that has been initialized and contains a 
           name and value

    Side-Effects/Notes: Broadcasting a register value is not a guarantee
                        that it will be sent or received correctly. If we
                        are currently not on a mesh (if we got kicked and have
                        to rejoin), the packet won't be sent. Also, since this
                        packet doesn't receive any ACK, we might just be sending
                        it to the ether. *Don't* use this for anything that 
                        is important.

    Returns:  
      * A7105_Mesh_NOT_ON_MESH: if node is not connected to a mesh
      * A7105_Mesh_STATUS_OK: if the broadcast register was sent
      * A7105_Mesh_INVALID_REGISTER_VALUE: If 'reg' wasn't a legitimate register or was 
                                           too long to fit in a packet.
*/
A7105_Mesh_Status A7105_Mesh_Broadcast(struct A7105_Mesh* node, struct A7105_Mesh_Register* reg);

//////////////////////// Utility Functions ///////////////////////
/*
  A7105_Get_Next_Present_Node:
    * node: The A7105 node whose internal presence bitmask to use in finding the
            next active node from the last PING
    * start: the node number to start (non-inclusive) Use 0 to start at the beginning.

    Returns: The node-id of the next node (after 'start') present on the mesh or 0 if no more nodes
             were found.

  This utility function can be used to iterate over a presence table and get every node ID. Simply 
  start at 0 and pass the last returned ID on each iteration until 0 is returned.
*/
byte A7105_Get_Next_Present_Node(struct A7105_Mesh* node, byte start);

/*
  _A7105_Mesh_Is_Node_Idle:
    * node: a node structure (intiailized)
  
  Returns:
    A7105_Mesh_NOT_ON_MESH: If the node is not connected to a mesh
    A7105_Mesh_BUSY: If the node is currently performing an operation
    A7105_Mesh_STATUS_OK: If the node is idle and on a mesh.
*/
A7105_Mesh_Status _A7105_Mesh_Is_Node_Idle(struct A7105_Mesh* node);

/*
  void _A7105_Mesh_Update_Repeats:
    * node: An initialized struct A7105_Mesh node

    This internal function is used manage the repeat
    cache and send repeater packets.
*/
void _A7105_Mesh_Update_Repeats(struct A7105_Mesh* node);

/*
  void _A7105_Mesh_Update_Response_Repeats:
    * node: An initialized struct A7105_Mesh node

    This internal function is used manage the response repeat
    cache and send repeating response packets (to overcome 
    packet collisions and other reasons for missed responses).
*/
void _A7105_Mesh_Update_Response_Repeats(struct A7105_Mesh* node);


/*
  void _A7105_Mesh_Update_Request_Repeats:
    * node: An initialized struct A7105_Mesh node

    This internal function is used manage the request repeat
    cache and send repeating request packets (to overcome 
    packet collisions and other reasons for missed requests).
*/
void _A7105_Mesh_Update_Request_Repeats(struct A7105_Mesh* node);

/*
  void _A7105_Mesh_Cache_Packet_For_Repeat:
    * node: An initialized struct A7105_Mesh node

    Side-Effect/Notes: The packet processed is the one sitting
    in node->packet_cache.

    This internal function assesses whether a received packet
    is applicable to put in the repeat cache and adds it if it
    is.
*/
void _A7105_Mesh_Cache_Packet_For_Repeat(struct A7105_Mesh* node);

//Appends node->packet_cache details into the response repeater cache
void _A7105_Mesh_Append_Response_Repeat(struct A7105_Mesh* node);

//Removes the first (oldest) response repeater cache item 
byte* _A7105_Mesh_Pop_Response_Repeat(struct A7105_Mesh* node);


//Compares two packets and returns if they're identical,
//but ignores the hop-count
byte _A7105_Mesh_Util_Is_Same_Packet_Sans_Hop(byte* a, byte* b);

/*
  void _A7105_Mesh_Prep_Packet_Header:
    * node: An initialized struct A7105_Mesh node
    * packet_type: The packet type to populate

    This internal function is used to prep a node's packet-cache 
    with identifying information.
*/
void _A7105_Mesh_Prep_Packet_Header(struct A7105_Mesh* node,
                                    byte packet_type);

/*
  void _A7105_Mesh_Send_Request:
    * node: A initialized struct A7105_Mesh node

    This internal function is used to push a response
    packet and set associated status for timers and
    interrupt/rejoin handling.

    Side-Effects/Notes:
      * the packet sitting in node->packet_cache is pushed to the radio
      * All the pending_* members are populated for this request (as 
        long as it isn't a JOIN, since that's the only interrupting
        operation).
      * The function blocks until the data is done writing at the radio
        and sends the STROBE command to listen again.
      * The sequence number for the node is incremented for the call
        to aid receivers in filtering seen packets
*/
void _A7105_Mesh_Send_Response(struct A7105_Mesh* node);

/*
  void _A7105_Mesh_Send_Request:
    * node: A initialized struct A7105_Mesh node

    This internal function is used to push a request
    packet and set associated status for timers and
    interrupt/rejoin handling.

    Side-Effects/Notes:
      * the packet sitting in node->packet_cache is pushed to the radio
      * the request_sent_time is set
      * _A7105_Mesh_Send_Response() is used to push the packet, 
        please review the docs for that function above.
*/
void _A7105_Mesh_Send_Request(struct A7105_Mesh* node);

//Same as request, but doesn't update the sent-time
void _A7105_Mesh_Send_Broadcast(struct A7105_Mesh* node);

//Returns true if the register name from 'reg' is in packet
//for GET/SET Register requests only
byte _A7105_Mesh_Util_Does_Packet_Have_Register(byte* packet,
                                                struct A7105_Mesh_Register* reg);

byte _A7105_Mesh_Util_Register_To_Packet(byte* packet,
                                         struct A7105_Mesh_Register* reg,
                                         byte include_value);

//Grabs the register index part of a REGISTER_NAME packet
byte _A7105_Mesh_Util_Get_Register_Index(byte* packet);

//Sets the register index of a REGISTER_NAME packet
byte _A7105_Mesh_Util_Set_Register_Index(byte* packet, byte index);

//NOTE: if include_value = true, include name is assumed true also
//Returns true on success, false for bogus values.
byte _A7105_Mesh_Util_Packet_To_Register(byte* packet,
                                         struct A7105_Mesh_Register* reg,
                                         byte include_value);

//Check the register name in the packet_cache against all serviced register 
//names and return the index of the match or -1 if not found
int _A7105_Mesh_Filter_RegisterName(struct A7105_Mesh* node);

//Returns true if the name is the same for reg and the name field in the packet
//false otherwise
byte _A7105_Mesh_Cmp_Packet_Register(byte* packet,
                                     struct A7105_Mesh_Register* reg,
                                     byte include_value); 

/*
  _A7105_Mesh_Handling_Request:
    * node: An initialized struct A7105_Mesh node
    * packet: The packet being handled (the one received)

    This internal function updates the node state to track
    the reqest being handled. This data is used to used to 
    filter duplicates in the packet handling so we don't respond
    to requests simply being repeated by other nodes.
*/
void _A7105_Mesh_Handling_Request(struct A7105_Mesh* node,
                                  byte* packet);

/*
  _A7105_Mesh_Update_Handled_Packet_Cache(struct A7105_Mesh* node):
    * node: An initialized struct A7105_Mesh node

    This internal function updatse the node state to expire any packets
    in the handled packet cache that have a sequence number that has been
    determined to be likely seen again soon (so we don't accidentially filter
    packets that are legitimate new requests with the same sequence number as
    old ones).
*/
void _A7105_Mesh_Update_Handled_Packet_Cache(struct A7105_Mesh* node);


/*
  _A7105_Mesh_Prep_Finishing_Callback:
    * node: An initialized struct A7105_Mesh node
    * user_finished_callback: NULL or a user-specified operation finished callback
    * blocking_finished_callback: The default library-provided callback to use if the user callback isn't specified.

  This internal function sets the node's operation callback and 
  updates the internal blocking callback status NO_STATUS. It's just
  a convenience function since we do this for every type of packet
  we send to handling blocking vs callback behavior.
*/
void _A7105_Mesh_Prep_Finishing_Callback(struct A7105_Mesh* node,
                                             void (*user_finished_callback)(struct A7105_Mesh*,A7105_Mesh_Status,void*),
                                             void (*blocking_finished_callback)(struct A7105_Mesh*,A7105_Mesh_Status,void*));

/*
  _A7105_Mesh_Handle_RX:
    * node: An initialized struct A7105_Mesh node

  This internal function checks for and processes received packets.
  It is the central hub for calling all the packet-specific handler
  functions.
*/
void _A7105_Mesh_Handle_RX(struct A7105_Mesh* node);

void _A7105_Mesh_Check_For_Node_ID_Conflict(struct A7105_Mesh* node);

/*
  A7105_Util_Get_Pkt_Unique_Id(byte* packet):
    * packet: A packet of length PACKET_LENGTH

  This utilty function returns a uint16_t representation
  of the unique_ID of the sender for 'packet'
*/
uint16_t A7105_Util_Get_Pkt_Unique_Id(byte* packet);

/*
  _A7105_Mesh_Filter_Packet:
    * node: An initialized struct A7105_Mesh node
    * packet_type: one of the packet type bytes defined in A7105_mesh_packet.h
    * treat_as_request: Boolean whether we're expect the packet in node->packet_cache
                        to be treated as a request (true) or response (false)

    Side-Effects/Notes:
      * This function filters the packet in node->packet_cache

    This internal function is used to filter incoming packets to either debounce
    incoming requests (so we only respond to a request once instead of 
    multiple times from the repeats of other nodes coming after the fact).

    Also, if the packet is not a request, this function will filter it 
    based on the senders node/unique ID using the node's target_node_id
    and target_unique_id fields (0 means don't filter for these).

    Returns: True if the packet is of the type specified and should be 
             processed. False otherwise.
*/
byte _A7105_Mesh_Filter_Packet(struct A7105_Mesh* node,
                               byte packet_type,
                               byte treat_as_request);

/*
  A7105_Mesh_Set_Registers:
    * node: An initialized struct A7105_Mesh node
    * regs: an array of A7105_Mesh_Registers that have been initialized and assigned names
    * num_regs: The length of 'regs'.

  Side-Effects/Notes:
    - The list regs replaces any other list managed by 'node.' 
    - Regs must not go out of scope for the lifetime of 'node' (it is not copied)
    - The names of the registers in 'regs' must be unique
*/
void A7105_Mesh_Set_Node_Registers(struct A7105_Mesh* node,
                              struct A7105_Mesh_Register* regs,
                              byte num_regs);

byte _A7105_Mesh_Get_Packet_Seq(byte* packet);

void _A7105_Mesh_Set_Packet_Seq(byte* packet, byte sequence);

byte _A7105_Mesh_Get_Packet_Hop(byte* packet);

void _A7105_Mesh_Set_Packet_Hop(byte* packet, byte hop);


byte _A7105_Bit_out(int analogPin); 

uint32_t A7105_Mesh_Get_Random_Seed(byte noOfBits, int analogPin);

#endif
