#ifndef _A7105_Mesh_MESH_H_
#define _A7105_Mesh_MESH_H_

#include <a7105.h>
#include "a7105_mesh_packet.h"

#define A7105_MESH_PACKET_SIZE 64 //64 bytes 
#define A7105_MESH_MAX_REGISTER_ARRAY_SIZE A7105_MESH_PACKET_SIZE - 7 //64 bytes - 7 needed for headers and sizes
#define A7105_MESH_MAX_REPEAT_CACHE_SIZE 3
#define A7105_MESH_MAX_HOP_COUNT 16

//Join process constants (milliseconds)
#define A7105_MESH_JOIN_ACCEPT_DELAY 1000
#define A7105_MESH_MIN_JOIN_RETRANSMIT_DELAY 100
#define A7105_MESH_MAX_JOIN_RETRANSMIT_DELAY 400

//Milliseconds 
#define A7105_MESH_MAX_RANDOM_DELAY 50
#define A7105_MESH_MIN_RANDOM_DELAY 10

#define A7105_MESH_REQUEST_HANDLING_DEBOUNCE 200 //milliseconds

#define A7105_MESH_PING_TIMEOUT 1000 

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
  A7105_Mesh_NOT_ON_MESH,
  A7105_Mesh_BUSY,
  A7105_Mesh_MESH_FULL,
  A7105_Mesh_MESH_ALREADY_JOINING,

  A7105_Mesh_RADIO_INIT_ERROR,
  A7105_Mesh_RADIO_CALIBRATION_ERROR,
  A7105_Mesh_RADIO_INVALID_CHANNEL,

};

struct A7105_Mesh_Register
{
  //Array used to store register name and data. Total must be <= MAX_ARRAY_SIZE;
  byte _data[A7105_MESH_MAX_REGISTER_ARRAY_SIZE];
  byte _name_len; 
  byte _data_len;
};

struct A7105_Mesh
{
  struct A7105 radio;
  A7105_Mesh_State state; //track what we're doing  

  uint16_t unique_id; //unique id salt for this node (generated at init-time)
  byte node_id;
  
  uint16_t random_delay; //used for repeating operations
  
  A7105_Mesh_Register* registers; //registers we serve
  byte num_registers; //number of registers

  //////// Request Tracking //////////
  unsigned long request_sent_time;
  byte target_node_id;
  uint16_t target_unique_id;

  //////// Response Tracking /////////
  byte last_request_handled[A7105_MESH_PACKET_SIZE];
  unsigned long last_request_handled_time;

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
  byte num_registers_cache; 
  byte packet_cache[A7105_MESH_PACKET_SIZE];
  void (*operation_callback)(struct A7105_Mesh*,A7105_Mesh_Status);
  A7105_Mesh_Status blocking_operation_status; //Used to store the return status during blocking interface usage

  ///// Auto Rejoin State Cache //////
  void (*pending_operation_callback)(struct A7105_Mesh*,A7105_Mesh_Status);
  A7105_Mesh_State pending_state;
  byte pending_target_node_id;
  byte pending_target_unique_id;
  byte pending_request_cache[A7105_MESH_PACKET_SIZE];
  byte pending_operation;  //true means the packet_cache contains a queued operation to repeat 
                           //due to a re-join from a node-id conflict

};

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
                                  void (*join_finished_callback)(struct A7105_Mesh*,A7105_Mesh_Status));

A7105_Mesh_Status A7105_Mesh_Join(struct A7105_Mesh* node, 
                                  byte node_id,
                                  void (*join_finished_callback)(struct A7105_Mesh*,A7105_Mesh_Status));


