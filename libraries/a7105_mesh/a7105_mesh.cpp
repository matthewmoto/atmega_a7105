#include <stdint.h>
#include <SPI.h>
#include "a7105_mesh.h"


void DebugHeader(struct A7105_Mesh* node)
{
  Serial.print(millis());
  putstring(" : (");
  Serial.print(node->unique_id,HEX);
  putstring(") ");
}
//////////////// A7105_Register Functions ////////////////////

void A7105_Mesh_Register_Initialize(struct A7105_Mesh_Register* reg,
                                    A7105_Mesh_Status (*set_callback)(struct A7105_Mesh_Register*,struct A7105_Mesh*,void*),
                                    A7105_Mesh_Status (*get_callback)(struct A7105_Mesh_Register*,void*))
{
  memset(reg->_data,0,A7105_MESH_MAX_REGISTER_ARRAY_SIZE);
  reg->_name_len = 0;
  reg->_data_len = 0;
  reg->_error_set = 0;

  reg->set_callback = set_callback;
  reg->get_callback = get_callback;
}

void A7105_Mesh_Register_Set_Error(struct A7105_Mesh* node, const char* error_msg)
{
  //Zero out the error buffer and copy the message, but truncate before we overrun the buffer
  memset(node->register_cache._data,0,A7105_MESH_MAX_REGISTER_ARRAY_SIZE);
  strncpy((char*)node->register_cache._data,error_msg,A7105_MESH_MAX_REGISTER_PART_SIZE-1);
  node->register_cache._error_set = true;
}

const char* A7105_Mesh_Register_Get_Error(struct A7105_Mesh_Register* reg)
{
  if (reg->_error_set)
    return (const char*)reg->_data;
  return NULL;
}

void _A7105_Mesh_Register_Clear_Error(struct A7105_Mesh_Register* reg)
{
  memset(reg->_data,0,A7105_MESH_MAX_REGISTER_ARRAY_SIZE);
  reg->_error_set = 0;
}

byte A7105_Mesh_Util_SetRegisterNameStr(struct A7105_Mesh_Register* reg,
                                        const char* name)
{
  //Bail if the name is too long
  if (A7105_MESH_MAX_REGISTER_PART_SIZE < strlen(name))
    return 0;

  reg->_name_len = (byte)strlen(name);
  memcpy(reg->_data,name,reg->_name_len);

  return (byte)A7105_MESH_MAX_REGISTER_PART_SIZE - reg->_name_len; 
}

byte A7105_Mesh_Util_SetRegisterValueStr(struct A7105_Mesh_Register* reg,
                                        const char* value)
{
  unsigned long max_len = A7105_MESH_MAX_REGISTER_ARRAY_SIZE - reg->_name_len;

  //Sanity check the length
  if (max_len == 0 || strlen(value) > (max_len-1))
    return false;

  //Put the string (including trailing null) behind the name in the buffer
  memcpy(&(reg->_data[reg->_name_len]),value,strlen(value)+1);
  reg->_data_len = strlen(value)+1; //include the trailing /0 in our calculations
  return true;
}

//Returns true or false based on success
byte A7105_Mesh_Util_SetRegisterValueU32(struct A7105_Mesh_Register* reg,
                                         const uint32_t value)
{
  unsigned long max_len = A7105_MESH_MAX_REGISTER_ARRAY_SIZE - reg->_name_len;

  //Sanity check the length
  if (max_len == 0 || 4 > max_len)
    return false;

  //Put the string (including trailing null) behind the name in the buffer
  int offset = reg->_name_len;
  reg->_data[offset++] = value >> 24 & 0xFF;
  reg->_data[offset++] = value >> 16 & 0xFF;
  reg->_data[offset++] = value >> 8 & 0xFF;
  reg->_data[offset++] = value & 0xFF;
  reg->_data_len = 4; //include the trailing /0 in our calculations
  return true;
}



byte A7105_Mesh_Util_GetRegisterNameStr(struct A7105_Mesh_Register* reg,
                                        char* buffer,
                                        int buffer_len)
{
  //cap our copy at 255 bytes and make sure we don't overrun the buffer
  buffer_len = (buffer_len > 0xFF) ? 0xFF : buffer_len;
  int bytes_to_copy = reg->_name_len > (buffer_len-1)?(buffer_len-1):reg->_name_len;

  //copy the name and slap a null to end the string
  memcpy(buffer,reg->_data,bytes_to_copy);
  buffer[bytes_to_copy] = 0;
  return (byte)bytes_to_copy;
}

byte A7105_Mesh_Util_GetRegisterValueStr(struct A7105_Mesh_Register* reg,
                                        char* buffer,
                                        int buffer_len)
{
  //cap our copy at 255 bytes and make sure we don't overrun the buffer
  buffer_len = (buffer_len > 0xFF) ? 0xFF : buffer_len;
  int bytes_to_copy = reg->_data_len > (buffer_len-1)?(buffer_len-1):reg->_data_len;

  //copy the name and slap a null to end the string
  memcpy(buffer,&(reg->_data[reg->_name_len]),bytes_to_copy);
  buffer[bytes_to_copy] = 0;
  return (byte)bytes_to_copy;
}

byte A7105_Mesh_Util_GetRegisterValueU32(struct A7105_Mesh_Register* reg,uint32_t* dest)
{
  //Sanity check the length
  if (reg->_data_len != 4)
  {
    return false;
  }

  //Put the string (including trailing null) behind the name in the buffer
  int offset = reg->_name_len;
  (*dest) = 0;
  for (int x = 0;x<4;x++)
  {
    (*dest) <<= 8;
    (*dest) |= reg->_data[offset+x];
  }
  return true;
}

void A7105_Mesh_Register_Copy(struct A7105_Mesh_Register* dest, struct A7105_Mesh_Register* src)
{
  dest->_name_len = src->_name_len;
  dest->_data_len = src->_data_len;
  memcpy(dest->_data,src->_data,A7105_MESH_MAX_REGISTER_ARRAY_SIZE);
  dest->_error_set = src->_error_set;
}

/////////////// A7105_Mesh Functions /////////////////////
void A7105_Mesh_Set_Context(struct A7105_Mesh* node,void* context_obj)
{
  node->client_context_obj = context_obj;
}


