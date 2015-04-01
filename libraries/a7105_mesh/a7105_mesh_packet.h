#ifndef _A7105_Mesh_PACKET_H_
#define _A7105_Mesh_PACKET_H_

#define A7105_MESH_PKT_CONFLICT_NAME       0x01
#define A7105_MESH_PKT_CONFLICT_REGISTER   0x02
#define A7105_MESH_PKT_JOIN                0x03
#define A7105_MESH_PKT_PING                0x04
#define A7105_MESH_PKT_PONG                0x05
#define A7105_MESH_PKT_GET_NUM_REGISTERS   0x06
#define A7105_MESH_PKT_NUM_REGISTERS       0x07
#define A7105_MESH_PKT_GET_REGISTER_NAME   0x08
#define A7105_MESH_PKT_REGISTER_NAME       0x09
#define A7105_MESH_PKT_GET_REGISTER        0x0A
#define A7105_MESH_PKT_REGISTER_VALUE      0x0B
#define A7105_MESH_PKT_SET_REGISTER        0x0C

#define A7105_MESH_PACKET_TYPE       0
#define A7105_MESH_PACKET_HOP        1
#define A7105_MESH_PACKET_NODE_ID    2
#define A7105_MESH_PACKET_UNIQUE_ID  3 //16 bit
#define A7105_MESH_PACKET_TARGET_ID  5 //Not present in all packets
#define A7105_MESH_PACKET_DATA_START 5 //byte that starts the register name/value area

#endif
