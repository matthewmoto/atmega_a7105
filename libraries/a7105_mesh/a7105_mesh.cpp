#include <stdint.h>
#include <SPI.h>
#include "a7105_mesh.h"


void DebugHeader(struct A7105_Mesh* node)
{
  Serial.print(millis());
  Serial.print(" : (");
  Serial.print(node->unique_id,HEX);
  Serial.print(") ");
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
  node->registers = NULL;
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
                             A7105_Mesh_Status status)
{
  //Set the operation_status so the original function can
  //realize we're done and return the value
  node->blocking_operation_status = status;
}


//Special callback for when we're interrupted with a re-join
//mid-operation. This restores the state pre-interrupt and 
//pushes the original packet again with the new node-id
void _rejoin_finished(struct A7105_Mesh* node,
                             A7105_Mesh_Status status)
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
                                  void (*join_finished_callback)(struct A7105_Mesh*,A7105_Mesh_Status))
{
  return A7105_Mesh_Join(node, 1, join_finished_callback, false);
}
A7105_Mesh_Status A7105_Mesh_Join(struct A7105_Mesh* node, 
                                  byte node_id,
                                  void (*join_finished_callback)(struct A7105_Mesh*,A7105_Mesh_Status))
{
  return A7105_Mesh_Join(node, node_id, join_finished_callback, false);
}
A7105_Mesh_Status A7105_Mesh_Join(struct A7105_Mesh* node, 
                                  byte node_id,
                                  void (*join_finished_callback)(struct A7105_Mesh*,A7105_Mesh_Status),
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
  Serial.print("Attempting to join as ");
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
    node->operation_callback(node, A7105_Mesh_STATUS_OK);
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
    Serial.print("deferring JOIN to ");
    Serial.print(A7105_Util_Get_Pkt_Unique_Id(node->packet_cache),HEX);
    Serial.println(" since they have a higher unique, rejoining...");
    
  
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
    Serial.print("Found different node pushing packets as us ->");
    Serial.print(A7105_Util_Get_Pkt_Unique_Id(node->packet_cache),HEX);
    Serial.println(" sending name conflict");
  
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

  //Update GET_NUM_REGISTERS activity 

  //Update GET_REGISTER_NAME activity 

  //Update GET_REGISTER activity 

  //Update SET_REGISTER activity 

  //Update packet repeat activity 
  _A7105_Mesh_Update_Repeats(node);
  

  //HACK: NUKEME
  return A7105_Mesh_STATUS_OK;
}



A7105_Mesh_Status A7105_Mesh_Ping(struct A7105_Mesh* node, 
                                  void (*ping_finished_callback)(struct A7105_Mesh*,A7105_Mesh_Status))
{
  //Check our status (make sure we are on a mesh and not busy)
  if (node->state == A7105_Mesh_JOINING ||
      node->state == A7105_Mesh_NOT_JOINED)
    return A7105_Mesh_NOT_ON_MESH;

  if (node->state != A7105_Mesh_IDLE)
    return A7105_Mesh_BUSY;

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
    node->state = A7105_Mesh_IDLE;

    //NUKEME
    int num_nodes = 0;
    for (int x = 1;x<256;x++)
    {
      if (node->presence_table[x/8] & (byte)(1<<(x%8)))
        num_nodes++;
    }


    DebugHeader(node);
    Serial.print("Finished ping, found ");
    Serial.print(num_nodes);
    Serial.println(" other nodes");
  }
}




//////////////////////// Utility Functions ///////////////////////
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
    Serial.print("REPEAT:[");
    for (int x = 0; x<5;x++)
    {
      Serial.print(packet[x],HEX);
      Serial.print("] [");
    }
    Serial.println("...");

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
      node->packet_cache[A7105_MESH_PACKET_TYPE] == A7105_MESH_PKT_GET_REGISTER_NAME) &&
      node->packet_cache[A7105_MESH_PACKET_NODE_ID] == node->node_id)
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
  //HACK: zero out the packet so our packet comparison function is easy to write
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
  Serial.print("TX:[");
  for (int x = 0; x<5;x++)
  {
    Serial.print(node->packet_cache[x],HEX);
    Serial.print("] [");
  }
  Serial.println("...");

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

void _A7105_Mesh_Send_Request(struct A7105_Mesh* node)
{
  //Update the request sending time
  node->request_sent_time = millis();
  _A7105_Mesh_Send_Response(node);

}
                            

byte _A7105_Mesh_Prep_Pkt_Register_Name_Value(byte* packet,
                                              byte* reg_name,
                                              byte reg_name_len,
                                              byte* reg_val,
                                              byte reg_val_len)
{
  //Make sure the data isn't too large for the packet
  if ((int)reg_name_len + (int)reg_val_len > (int)(A7105_MESH_PACKET_SIZE - A7105_MESH_PACKET_DATA_START))
    return 0;

  int offset = A7105_MESH_PACKET_DATA_START; 
  packet[offset] = reg_name_len;
  offset++;
  memcpy(&(packet[offset]),reg_name,reg_name_len);
  offset+= reg_name_len;
  packet[offset] = reg_val_len;
  offset++;
  memcpy(&(packet[offset]),reg_val,reg_val_len);
  return 1;
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

void _A7105_Mesh_Handling_Request(struct A7105_Mesh* node,
                                  byte* packet)
{
  memcpy(node->last_request_handled,packet,A7105_MESH_PACKET_SIZE);
  node->last_request_handled_time = millis();
}

void _A7105_Mesh_Prep_Finishing_Callback(struct A7105_Mesh* node,
                                             void (*user_finished_callback)(struct A7105_Mesh*,A7105_Mesh_Status),
                                             void (*blocking_finished_callback)(struct A7105_Mesh*,A7105_Mesh_Status))
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
  Serial.print("RX:[");
  for (int x = 0; x<5;x++)
  {
    Serial.print(node->packet_cache[x],HEX);
    Serial.print("] [");
  }
  Serial.println("...");

  //Check for packets bogusly pushed by duplicate node-id's
  if (_A7105_Mesh_Check_For_Node_ID_Conflicts(node))
    return; //Bail here so nobody processes our sent packet

  //Check for node ID conflicts (all states where we're joined)
  _A7105_Mesh_Handle_Conflict_Name(node);
  
  //Check node ID conflicts while we're joining (deferring to a higher unique ID node)
  _A7105_Mesh_Handle_Deferred_Joining(node);

  //Update the repeat cache with the new packet
  _A7105_Mesh_Cache_Packet_For_Repeat(node);

  //Handle any PING requests
  _A7105_Mesh_Handle_Ping(node);

  //Handle any PONG responses
  _A7105_Mesh_Handle_Pong(node);
  
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