A7105_Mesh_Status A7105_Mesh_Initialize(struct A7105_Mesh* node, 
                                        int chip_select_pin,
                                        int wtr_pin,
                                        uint32_t mesh_id,
                                        A7105_DataRate data_rate,
                                        byte channel,
                                        A7105_TxPower power,
                                        int unconnected_analog_pin)
{

  //Try to set up the radio
  A7105_Status_Code ret = A7105_Easy_Setup_Radio(&(node->radio),
                                                 chip_select_pin,
                                                 wtr_pin,
                                                 mesh_id,
                                                 data_rate,
                                                 channel,
                                                 power,
                                                 1,1);
  //Set up our random generator seed
  //HACK: set up randomSeed 0 so our ID's stay consistent in debug
  //NUKEME for production
  //randomSeed(analogRead(unconnected_analog_pin));
  randomSeed(0);
  random(1024);

  //Init the node state
  node->state = A7105_Mesh_NOT_JOINED;
  //DEBUG
  //node->registers = NULL;
  node->unique_id = (uint16_t)(random(0xFFFF) + 1); //1-0xFFFF unique ID. 0 means uninitialized
;
  node->random_delay = (uint16_t)(random(A7105_MESH_MIN_RANDOM_DELAY,A7105_MESH_MAX_RANDOM_DELAY));

  //request tracking variables
  node->request_sent_time = 0;
  node->target_node_id = 0; //0 is a special variable meaning "not set"
  node->target_unique_id = 0; // (ditto)

  //response tracking variables
  memset(node->last_request_handled,0,A7105_MESH_PACKET_SIZE);
  node->last_request_handled_time = 0;

  //Join tracking
  node->join_retransmit_delay = (uint16_t)random(A7105_MESH_MIN_JOIN_RETRANSMIT_DELAY,
                                                 A7105_MESH_MAX_JOIN_RETRANSMIT_DELAY);
  node->join_first_tx_time = 0;

  //Presence Table
  memset(node->presence_table,0,32);

  //Repeater state
  node->repeat_cache_start = 0;
  node->repeat_cache_end = 0;
  node->repeat_cache_size = 0;
  node->last_repeat_sent_time = 0;
  for (int x = 0; x < A7105_MESH_MAX_REPEAT_CACHE_SIZE;x++)
    memset(node->repeat_cache[x],0,A7105_MESH_PACKET_SIZE);

  //Client Data Storage Cache
  node->client_context_obj = NULL;
  node->num_registers_cache = 0;
  memset(node->packet_cache,0,A7105_MESH_PACKET_SIZE);
  node->operation_callback = NULL;
  node->blocking_operation_status = A7105_Mesh_NO_STATUS;

  //Pending Operation state
  node->pending_operation = 0;

  //Start listening for packets
  A7105_Easy_Listen_For_Packets(&(node->radio), A7105_MESH_PACKET_SIZE);

  switch(ret)
  {
    case A7105_INIT_ERROR:
      return A7105_Mesh_RADIO_INIT_ERROR;
    case A7105_CALIBRATION_ERROR:
      return A7105_Mesh_RADIO_CALIBRATION_ERROR;
    case A7105_INVALID_CHANNEL:
      return A7105_Mesh_RADIO_INVALID_CHANNEL;
    default:
      return A7105_Mesh_STATUS_OK;
  }
}

void _blocking_op_finished(struct A7105_Mesh* node,
                           A7105_Mesh_Status status,
                           void* context)
{
  //Set the operation_status so the original function can
  //realize we're done and return the value
  node->blocking_operation_status = status;
}


//Special callback for when we're interrupted with a re-join
//mid-operation. This restores the state pre-interrupt and 
//pushes the original packet again with the new node-id
void _rejoin_finished(struct A7105_Mesh* node,
                             A7105_Mesh_Status status,
                             void* context)
{
  //If we get here, we've rejoined as a different node ID.
  //Restore our pre-rejoin state
  node->state = node->pending_state;
  node->operation_callback = node->pending_operation_callback;
  node->target_node_id = node->pending_target_node_id;
  node->target_unique_id = node->pending_target_unique_id;

  //Restore the interrupted packet and set our new node-id
  memcpy(node->packet_cache,node->pending_request_cache,A7105_MESH_PACKET_SIZE);
  byte original_op = node->packet_cache[A7105_MESH_PACKET_TYPE];

  //Push the original packet again
  _A7105_Mesh_Prep_Packet_Header(node, original_op);
  _A7105_Mesh_Send_Request(node);
}
  
A7105_Mesh_Status A7105_Mesh_Join(struct A7105_Mesh* node, 
                                  void (*join_finished_callback)(struct A7105_Mesh*,A7105_Mesh_Status,void*))
{
  return A7105_Mesh_Join(node, 1, join_finished_callback, false);
}
A7105_Mesh_Status A7105_Mesh_Join(struct A7105_Mesh* node, 
                                  byte node_id,
                                  void (*join_finished_callback)(struct A7105_Mesh*,A7105_Mesh_Status,void*))
{
  return A7105_Mesh_Join(node, node_id, join_finished_callback, false);
}
A7105_Mesh_Status A7105_Mesh_Join(struct A7105_Mesh* node, 
                                  byte node_id,
                                  void (*join_finished_callback)(struct A7105_Mesh*,A7105_Mesh_Status,void*),
                                  byte interrupting)
{
  //TODO: Implement mesh-full behavior and make sure the failure
  //carries through for any pending interrupted operations


  //If this is a rejoin, we're interrupting another request already
  //in progress. 
  //cache the current request callback (to restore later)
  //NOTE: The request packet sent is already cached
  //      in node->pending_request_cache so we can re-send
  //      later
  if (interrupting)
  {
    //Save our state for restoring after rejoin
    node->pending_operation = true;
    node->pending_operation_callback = node->operation_callback;
    node->pending_state = node->state;
    node->pending_target_node_id = node->target_node_id;
    node->pending_target_unique_id = node->target_unique_id;
  }


  //Set up our state, requested node ID and track our first tx time
  node->state = A7105_Mesh_JOINING;
  node->node_id = node_id;
  node->join_first_tx_time = millis();
  DebugHeader(node);
  putstring("Attempting to join as ");
  Serial.println(node->node_id);

  //Set up the finishing callback
  if (interrupting)
    _A7105_Mesh_Prep_Finishing_Callback(node,
                                        _rejoin_finished,
                                        NULL);
  else
    _A7105_Mesh_Prep_Finishing_Callback(node,
                                        join_finished_callback,
                                       _blocking_op_finished);

  //Prep and send the first JOIN Packet
  _A7105_Mesh_Prep_Packet_Header(node, A7105_MESH_PKT_JOIN);
  _A7105_Mesh_Send_Request(node);


  //If no callback was specified, block until we get a status value
  if (join_finished_callback == NULL)
  {
    while (node->blocking_operation_status == A7105_Mesh_NO_STATUS)
    {
      A7105_Mesh_Update(node);
    }

    return node->blocking_operation_status;
  }

  //Otherwise just return an OK status
  return A7105_Mesh_STATUS_OK;
}

void _A7105_Mesh_Update_Join(struct A7105_Mesh* node)
{
  //We only do anything here if we're in the middle of joining
  if (node->state != A7105_Mesh_JOINING)
    return;

  //Can we say we're part of the mesh yet (We've waited long enough)
  if (millis() - node->join_first_tx_time > A7105_MESH_JOIN_ACCEPT_DELAY)
  {
    node->state = A7105_Mesh_IDLE;
    node->operation_callback(node, A7105_Mesh_STATUS_OK, node->client_context_obj);
  }

  //Otherwise do we need to send another JOIN packet?
  else if (millis() - node->request_sent_time > (unsigned long)(node->join_retransmit_delay))
  {
    //Prep and send the next JOIN Packet
    //NOTE: request_sent_time is updated in the Prep call below
    _A7105_Mesh_Prep_Packet_Header(node, A7105_MESH_PKT_JOIN);
    _A7105_Mesh_Send_Request(node);
  }
}

void _A7105_Mesh_Handle_Conflict_Name(struct A7105_Mesh* node)
{
  //Is this CONFLICT_NAME packet for us?
  if (node->packet_cache[A7105_MESH_PACKET_TYPE] == A7105_MESH_PKT_CONFLICT_NAME &&
      node->packet_cache[A7105_MESH_PACKET_NODE_ID] == node->node_id &&
      A7105_Util_Get_Pkt_Unique_Id(node->packet_cache) != node->unique_id)
  {

    byte interrupting_existing_operation = false;
    byte need_to_rejoin = false;
    byte rejoin_id = 1;
     
    //If we're joining, try joining with a different ID
    if (node->state == A7105_Mesh_JOINING)
    {
      need_to_rejoin = true;
      rejoin_id = node->node_id + 1;
    }
  
    //Otherwise, if we're on the mesh, use the unique ID's to determine
    //who rejoins
    else if (node->state != A7105_Mesh_NOT_JOINED &&
             node->unique_id < A7105_Util_Get_Pkt_Unique_Id(node->packet_cache))
    {
      need_to_rejoin = true;
    }

    //If we need to re-join, kick it off from here
    if (need_to_rejoin)
    {
      //Mark the CONFLICT_NAME packet as handled since we're taking action
      _A7105_Mesh_Handling_Request(node, node->packet_cache);
     
      //NOTE: If we're in the middle of another operation, push it to the
      //      pending status so we can get back to it once we're done rejoining
      if (node->state != A7105_Mesh_NOT_JOINED &&
          node->state != A7105_Mesh_IDLE && 
          node->state != A7105_Mesh_JOINING)
      {
        interrupting_existing_operation = true;
      }

      //Trigger a rejoin the caches the current operation if needed
      A7105_Mesh_Join(node, rejoin_id, node->operation_callback, interrupting_existing_operation);
    }

  } 
}