A7105_Mesh_Status A7105_Mesh_Join(struct A7105_Mesh* node, 
                                  byte node_id,
                                  void (*join_finished_callback)(struct A7105_Mesh*,A7105_Mesh_Status),
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
        * A7105_Mesh_TIMEOUT if the operation finished without getting any PONG responses from other nodes.
        * A7105_Mesh_NOT_ON_MESH if 'node' is not on a mesh
        * A7105_Mesh_BUSY if 'node' is currently performing a different operation
*/
A7105_Mesh_Status A7105_Mesh_Ping(struct A7105_Mesh* node, 
                                  void (*ping_finished_callback)(struct A7105_Mesh*,A7105_Mesh_Status));

void _A7105_Mesh_Handle_Ping(struct A7105_Mesh* node);
void _A7105_Mesh_Handle_Pong(struct A7105_Mesh* node);
void _A7105_Mesh_Update_Ping(struct A7105_Mesh* node);

/*
    A7105_Mesh_Status A7105_Mesh_GetNumRegisters(struct A7105_Mesh* node, byte node_id, void (*get_num_registers_callback)(A7105_Mesh_Status))
      * node: Must be a node successfully initialized with A7105_Mesh_Initialize() and the node must be successfully
              joined to a mesh.
      * get_num_registers_callback: The function called when the GET_NUM_REGISTERS operation completes or times out. If this is NULL,
                                    A7105_Mesh_GetNumRegisters will use an internal callback and block until the operation completes
                                    or times out.

      Side-Effects/Notes: If this method completes successfully, the number of registers will be stored in node->num_registers_cache.
                          Otherwise, an error status will be returned or sent to the callback (if specified).

      Returns:
        * A7105_Mesh_STATUS_OK on success or if get_num_registers_callback is NULL.
        * A7105_Mesh_TIMEOUT if the operation finished without getting any response from the target node.
*/
A7105_Mesh_Status A7105_Mesh_GetNumRegisters(struct A7105_Mesh* node, 
                                             byte node_id, 
                                             void (*get_num_registers_callback)(A7105_Mesh_Status));


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
                          NOTE: These values are stored internally in the A7105_Node structure and will be reset at the next
                          register operation.
                          Otherwise, an error status will be returned or sent to the callback (if specified).

      Returns:
        * A7105_Mesh_STATUS_OK on success or if get_num_registers_callback is NULL.
        * A7105_Mesh_INVALID_REGISTER_INDEX if the target register doesn't serve a register with the specified index.
        * A7105_Mesh_TIMEOUT if the operation finished without getting any response from the target node.
*/
A7105_Mesh_Status A7105_Mesh_GetRegisterName(struct A7105_Mesh* node, 
                                             byte node_id, 
                                             byte reg_index, 
                                             uint16_t filter_unique_id,
                                             void (*get_register_name_callback)(A7105_Mesh_Status));


/*
    A7105_Mesh_Status A7105_Mesh_GetRegister(struct A7105_Mesh* node, byte* register_name, byte register_name_len, void (*get_register_callback)(A7105_Mesh_Status))
      * node: Must be a node successfully initialized with A7105_Mesh_Initialize() and the node must be successfully
              joined to a mesh.
      * register_name: Byte array of length register_name_len).
      * register_name_len: Length of the array passed as 'register_name' in bytes.
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
*/
A7105_Mesh_Status A7105_Mesh_GetRegister(struct A7105_Mesh* node, 
                                         byte* register_name, 
                                         byte register_name_len, 
                                         void (*get_register_callback)(A7105_Mesh_Status));


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

      Returns:
        * A7105_Mesh_STATUS_OK on success or if get_num_registers_callback is NULL.
        * A7105_Mesh_TIMEOUT if the operation finished without getting any response from the target node.
*/
    A7105_Mesh_Status A7105_Mesh_SetRegister(struct A7105_Mesh* node, 
                                             byte* register_name, 
                                             byte register_name_len,
                                             byte* register_value, 
                                             byte register_value_len,
                                             void (*set_register_callback)(A7105_Mesh_Status));

//////////////////////// Utility Functions ///////////////////////

/*
  void _A7105_Mesh_Update_Repeats:
    * node: An initialized struct A7105_Mesh node

    This internal function is used manage the repeat
    cache and send repeater packets.
*/
void _A7105_Mesh_Update_Repeats(struct A7105_Mesh* node);

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

/*
  _A7105_Mesh_Prep_Pkt_Register_Name_Value:
    * packet: the packet to populate (assumed A7105_MESH_PACKET_SIZE length)
    * reg_name: the buffer containing the register name
    * reg_name_len: the length of reg_name
    * reg_val: the buffer containing the register value
    * reg_val_len: the length of reg_val

    This internal function populates a packet buffer with 
    register name and value data.

    Returns: 0 if the data was too long for the packet,
             1 otherwise.
*/
byte _A7105_Mesh_Prep_Pkt_Register_Name_Value(byte* packet,
                                              byte* reg_name,
                                              byte reg_name_len,
                                              byte* reg_val,
                                              byte reg_val_len);

//Returns true if the register name from 'reg' is in packet
//for GET/SET Register requests only
byte _A7105_Mesh_Util_Does_Packet_Have_Register(byte* packet,
                                                struct A7105_Mesh_Register* reg);

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
                                             void (*user_finished_callback)(struct A7105_Mesh*,A7105_Mesh_Status),
                                             void (*blocking_finished_callback)(struct A7105_Mesh*,A7105_Mesh_Status));

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

    Side-Effects/Nodes:
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
#endif
