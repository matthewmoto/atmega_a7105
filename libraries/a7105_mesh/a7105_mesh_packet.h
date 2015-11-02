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
#define A7105_MESH_PKT_SET_REGISTER_ACK    0x0D

#define A7105_MESH_PACKET_TYPE       0
#define A7105_MESH_PACKET_HOP_SEQ    1
#define A7105_MESH_PACKET_NODE_ID    2
#define A7105_MESH_PACKET_UNIQUE_ID  3 //16 bit
#define A7105_MESH_PACKET_TARGET_ID  5 //Not present in all packets
#define A7105_MESH_PACKET_DATA_START 5 //byte that starts the register name/value area
#define A7105_MESH_PACKET_REG_INDEX  6 //byte that has register index for GET_REGISTER_NAME requests
#define A7105_MESH_PACKET_NAME_START  6 //start index of the name data for GET_REGISTER/SET_REGISTER/REGISTER_NAME packets
#define A7105_MESH_PACKET_ERR_MSG_START  6 //start index of the error message for SET_REGISTER_ACK packets

//The different nibbles in the HOP/SEQ byte of the packet
//header for the HOP count and Sequence number (used for
//packet repeat filtering)
#define A7105_MESH_PACKET_HOP_MASK 0x0F
#define A7105_MESH_PACKET_SEQ_MASK 0xF0

//Bytes stored for each entry in the handled packet cache
#define A7105_MESH_HANDLED_PACKET_HEADER_SIZE 4 
#define A7105_MESH_HANDLED_PACKET_OP 0
#define A7105_MESH_HANDLED_PACKET_SEQ 1
#define A7105_MESH_HANDLED_PACKET_UNIQUE_ID 2


#endif