void _A7105_Mesh_Handle_Deferred_Joining(struct A7105_Mesh* node)
{

  if (node->state == A7105_Mesh_JOINING &&
      node->packet_cache[A7105_MESH_PACKET_TYPE] == A7105_MESH_PKT_JOIN &&
      node->packet_cache[A7105_MESH_PACKET_NODE_ID] == node->node_id &&
      A7105_Util_Get_Pkt_Unique_Id(node->packet_cache) > node->unique_id)
  {
    //DEBUG
    DebugHeader(node);
    putstring("deferring JOIN to ");
    Serial.print(A7105_Util_Get_Pkt_Unique_Id(node->packet_cache),HEX);
    putstring(" since they have a higher unique, rejoining...\r\n");
    
  
    //If we see a conflict and we're the lower Unique ID, rejoin with a +1 node id
    byte rejoin_id = node->node_id+1;
    A7105_Mesh_Join(node,rejoin_id,node->operation_callback);
  }
}

byte _A7105_Mesh_Check_For_Node_ID_Conflicts(struct A7105_Mesh* node)
{
  //If we're on the mesh and we find a packet
  //whose sender node-ID is ours, but whose unique is lower
  if (node->state != A7105_Mesh_NOT_JOINED &&
      node->state != A7105_Mesh_JOINING &&
      node->packet_cache[A7105_MESH_PACKET_NODE_ID] == node->node_id &&
      A7105_Util_Get_Pkt_Unique_Id(node->packet_cache) != node->unique_id)
  {
   //DEBUG
    DebugHeader(node);
    putstring("Found different node pushing packets as us ->");
    Serial.print(A7105_Util_Get_Pkt_Unique_Id(node->packet_cache),HEX);
    putstring(" sending name conflict\r\n");
  
    _A7105_Mesh_Prep_Packet_Header(node, A7105_MESH_PKT_CONFLICT_NAME);
    _A7105_Mesh_Send_Request(node);
    return true;
  }
  return false;
    
}

A7105_Mesh_Status A7105_Mesh_Update(struct A7105_Mesh* node)
{
  //Process any incoming packets
  _A7105_Mesh_Handle_RX(node);

  //Update JOIN activity 
  _A7105_Mesh_Update_Join(node);

  //Update PING activity 
  _A7105_Mesh_Update_Ping(node);

  //Check for timeout with GET_NUM_REGISTERS
  _A7105_Mesh_Check_For_Timeout(node,
                                A7105_Mesh_GET_NUM_REGISTERS,
                                A7105_MESH_REQUEST_TIMEOUT);

  //Check for timeout with GET_REGISTER_NAME 
  _A7105_Mesh_Check_For_Timeout(node,
                                A7105_Mesh_GET_REGISTER_NAME,
                                A7105_MESH_REQUEST_TIMEOUT);

  //Check for timeout with GET_REGISTER 
  _A7105_Mesh_Check_For_Timeout(node,
                                A7105_Mesh_GET_REGISTER,
                                A7105_MESH_REQUEST_TIMEOUT);

  //Check for timeout with SET_REGISTER 
  _A7105_Mesh_Check_For_Timeout(node,
                                A7105_Mesh_SET_REGISTER,
                                A7105_MESH_REQUEST_TIMEOUT);


  //Update packet repeat activity 
  _A7105_Mesh_Update_Repeats(node);
  

  //HACK: NUKEME
  return A7105_Mesh_STATUS_OK;
}



A7105_Mesh_Status A7105_Mesh_Ping(struct A7105_Mesh* node, 
                                  void (*ping_finished_callback)(struct A7105_Mesh*,A7105_Mesh_Status,void*))
{
  //Check our status (make sure we are on a mesh and not busy)
  A7105_Mesh_Status ret;
  if ((ret = _A7105_Mesh_Is_Node_Idle(node)) != A7105_Mesh_STATUS_OK)
    return ret;
  
  //Update our state
  node->state = A7105_Mesh_PING;
  node->target_node_id = 0;
  node->target_unique_id = 0;
 
  //Set our completed callback
  _A7105_Mesh_Prep_Finishing_Callback(node,
                                      ping_finished_callback,
                                      _blocking_op_finished);
 
  //Clear (zero) the presence table
  memset(node->presence_table,0,32);
  
  //Prep and send the first PING Packet
  _A7105_Mesh_Prep_Packet_Header(node, A7105_MESH_PKT_PING);
  _A7105_Mesh_Send_Request(node);
    
  //If no callback was specified, block until we get a status value
  if (ping_finished_callback == NULL)
  {
    while (node->blocking_operation_status == A7105_Mesh_NO_STATUS)
    {
      A7105_Mesh_Update(node);
    }

    return node->blocking_operation_status;
  }

  //Otherwise just return an OK status
  return A7105_Mesh_STATUS_OK;
}

void _A7105_Mesh_Handle_Ping(struct A7105_Mesh* node)
{
  //If we're on a mesh and we see a PING packet, respond to it
  //with a PONG
  if (node->state != A7105_Mesh_NOT_JOINED &&
      node->state != A7105_Mesh_JOINING &&
      _A7105_Mesh_Filter_Packet(node,A7105_MESH_PKT_PING,true))
  {
    //Mark the request as handled and push back a PONG
    _A7105_Mesh_Handling_Request(node, node->packet_cache);
    _A7105_Mesh_Prep_Packet_Header(node, A7105_MESH_PKT_PONG);
    _A7105_Mesh_Send_Response(node);
  }
}

void _A7105_Mesh_Handle_Pong(struct A7105_Mesh* node)
{
  //If we sent a PING, and see a PONG come back
  if (node->state == A7105_Mesh_PING &&
      _A7105_Mesh_Filter_Packet(node,A7105_MESH_PKT_PONG,false))
  {

    //Update the presence table with the bit for the responding
    //node-id
    byte node_id = node->packet_cache[A7105_MESH_PACKET_NODE_ID];
    byte byte_offset = node_id/8;
    byte bit_mask = 1<<(node_id%8);
    node->presence_table[byte_offset] |= bit_mask;
  }
}

void _A7105_Mesh_Update_Ping(struct A7105_Mesh* node)
{
  if (node->state == A7105_Mesh_PING &&
      millis() - node->request_sent_time > A7105_MESH_PING_TIMEOUT)
  {
    //NUKEME
    int num_nodes = 0;
    for (int x = 1;x<256;x++)
    {
      if (node->presence_table[x/8] & (byte)(1<<(x%8)))
        num_nodes++;
    }

    DebugHeader(node);
    putstring("Finished ping, found ");
    Serial.print(num_nodes);
    putstring(" other nodes\r\n");

    node->state = A7105_Mesh_IDLE;
    node->operation_callback(node, A7105_Mesh_STATUS_OK,node->client_context_obj);
  }
}

A7105_Mesh_Status A7105_Mesh_GetNumRegisters(struct A7105_Mesh* node,
                                             byte node_id,
                                             void (*get_num_registers_finished_callback)(struct A7105_Mesh*,A7105_Mesh_Status,void*))
{
  return A7105_Mesh_GetNumRegisters(node, node_id, 0,get_num_registers_finished_callback);
}
A7105_Mesh_Status A7105_Mesh_GetNumRegisters(struct A7105_Mesh* node,
                                             byte node_id,
                                             uint16_t target_unique_id,
                                             void (*get_num_registers_finished_callback)(struct A7105_Mesh*,A7105_Mesh_Status,void*))
{
  //Make sure we're idle on a mesh
  A7105_Mesh_Status ret;
  if ((ret = _A7105_Mesh_Is_Node_Idle(node)) != A7105_Mesh_STATUS_OK)
    return ret;

  //Update our state and target filters
  node->state = A7105_Mesh_GET_NUM_REGISTERS;
  node->target_node_id = node_id; //only read responses from node_id
  node->target_unique_id = target_unique_id; //filter (optional) unique ID 

  //Set our completed callback
  _A7105_Mesh_Prep_Finishing_Callback(node,
                                      get_num_registers_finished_callback,
                                      _blocking_op_finished);

  //Prep and send the first GET_NUM_REGISTERS Packet
  _A7105_Mesh_Prep_Packet_Header(node, A7105_MESH_PKT_GET_NUM_REGISTERS);
  node->packet_cache[A7105_MESH_PACKET_TARGET_ID] = node_id;
  _A7105_Mesh_Send_Request(node);
    
  //If no callback was specified, block until we get a status value
  if (get_num_registers_finished_callback == NULL)
  {
    while (node->blocking_operation_status == A7105_Mesh_NO_STATUS)
      A7105_Mesh_Update(node);

    return node->blocking_operation_status;
  }

  //Otherwise just return an OK status
  return A7105_Mesh_STATUS_OK;
}

void _A7105_Mesh_Handle_GetNumRegisters(struct A7105_Mesh* node)
{
  //If we're on a mesh and we see a GET_NUM_REGISTERS request
  //and it's addressed to us
  if (node->state != A7105_Mesh_NOT_JOINED &&
      node->state != A7105_Mesh_JOINING &&
      _A7105_Mesh_Filter_Packet(node,A7105_MESH_PKT_GET_NUM_REGISTERS,true) &&
      node->packet_cache[A7105_MESH_PACKET_TARGET_ID] == node->node_id)
  {
    //Mark the request as handled and push back a NUM_REGISTERS packet
    _A7105_Mesh_Handling_Request(node, node->packet_cache);
    _A7105_Mesh_Prep_Packet_Header(node, A7105_MESH_PKT_NUM_REGISTERS);
    node->packet_cache[A7105_MESH_PACKET_DATA_START] = node->num_registers;
    _A7105_Mesh_Send_Response(node);
  }
}

void _A7105_Mesh_Handle_NumRegisters(struct A7105_Mesh* node)
{
  //If we sent a GET_NUM_REGISTERS, and see a NUM_REGISTERS come back
  //from our target (Filter_Packet handles that part)
  if (node->state == A7105_Mesh_GET_NUM_REGISTERS &&
      _A7105_Mesh_Filter_Packet(node,A7105_MESH_PKT_NUM_REGISTERS,false))
  {


    //Update the num registers cache
    node->num_registers_cache = node->packet_cache[A7105_MESH_PACKET_DATA_START];

    //Record the responder info
    node->responder_node_id = node->packet_cache[A7105_MESH_PACKET_NODE_ID]; 
    node->responder_unique_id = A7105_Util_Get_Pkt_Unique_Id(node->packet_cache);

    //Update our status back to IDLE
    node->state = A7105_Mesh_IDLE;
    node->operation_callback(node, A7105_Mesh_STATUS_OK,node->client_context_obj);
  }

}

A7105_Mesh_Status A7105_Mesh_GetRegisterName(struct A7105_Mesh* node,
                                             byte node_id,
                                             byte reg_index,
                                             void (*get_register_name_finished_callback)(struct A7105_Mesh*,A7105_Mesh_Status,void*))
{
  return A7105_Mesh_GetRegisterName(node,node_id,reg_index,0,get_register_name_finished_callback);
}

A7105_Mesh_Status A7105_Mesh_GetRegisterName(struct A7105_Mesh* node,
                                             byte node_id,
                                             byte reg_index,
                                             uint16_t filter_unique_id,
                                             void (*get_register_name_finished_callback)(struct A7105_Mesh*,A7105_Mesh_Status,void*))
{
  //Make sure we're idle on a mesh
  A7105_Mesh_Status ret;
  if ((ret = _A7105_Mesh_Is_Node_Idle(node)) != A7105_Mesh_STATUS_OK)
    return ret;

  //Update our state and target filters
  node->state = A7105_Mesh_GET_REGISTER_NAME;
  node->target_node_id = node_id; //only read responses from node_id
  node->target_unique_id = filter_unique_id; //filter (optional) unique ID 

  //Set our completed callback
  _A7105_Mesh_Prep_Finishing_Callback(node,
                                      get_register_name_finished_callback,
                                      _blocking_op_finished);

  //Prep and send the first GET_REGISTER_NAME Packet
  _A7105_Mesh_Prep_Packet_Header(node, A7105_MESH_PKT_GET_REGISTER_NAME);
  node->packet_cache[A7105_MESH_PACKET_TARGET_ID] = node_id;
  node->packet_cache[A7105_MESH_PACKET_REG_INDEX] = reg_index;
  _A7105_Mesh_Send_Request(node);
    
  //If no callback was specified, block until we get a status value
  if (get_register_name_finished_callback == NULL)
  {
    while (node->blocking_operation_status == A7105_Mesh_NO_STATUS)
      A7105_Mesh_Update(node);

    return node->blocking_operation_status;
  }

  //Otherwise just return an OK status
  return A7105_Mesh_STATUS_OK;
}

void _A7105_Mesh_Handle_GetRegisterName(struct A7105_Mesh* node)
{
  //If we're on a mesh and we see a GET_REGISTER_NAME request
  //and it's addressed to us
  if (node->state != A7105_Mesh_NOT_JOINED &&
      node->state != A7105_Mesh_JOINING &&
      _A7105_Mesh_Filter_Packet(node,A7105_MESH_PKT_GET_REGISTER_NAME,true) &&
      node->packet_cache[A7105_MESH_PACKET_TARGET_ID] == node->node_id)
  {
    //Grab the reg index before we overwrite the packet (prep header below)
    byte reg_index = node->packet_cache[A7105_MESH_PACKET_REG_INDEX];

    //Mark the request as handled and push back a REGISTER_NAME packet
    _A7105_Mesh_Handling_Request(node, node->packet_cache);
    _A7105_Mesh_Prep_Packet_Header(node, A7105_MESH_PKT_REGISTER_NAME);

    //Set the defaul "No register found" state on the packet
    node->packet_cache[A7105_MESH_PACKET_DATA_START] = 0;
    
    //if the register index exists, populate the packet with the name
    if (node->num_registers > reg_index)
    {
      _A7105_Mesh_Util_Register_To_Packet(node->packet_cache,
                                          &(node->registers[reg_index]),
                                          false); //include value = false
    }
    _A7105_Mesh_Send_Response(node);
  }
}

void _A7105_Mesh_Handle_RegisterName(struct A7105_Mesh* node)
{
  //If we sent a GET_REGISTER_NAME, and see a REGISTER_NAME come back
  //from our target (Filter_Packet handles that part)
  if (node->state == A7105_Mesh_GET_REGISTER_NAME &&
      _A7105_Mesh_Filter_Packet(node,A7105_MESH_PKT_REGISTER_NAME,false))
  {
    A7105_Mesh_Status ret = A7105_Mesh_STATUS_OK;

    //Update the register cache with the returned data
    byte converted = _A7105_Mesh_Util_Packet_To_Register(node->packet_cache,
                                                         &(node->register_cache),
                                                         false); //include value = false

    //Check if we got a valid value and update return status
    if (!converted)
      ret = A7105_Mesh_INVALID_REGISTER_INDEX;

    //Record the responder info
    node->responder_node_id = node->packet_cache[A7105_MESH_PACKET_NODE_ID]; 
    node->responder_unique_id = A7105_Util_Get_Pkt_Unique_Id(node->packet_cache);

    //Update our status back to IDLE
    node->state = A7105_Mesh_IDLE;
    node->operation_callback(node, ret, node->client_context_obj);
  }
}

A7105_Mesh_Status A7105_Mesh_GetRegister(struct A7105_Mesh* node,
                                         struct A7105_Mesh_Register* reg,
                                         void (*get_register_finished_callback)(struct A7105_Mesh*,A7105_Mesh_Status,void*))
{
  //Make sure we're idle on a mesh
  A7105_Mesh_Status ret;
  if ((ret = _A7105_Mesh_Is_Node_Idle(node)) != A7105_Mesh_STATUS_OK)
    return ret;

  //Update our state and target filters
  node->state = A7105_Mesh_GET_REGISTER;
  //reset target filters, GET_REGISTER is a global operation
  node->target_node_id = 0; 
  node->target_unique_id = 0; 

  //Set our completed callback
  _A7105_Mesh_Prep_Finishing_Callback(node,
                                      get_register_finished_callback,
                                      _blocking_op_finished);

  //Prep the GET_REGISTER packet
  _A7105_Mesh_Prep_Packet_Header(node, A7105_MESH_PKT_GET_REGISTER);

  //Bail if the Register structure had invalid lengths/data
  if (!_A7105_Mesh_Util_Register_To_Packet(node->packet_cache,
                                                 reg,
                                                 false))
  {
    return A7105_Mesh_INVALID_REGISTER_LENGTH;
  }

  //Push the request to the radio
  _A7105_Mesh_Send_Request(node);
    
  //If no callback was specified, block until we get a status value
  if (get_register_finished_callback == NULL)
  {
    while (node->blocking_operation_status == A7105_Mesh_NO_STATUS)
      A7105_Mesh_Update(node);

    return node->blocking_operation_status;
  }

  //Otherwise just return an OK status
  return A7105_Mesh_STATUS_OK;
}

void _A7105_Mesh_Handle_GetRegister(struct A7105_Mesh* node)
{
  //If we're on a mesh and we see a GET_REGISTER request
  if (node->state != A7105_Mesh_NOT_JOINED &&
      node->state != A7105_Mesh_JOINING &&
      _A7105_Mesh_Filter_Packet(node,A7105_MESH_PKT_GET_REGISTER,true))
  {

    //bail if this isn't a register we service (otherwise, we'll have the index)
    int register_index = -1;
    if ((register_index = _A7105_Mesh_Filter_RegisterName(node)) == -1)
      return; 

    //Mark the request as handled and push back a REGISTER_VALUE packet
    _A7105_Mesh_Handling_Request(node, node->packet_cache);
    _A7105_Mesh_Prep_Packet_Header(node, A7105_MESH_PKT_REGISTER_VALUE);

    //If the client has specified a GET_REGISTER callback for this Register, call it
    //now so they can update/populate the Register data
    if (node->registers[register_index].get_callback != NULL)
      node->registers[register_index].get_callback(&(node->registers[register_index]),
                                                   node->client_context_obj);

    //Put the current register name/value in the response packet   
    _A7105_Mesh_Util_Register_To_Packet(node->packet_cache,
                                        &(node->registers[register_index]),
                                        true);
 
    _A7105_Mesh_Send_Response(node);
  }
}

void _A7105_Mesh_Handle_RegisterValue(struct A7105_Mesh* node)
{
  //If we sent a GET_REGISTER, and see a REGISTER_VALUE come back
  //for the register we queried
  if (node->state == A7105_Mesh_GET_REGISTER &&
      _A7105_Mesh_Filter_Packet(node,A7105_MESH_PKT_REGISTER_VALUE,false))
  {
    A7105_Mesh_Status ret = A7105_Mesh_STATUS_OK;

    //bail if this wasn't the register we requested
    if (!_A7105_Mesh_Cmp_Packet_Register_Name(node->packet_cache,
                                              &(node->register_cache))) 
      return;

    //Otherwise, update the register cache with the returned data
    byte converted = _A7105_Mesh_Util_Packet_To_Register(node->packet_cache,
                                                         &(node->register_cache),
                                                         true); //include value

    //Check if we got a valid value and update return status
    if (!converted)
      ret = A7105_Mesh_INVALID_REGISTER_RETURNED;

    //Record the responder info
    node->responder_node_id = node->packet_cache[A7105_MESH_PACKET_NODE_ID]; 
    node->responder_unique_id = A7105_Util_Get_Pkt_Unique_Id(node->packet_cache);

    //Update our status back to IDLE
    node->state = A7105_Mesh_IDLE;
    node->operation_callback(node, ret, node->client_context_obj);
  }
}

void _A7105_Mesh_Check_For_Timeout(struct A7105_Mesh* node,
                                   A7105_Mesh_State operation,
                                   int timeout)
{
  if (node->state == operation &&
      millis() - node->request_sent_time > (unsigned long)timeout)
  {
    //Update our status back to IDLE and mark the request as a timeout
    node->state = A7105_Mesh_IDLE;
    node->operation_callback(node, A7105_Mesh_TIMEOUT, node->client_context_obj);
  }
}

A7105_Mesh_Status A7105_Mesh_SetRegister(struct A7105_Mesh* node,
                                         struct A7105_Mesh_Register* reg,
                                         void (*set_register_finished_callback)(struct A7105_Mesh*,A7105_Mesh_Status,void*))
{
  //Make sure we're idle on a mesh
  A7105_Mesh_Status ret;
  if ((ret = _A7105_Mesh_Is_Node_Idle(node)) != A7105_Mesh_STATUS_OK)
    return ret;

  //Update our state and target filters
  node->state = A7105_Mesh_SET_REGISTER;
  //reset target filters, SET_REGISTER is a global operation
  node->target_node_id = 0; 
  node->target_unique_id = 0; 

  //Set our completed callback
  _A7105_Mesh_Prep_Finishing_Callback(node,
                                      set_register_finished_callback,
                                      _blocking_op_finished);

  //Prep the SET_REGISTER packet
  _A7105_Mesh_Prep_Packet_Header(node, A7105_MESH_PKT_SET_REGISTER);

  //Bail if the Register structure had invalid lengths/data
  if (!_A7105_Mesh_Util_Register_To_Packet(node->packet_cache,
                                                 reg,
                                                 true))
  {
    return A7105_Mesh_INVALID_REGISTER_LENGTH;
  }

  //Push the request to the radio
  _A7105_Mesh_Send_Request(node);
    
  //If no callback was specified, block until we get a status value
  if (set_register_finished_callback == NULL)
  {
    while (node->blocking_operation_status == A7105_Mesh_NO_STATUS)
      A7105_Mesh_Update(node);

    return node->blocking_operation_status;
  }

  //Otherwise just return an OK status
  return A7105_Mesh_STATUS_OK;

}

void _A7105_Mesh_Handle_SetRegister(struct A7105_Mesh* node)
{
  //If we're on a mesh and we see a SET_REGISTER request
  if (node->state != A7105_Mesh_NOT_JOINED &&
      node->state != A7105_Mesh_JOINING &&
      _A7105_Mesh_Filter_Packet(node,A7105_MESH_PKT_SET_REGISTER,true))
  {

    //bail if this isn't a register we service (otherwise, we'll have the index)
    int register_index = -1;
    if ((register_index = _A7105_Mesh_Filter_RegisterName(node)) == -1)
      return; 

    //If the client has specified a SET_REGISTER callback for this Register, call it
    //now so they can update/populate the Register data
    byte error_set = false;
    byte auto_set = false;
    if (node->registers[register_index].set_callback != NULL)
    {
      byte ret = node->registers[register_index].set_callback(&(node->registers[register_index]),
                                                              node,
                                                   node->client_context_obj);
      
      //If the callback says auto-set, just copy the value from the 
      //packet into the register
      if (ret == A7105_Mesh_AUTO_SET_REGISTER)
      {
        auto_set = true;
      }
      else if (ret == A7105_Mesh_INVALID_REGISTER_VALUE)
      {
        error_set = true;

        //Put a generic error in if none was specified
        if (node->register_cache._error_set == 0)
        {
          strncpy_P((char*)node->register_cache._data, (char*)pgm_read_word(&A7105_ERROR_STRINGS[SET_REG_CB_NO_ERROR_SET]),A7105_MESH_MAX_REGISTER_PART_SIZE - 1);
        }
      }

      //If the callback return something other than "OK"
      //Let's mark it as an error for the caller so they aren't 
      //left wondering
      else if (ret != A7105_Mesh_STATUS_OK)
      {
        strncpy_P((char*)node->register_cache._data, (char*)pgm_read_word(&A7105_ERROR_STRINGS[SET_REG_CB_BOGUS_RETURN]),A7105_MESH_MAX_REGISTER_PART_SIZE - 1);
      }
    }

    //Handle no callback or autoset behavior
    if (node->registers[register_index].set_callback == NULL ||
        auto_set)
    {
      if (!_A7105_Mesh_Util_Packet_To_Register(node->packet_cache,
                                          &(node->registers[register_index]),
                                          true))
      {
        strncpy_P((char*)node->register_cache._data, (char*)pgm_read_word(&A7105_ERROR_STRINGS[SET_REG_INVALID_VALUE_SIZE]),A7105_MESH_MAX_REGISTER_PART_SIZE - 1);
        error_set = true;
      }
    }

    byte return_target = node->packet_cache[A7105_MESH_PACKET_NODE_ID];
    //Mark the request as handled and push back a SET_REGISTER packet
    _A7105_Mesh_Handling_Request(node, node->packet_cache);
    _A7105_Mesh_Prep_Packet_Header(node, A7105_MESH_PKT_SET_REGISTER_ACK);
    node->packet_cache[A7105_MESH_PACKET_TARGET_ID] = return_target;

    //Copy the error from the register to the returning packet if it was set
    if (error_set)
    {
      strncpy((char*)&(node->packet_cache[A7105_MESH_PACKET_ERR_MSG_START]),
              (char*)node->register_cache._data,
              A7105_MESH_MAX_REGISTER_PART_SIZE - 1);

       //Re-Init the register cache also (since we potentially used it to store an error message)
       A7105_Mesh_Register_Initialize(&(node->register_cache),NULL,NULL);
    }

    _A7105_Mesh_Send_Response(node);

   //Zero out the register error for next time
   _A7105_Mesh_Register_Clear_Error(&(node->registers[register_index]));
  }
}

void _A7105_Mesh_Handle_SetRegisterAck(struct A7105_Mesh* node)
{
  //If we sent a SET_REGISTER, and see a SET_REGISTER_VALUE_ACK come back addressed to us.
  if (node->state == A7105_Mesh_SET_REGISTER &&
      _A7105_Mesh_Filter_Packet(node,A7105_MESH_PKT_SET_REGISTER_ACK,false) &&
      node->packet_cache[A7105_MESH_PACKET_TARGET_ID] == node->node_id)
  {
    A7105_Mesh_Status ret = A7105_Mesh_STATUS_OK;

    //Check if there is an error message set and, if so, set the 
    //error message for the register cache so the client code can
    //"see and smells things" 
    if (node->packet_cache[A7105_MESH_PACKET_ERR_MSG_START] != 0)
    {
      //Set the return value
      ret = A7105_Mesh_INVALID_REGISTER_VALUE;
      //Set the error flag in the register_cache
      node->register_cache._error_set = true;
      //copy the error string from the packet
      strncpy((char*)node->register_cache._data,
              (char*)&(node->packet_cache[A7105_MESH_PACKET_ERR_MSG_START]),
              A7105_MESH_MAX_REGISTER_PART_SIZE - 1);

    }

    //Record the responder info
    node->responder_node_id = node->packet_cache[A7105_MESH_PACKET_NODE_ID]; 
    node->responder_unique_id = A7105_Util_Get_Pkt_Unique_Id(node->packet_cache);

    //Update our status back to IDLE
    node->state = A7105_Mesh_IDLE;
    node->operation_callback(node, ret, node->client_context_obj);
  }

}


//////////////////////// Utility Functions ///////////////////////
      
byte A7105_Get_Next_Present_Node(struct A7105_Mesh* node, byte start)
{
  if (start == 0xFF)
    return 0;

  for (byte x = start+1;x<0xFF;x++)
    if (node->presence_table[x/8] & (byte)(1<<(x%8)))
    {
      return x;
    }

  return 0;
}

A7105_Mesh_Status _A7105_Mesh_Is_Node_Idle(struct A7105_Mesh* node)
{
 if (node->state == A7105_Mesh_JOINING ||
     node->state == A7105_Mesh_NOT_JOINED)
    return A7105_Mesh_NOT_ON_MESH;


  //Ensure we're in a good state to handle this
  if (node->state != A7105_Mesh_IDLE)
    return A7105_Mesh_BUSY; 

  return A7105_Mesh_STATUS_OK;
} 

//Push a packet from the packet_cache to the end of the repeat
//ring buffer
void _A7105_Mesh_Append_Repeat(struct A7105_Mesh* node)
{
  if (node->repeat_cache_size == A7105_MESH_MAX_REPEAT_CACHE_SIZE)
    return;

  memcpy(node->repeat_cache[node->repeat_cache_end],node->packet_cache,A7105_MESH_PACKET_SIZE);
  node->repeat_cache_end += 1;
  node->repeat_cache_end %= A7105_MESH_MAX_REPEAT_CACHE_SIZE;
  node->repeat_cache_size++;
}

//pop packet from the front of the ring buffer
byte* _A7105_Mesh_Pop_Repeat(struct A7105_Mesh* node)
{
  if (node->repeat_cache_size == 0)
    return NULL;
  byte* ret = node->repeat_cache[node->repeat_cache_start]; 
  node->repeat_cache_start += 1;
  node->repeat_cache_start %= A7105_MESH_MAX_REPEAT_CACHE_SIZE;
  node->repeat_cache_size--;
  return ret;
}


void _A7105_Mesh_Update_Repeats(struct A7105_Mesh* node)
{
  //If we're on the mesh, we have packets to repeat,
  //and it's been long enough since our last repeat (device id delay)
  if (node->state != A7105_Mesh_NOT_JOINED &&
      node->state != A7105_Mesh_JOINING &&
      node->repeat_cache_size > 0 &&
      millis() - node->last_repeat_sent_time > (unsigned long)node->random_delay)
  {
    //Pop the packet
    byte* packet = _A7105_Mesh_Pop_Repeat(node);
    //Increase the hop count
    packet[A7105_MESH_PACKET_HOP]++; 
    //Push the packet to the network (don't use the SendRequest sinc
    //we don't want to cache this anywhere else)
    A7105_WriteData(&(node->radio), packet, A7105_MESH_PACKET_SIZE);

    //DEBUG (print packet to serial)
    DebugHeader(node);
    putstring("REPEAT:[");
    for (int x = 0; x<6;x++)
    {
      Serial.print(packet[x],HEX);
      putstring("] [");
    }
    putstring("...\r\n");

    //Wait for the transmission to finish,then start the radio
    //listening again
    while (A7105_CheckTXFinished(&(node->radio)) != A7105_STATUS_OK)
    {
      //Serial.println(digitalRead((node->radio)._INTERRUPT_PIN));
      //HACK: Should we just busy wait or keep these delays()...
      delay(10);
    }

    //Tell the radio to go back to listening
    A7105_Easy_Listen_For_Packets(&(node->radio), A7105_MESH_PACKET_SIZE);



  }

}

void _A7105_Mesh_Cache_Packet_For_Repeat(struct A7105_Mesh* node)
{
  //Don't repeat if we're not on a mesh
  if (node->state == A7105_Mesh_NOT_JOINED ||
      node->state == A7105_Mesh_JOINING)
    return;

  //Don't repeat if we originally sent the packet
  if (node->packet_cache[A7105_MESH_PACKET_NODE_ID] == node->node_id &&
      A7105_Util_Get_Pkt_Unique_Id(node->packet_cache) == node->unique_id)
    return;

  //Don't repeat if this was a request addressed to us
  if ((node->packet_cache[A7105_MESH_PACKET_TYPE] == A7105_MESH_PKT_GET_NUM_REGISTERS ||
      node->packet_cache[A7105_MESH_PACKET_TYPE] == A7105_MESH_PKT_SET_REGISTER_ACK ||
      node->packet_cache[A7105_MESH_PACKET_TYPE] == A7105_MESH_PKT_GET_REGISTER_NAME) &&
      node->packet_cache[A7105_MESH_PACKET_TARGET_ID] == node->node_id)
    return;

  //Don't repeat if this packet is a response for our latest *directed* request
  //(GET_NUM_REGISTERS is a directed query)
  if (node->state == A7105_Mesh_GET_NUM_REGISTERS &&
      node->packet_cache[A7105_MESH_PACKET_TYPE] == A7105_MESH_PKT_NUM_REGISTERS &&
      node->packet_cache[A7105_MESH_PACKET_NODE_ID] == node->target_node_id &&
      (node->target_unique_id == 0 || A7105_Util_Get_Pkt_Unique_Id(node->packet_cache) == node->target_unique_id))
    return;
  //(ditto for GET_REGISTER_NAME)       
  if (node->state == A7105_Mesh_GET_REGISTER_NAME &&
      node->packet_cache[A7105_MESH_PACKET_TYPE] == A7105_MESH_PKT_REGISTER_NAME &&
      node->packet_cache[A7105_MESH_PACKET_NODE_ID] == node->target_node_id &&
      (node->target_unique_id == 0 || A7105_Util_Get_Pkt_Unique_Id(node->packet_cache) == node->target_unique_id))
    return;

  //Don't repeat if this was a request for one of our registers
  if (node->packet_cache[A7105_MESH_PACKET_TYPE] == A7105_MESH_PKT_GET_REGISTER ||
      node->packet_cache[A7105_MESH_PACKET_TYPE] == A7105_MESH_PKT_SET_REGISTER)
  {
      //check the register names we service vs the packet
      for (int x = 0;x<node->num_registers;x++)
        if (_A7105_Mesh_Util_Does_Packet_Have_Register(node->packet_cache,
                                                  &(node->registers[x])))
          return;
  }   

  //Don't repeat if this packet has already been repeated
  //(check the repeat cache and compare it sans hop-count)
  //EXCEPTION: Allow JOIN packets to be repeated again and again
  //           since every node needs a good chance to see JOIN
  //           packets
  for (int x = 0;x<A7105_MESH_MAX_REPEAT_CACHE_SIZE;x++)
    if (node->packet_cache[A7105_MESH_PACKET_TYPE] != A7105_MESH_PKT_JOIN &&
        _A7105_Mesh_Util_Is_Same_Packet_Sans_Hop(node->packet_cache,
                                                 node->repeat_cache[x]))
      return;

  //Don't repeat of the packet has been repeated too many times
  if (node->packet_cache[A7105_MESH_PACKET_HOP] >= A7105_MESH_MAX_HOP_COUNT)
    return;

  //If we make it here, the packet is suitable to be repeated
  _A7105_Mesh_Append_Repeat(node); 
}

byte _A7105_Mesh_Util_Is_Same_Packet_Sans_Hop(byte* a, byte* b)
{
  byte ret = true;
  for (int x = 0;x<A7105_MESH_PACKET_SIZE;x++)
    if (a[x] != b[x] && x != A7105_MESH_PACKET_HOP)
      ret = false;
  return ret;
}

void _A7105_Mesh_Prep_Packet_Header(struct A7105_Mesh* node,
                                    byte packet_type)
{
  //HACK: zero out the packet so our packet comparison function is easy to write and any strings written as payload get automatically zero delimited
  memset(node->packet_cache,0,A7105_MESH_PACKET_SIZE);


  node->packet_cache[A7105_MESH_PACKET_TYPE] = packet_type;
  node->packet_cache[A7105_MESH_PACKET_HOP] = 0;
  node->packet_cache[A7105_MESH_PACKET_NODE_ID] = node->node_id;
  node->packet_cache[A7105_MESH_PACKET_UNIQUE_ID] = (node->unique_id)>>8;
  node->packet_cache[A7105_MESH_PACKET_UNIQUE_ID+1] = (node->unique_id) & 0xFF;

}

void _A7105_Mesh_Send_Response(struct A7105_Mesh* node)
{
  //If the packet isn't a join, save a copy in case we're interrupted
  if (node->packet_cache[A7105_MESH_PACKET_TYPE] != A7105_MESH_PKT_JOIN)
  {
    //NOTE: the rest of the pending_* members are set in the Join
    //      function at the time of interrupt.
    memcpy(node->pending_request_cache,node->packet_cache,A7105_MESH_PACKET_SIZE);
  } 

  //Push the packet to the radio
  A7105_WriteData(&(node->radio), node->packet_cache, A7105_MESH_PACKET_SIZE);

  //DEBUG (print packet to serial)
  DebugHeader(node);
  putstring("TX:[");
  for (int x = 0; x<6;x++)
  {
    Serial.print(node->packet_cache[x],HEX);
    putstring("] [");
  }
  putstring("...\r\n");

  //Wait for the transmission to finish,then start the radio
  //listening again
  while (A7105_CheckTXFinished(&(node->radio)) != A7105_STATUS_OK)
  {
    //HACK: Should we just busy wait or keep these delays()...
    delay(10);
  }

  //Tell the radio to go back to listening
  A7105_Easy_Listen_For_Packets(&(node->radio), A7105_MESH_PACKET_SIZE);

}

void _A7105_Mesh_Send_Request(struct A7105_Mesh* node)
{
  //Update the request sending time
  node->request_sent_time = millis();
  _A7105_Mesh_Send_Response(node);

}
                            

byte _A7105_Mesh_Util_Does_Packet_Have_Register(byte* packet,
                                                struct A7105_Mesh_Register* reg)
{
  //Ignore non GET/SET_REGISTER packets
  if (packet[A7105_MESH_PACKET_TYPE] != A7105_MESH_PKT_GET_REGISTER &&
      packet[A7105_MESH_PACKET_TYPE] != A7105_MESH_PKT_SET_REGISTER)
    return false;

  //check the name length vs packet
  int name_offset = A7105_MESH_PACKET_DATA_START;
  if (reg->_name_len != packet[name_offset])
    return false;

  //compare name contents vs packet
  for (int x = 0; x < reg->_name_len; x++)
    if (packet[name_offset + 1 + x] != reg->_data[x])
      return false;

  return true;

}

byte _A7105_Mesh_Util_Register_To_Packet(byte* packet,                  
                                         struct A7105_Mesh_Register* reg,
                                         byte include_value)
{
  //Sanity check the lengths
  int total_size = (int)reg->_name_len + (int)reg->_data_len;
  if (total_size < 2 || total_size > A7105_MESH_MAX_REGISTER_ARRAY_SIZE)
    return false;
  
  //Copy the name
  int offset = A7105_MESH_PACKET_DATA_START; 
  packet[offset] = reg->_name_len;
  offset++;
  memcpy(&(packet[offset]),reg->_data,reg->_name_len);
  offset += reg->_name_len;

  //Optionally copy the value
  if (include_value)
  {
    packet[offset] = reg->_data_len;
    offset++;
    memcpy(&(packet[offset]),&(reg->_data[reg->_name_len]),reg->_data_len);
  }
  return true;
}
                                                                        
byte _A7105_Mesh_Util_Packet_To_Register(byte* packet,                  
                                         struct A7105_Mesh_Register* reg,
                                         byte include_value)           
{
  int offset = A7105_MESH_PACKET_DATA_START; 

  //Do a sanity check before blowing away 'reg'
  //HACK: we do this so if we get a bogus response
  //      from another node, we don't lose the value in register_cache
  //      so the client code can see what failed.
  int total_size = (int)(packet[offset]);
  if (total_size == 0 || total_size > A7105_MESH_MAX_REGISTER_ARRAY_SIZE)
    return false;
  if (include_value)
  {
    offset += packet[offset] + 1;
    total_size += (int)(packet[offset]);
    if (total_size < 2 || total_size > A7105_MESH_MAX_REGISTER_ARRAY_SIZE)
      return false;
  }

  //Copy the name
  offset = A7105_MESH_PACKET_DATA_START; 
  reg->_name_len = packet[offset]; 
  offset++;
  memcpy(reg->_data,&(packet[offset]),reg->_name_len);
  offset += reg->_name_len;

  //Optionally copy the value
  if (include_value)
  {
    reg->_data_len = packet[offset];
    offset++;
    memcpy(&(reg->_data[reg->_name_len]),&(packet[offset]),reg->_data_len);
  }
  return true;
}

int _A7105_Mesh_Filter_RegisterName(struct A7105_Mesh* node)
{
  for (int x = 0;x<node->num_registers;x++)
  {
    if (_A7105_Mesh_Cmp_Packet_Register_Name(node->packet_cache,
                                             &(node->registers[x])))
      return x;
  }
  return -1;
}

byte _A7105_Mesh_Cmp_Packet_Register_Name(byte* packet,
                                          struct A7105_Mesh_Register* reg)
{
  byte expected_len = packet[A7105_MESH_PACKET_DATA_START];

  if (reg->_name_len == expected_len &&
      memcmp(reg->_data,
             &(packet[A7105_MESH_PACKET_NAME_START]),
             (size_t)expected_len) == 0)
    return true;
  return false;

}

void _A7105_Mesh_Handling_Request(struct A7105_Mesh* node,
                                  byte* packet)
{
  memcpy(node->last_request_handled,packet,A7105_MESH_PACKET_SIZE);
  node->last_request_handled_time = millis();
}

void _A7105_Mesh_Prep_Finishing_Callback(struct A7105_Mesh* node,
                                             void (*user_finished_callback)(struct A7105_Mesh*,A7105_Mesh_Status,void*),
                                             void (*blocking_finished_callback)(struct A7105_Mesh*,A7105_Mesh_Status,void*))
{
  //Set blocking state vars
  node->blocking_operation_status = A7105_Mesh_NO_STATUS;
  if (user_finished_callback == NULL)
    node->operation_callback = blocking_finished_callback;
  else
    node->operation_callback = user_finished_callback;
}

void _A7105_Mesh_Handle_RX(struct A7105_Mesh* node)
{
  //TODO: Determine if we need to be more 
  //      careful and bail if we send a packet
  //      (i.e. pollute node->packet_cache with our response)
  

  //Bail if there is no packet waiting at the radio
  if (A7105_CheckRXWaiting(&(node->radio)) != A7105_RX_DATA_WAITING)
    return;

  //Fill our packet cache
  //TODO: Maybe we can pass radio errors back to the calling
  //code at some point?
  if (A7105_ReadData(&(node->radio), node->packet_cache,A7105_MESH_PACKET_SIZE) != A7105_STATUS_OK)
    return;

  //Strobe the radio back to the RX state (it auto-jumps back to standby)
  A7105_Easy_Listen_For_Packets(&(node->radio), A7105_MESH_PACKET_SIZE);
  

  DebugHeader(node);
  putstring("RX:[");
  for (int x = 0; x<6;x++)
  {
    Serial.print(node->packet_cache[x],HEX);
    putstring("] [");
  }
  putstring("...\r\n");

  //Check for packets bogusly pushed by duplicate node-id's
  if (_A7105_Mesh_Check_For_Node_ID_Conflicts(node))
    return; //Bail here so nobody processes our sent packet

  //Check for node ID conflicts (all states where we're joined)
  _A7105_Mesh_Handle_Conflict_Name(node);
  
  //Check node ID conflicts while we're joining (deferring to a higher unique ID node)
  _A7105_Mesh_Handle_Deferred_Joining(node);

  //Update the repeat cache with the new packet
  //NOTE: This method should be before any of the Handle_ stuff for requests like GetNumRegisters 
  //      since we use the node state to filter out some response packets (not all)
  _A7105_Mesh_Cache_Packet_For_Repeat(node);

  //Handle any PING requests
  _A7105_Mesh_Handle_Ping(node);

  //Handle any PONG responses
  _A7105_Mesh_Handle_Pong(node);

  //Handle GET_NUM_REGISTERS requests
  _A7105_Mesh_Handle_GetNumRegisters(node);

  //Handle NUM_REGISTERS responses
  _A7105_Mesh_Handle_NumRegisters(node);

  //Handle GET_REGISTER_NAME requests
  _A7105_Mesh_Handle_GetRegisterName(node);

  //Handle REGISTER_NAME response
  _A7105_Mesh_Handle_RegisterName(node);

  //Handle GET_REGISTER request
  _A7105_Mesh_Handle_GetRegister(node);
  
  //Handle REGISTER_VALUE response
  _A7105_Mesh_Handle_RegisterValue(node);

  //Handle SET_REGISTER request
  _A7105_Mesh_Handle_SetRegister(node);

  //Handle SET_REGISTER_ACK response
  _A7105_Mesh_Handle_SetRegisterAck(node);
}

uint16_t A7105_Util_Get_Pkt_Unique_Id(byte* packet)
{
  uint16_t ret = 0;
  ret |= (uint16_t)packet[A7105_MESH_PACKET_UNIQUE_ID];
  ret <<=8;
  ret +=(uint16_t)packet[A7105_MESH_PACKET_UNIQUE_ID+1]; 
  return ret;
}

byte _A7105_Mesh_Filter_Packet(struct A7105_Mesh* node,
                               byte packet_type,
                               byte treat_as_request)
{
  //bail if this packet is the wrong type
  if (node->packet_cache[A7105_MESH_PACKET_TYPE] != packet_type)
    return false;

  //Filter requests using the debounce timer
  if (treat_as_request)
  {
    //If we are outside our request handling debounce window, just 
    //let the request through
    if (millis() - node->last_request_handled_time > A7105_MESH_REQUEST_HANDLING_DEBOUNCE)
      return true; 

    //Otherwise, make sure it's different than the last request handled
    return !_A7105_Mesh_Util_Is_Same_Packet_Sans_Hop(node->packet_cache,
                                                     node->last_request_handled);
  }

  //Filter response packets using the target node/unique id filters
  else if ((node->target_node_id == 0 || node->packet_cache[A7105_MESH_PACKET_NODE_ID] == node->target_node_id) &&
           (node->target_unique_id == 0 || A7105_Util_Get_Pkt_Unique_Id(node->packet_cache) == node->target_unique_id))
    return true;

  return false;
}


void A7105_Mesh_Set_Node_Registers(struct A7105_Mesh* node,
                              struct A7105_Mesh_Register* regs,
                              byte num_regs)
{
  node->registers = regs;
  node->num_registers = num_regs;
}
