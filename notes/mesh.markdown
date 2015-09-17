# Mesh Network v1.0 Overview #

## Primary Canons ##
 1. As stateless as practical. Large state means high RAM usage and inflexible rules. Both are catastrophic.
 2. Requestors are responsible for retries and request state management.
 2. Global registers. Nodes serve registers, it doesn't matter who or where (at the API level).
 3. All non-broadcast packets must be ACK'd (either explicitly or with a response packet)
 4. Max of 255 nodes (0 is a special value meaning broadcast)
 5. Joining is the responsibility of good citizen nodes. There is no gatekeeper.
 6. Easy of use and reliability are the targets, security is not very important (it's a low power mesh network)
 7. Max of 16 hops/repeats to avoid traffic contention.

## Node Characteristics ##

### Salt ###

  Each node has a unique ID (salt) used to handle identify conflicts.
  Nodes on a mesh are numbered 1-255, but this is handled dynamically
  so the salt is used to identify conflicts joining and for nodes
  responding to the same request if they didn't figure out the 
  conflict before it came.

### Presence Table ###

  A 255-bit bitmask (32-byte) is used to keep track of the node-id's
  seen recently for certain operations. It is refreshed only for these
  operations (like getting all registers on the mesh).

### Short Packet History ###
 
  A few (2-3 likely) packets are kept in memory to use for rebroadcasting
  packets as well as avoiding bouncing packets we already re-broadcast.
    
  This can be done with a circular buffer and start/end pointers. 

### Number of registers limited to 64 ###
  64 registers is a ton for these tiny devices and you'll probably run out of
  memory before you can have that many anyway.

  The 64 register rule is for a 64-bit presence table when getting register
  names to ensure we get all of them and can attempt to retry.
  
  This presence table is 8 bytes (obviously)
  
### Target Node Cache ###
  Each node saves the node-id and/or unique-id of a node it is corresponding with
  directly. These values are used to filter response packets a requesting node
  is interested in.

### Last Request Handled Cache ###
  This is a 64-byte cache that stores the last request packet we responded to.
  During a debounce window, if we see the same packet (ignoring hop count), we ignore
  it.  

  This prevents us from responding to the same response from the same node as it bounces 
  around the mesh.

### Packet Filtering ###
  Nodes filter packets for requests, responses and repeating. Below are details on the
  algorithm for each.

  Requests:
    Requests are filtered using the "Last Request Handled Cache" detailed above. If a
    node sees the same request it just handled (ignoring hop count), it will ignore that
    request for the REQUEST_DEBOUNCE interval (probably a few hundred milliseconds).

  Responses:
    Response packets are filtered out by utilizing the "Target Node Cache" described above.
    If either a node-id and/or a unique-id are specified in the cache, only packets from those
    id's will be processed by us. This helps us to manage requests in the state machine such
    as GET_NUM_REGISTERS where we will then know which NUM_REGISTERS packet to pay attention to
    if it comes back (since it will be from the node we originally specified).

    Also, this prevents us from having our target node ID swapped in the middle of long-running
    operations (like retrieving the names of all registers for a given node). Once we get the 
    first NUM_REGISTERS packet back, we can filter on the unique ID of the node also so we won't
    get garbage from a different node if a ID swap happens.


### Packet Repeating ###
To overcome the range limitations of the tiny radios used for the Mesh, all nodes on a Mesh 
act as repeaters and repeat the packets they receive. However, this is not done blindly as
simply repeating all the packets would cause a ton of useless traffic in addition to the 
beneficial.

Aside from the implementation, packets should be repeated up to 16 times (tracked as "hop count"
in the packets themselves). All packets will be repeated, except:
  * If the node receiving a packet is not Joined to a mesh (either joining or not connected at all).
  * The receiving node originally sent the packet (identified by node-ID and unique-ID in the packet
    header).
  * The packet was a request addressed to the receiving node directly (like GET_NUM_REGISTERS) or
    indirectly (like GET_REGISTER for a node that services a particular register). The idea is that
    we're the only ones that are servicing the request, so we don't need to share it further.
  * Packets we previously repeated (identical packets except with a different hop count.

See details of hop count below in the Packet Characteristics description

### Response Repeating ###
Like the packet repeating above, nodes also maintain a cache of responses to requests they've serviced
(PING, GET_REGISTER, etc) and repeat those a few times for every response to overcome the 
inevitable packet collisions that come with multi-millisecond transmission times for packets.

Without this, there is a good chance that responses will get lost in the ether for nodes
based on random timing. Since the sender has no way to know their response made it through, we 
re-send them a few times (tunable, currently 4) at random intervals to make sure they got there.

The repeated responses have the *same* sequence number as the original packet so the general repeaters
running on other nodes can filter it out after their first repeat and not end up flooding the mesh.

As an implementors note, the repeats for things like REGISTER_VALUE can have different values than the 
original transmitted packet if it has changed from the time of the original response. This allows
the implementation to avoid caching entire packets in memory (a very sparse resource on AVR microcontrollers).

The packets that are currently repeated are:
  * PONG
  * NUM_REGISTERS
  * REGISTER_NAME
  * REGISTER_VALUE
  * SET_REGISTER_ACK

### Request Repeating ###

TBD

### Ignoring Duplicate Packets ###

With packet repeating functionality comes the necessity of differentating packets a
node has handled from the delayed repeats of the same packet (so we don't handle the same
packet multiple times).

The solution implemented here is a sequence number included in the second byte of each packet.
This number (0-15) is sent with every request from a node and incremented for every subsequent
request.

Thusly, receiving nodes can differentiate between repeated packets for requests they've already
seen and the original request itself. Additionally, receiving nodes can also differentate between
two identical (but distinct) requests sent from the same node. 

The reference implementation of this mesh has Nodes keeping a "handled packet cache" where the 
packet type, sequence number and sending node Unique-ID are kept in a rotating buffer to filter
packets that have already been acted upon.

The size of this cache is small (4 bytes for every packet) so it can be increased (and should be)
for larger meshes. A good rule of thumb is to make this cache size identical to the number of expected
nodes on the mesh -1 to handle the worst case where every node except one sends a packet at nearly 
the same time to the lone receiver. That node will then need to distinguish between the packets 
it has handled and the repeats it doesn't want).


#### Exception: The JOIN packet ####
  The one exception to the exclusion rules above are that JOIN packets are *always* repeated. 
The reasoning behind this is to maximize visibility of joining nodes to ensure we minimize node-ID
collisions.  Since JOIN packets are repeated a bunch of times while the node is joining, each 
repetition will be repeated regardless and will not be subject to the rule about repeating identical
packets.


### Auto-Rejoin and Node ID ###
The node ID of a node on a mesh can change at any time due to a CONFLICT_NAME being passed 
around. Thusly the user should not trust the ID of a node for any semblance of uniqueness.

The Node-ID is meant to be an easy way to address nodes with a limit on the number that 
can be on the mesh at any time (255) and keep the packet requirements short.


## Packet Characteristics ##

All packets should have the following characteristic

OPERATION | SEQ-NUM/HOP-COUNT | NODE_ID | UNIQUE_ID | <operation-specific-data>

Packets are 64 bytes long at a maximum, operation specific data
is a maximum of 60 bytes long.

The packet header looks like this:
  1. Byte 0: The operation (PING, JOIN, etc)
  2. Byte 1: The sequence number and hop count (upper nibble is sequence, lower is hop)
             See notes below on these fields.
  3. Byte 2: The node-ID of the sender (1-255, 0 is reserved)  
  4. Byte 3-4: The unique-ID of the sender (1-16535, 0 is reserved)

### Hop Count ###
The hop count in the SEQ-NUM/HOP-COUNT byte is a number 0-15(max) denoting
how many times a packet has been repeated by somebody other than the sender.

When a node sends a packet normally, the hop count should be 0. Every node that
repeats the packet should do so verbatim except incrementing the hop count nibble 
by one so the next node will know whether or not the packet can be repeated again.

### Sequence Number ###
The sequence number nibble in the SEQ-NUM/HOP-COUNT byte is a number (0-15) that
is maintained by each node when sending requests (or value broadcasts). This number 
is incremented (15 + 1 rolls over to 0) after each sent packet. Thusly, two identical
requests sent back-to-back from a node can be distinguished from the same packet being
repeated by other nodes in the mesh (since we don't know the route a packet will take,
hop count cannot be used for this reliably).

## Maintaining Node Numbering ##

  To be as effective as possible at correctly assigning and maintaining
  the node numbering on the mesh as nodes come and go, each node generates
  a unique ID (16 or 32 bit, not sure yet) to be included in the packets
  they send.  This deals with the problem of joining the mesh as a certain
  node number that's already in use (the node using it will respond with a 
  CONFLICT_NAME packet) like this:
    * CONFLICT_NAME | HOP/SEQ | NODE_ID | UNIQUE_ID

  Also, this deals with the issue of nodes that appear thinking they're
  already part of the mesh (if they were blocked or something while somebody
  joined and took their ID).

  For example, if we have 5 nodes (1-5) on the network and node 2 is blocked from 
  the mesh signals (we'll call this node ABC) for a few seconds while 
  a 6'th node (DEF) joins (taking the node 2 name).

  If a request comes from node 1 to get the listing of register names 
  from node 2 (which is actually 2 nodes thinking they're on the mesh
  right now since nobody has pinged while they're both on), they both
  will respond.

  How do we deal with ABC and DEF both thinking they're node 2? They
  can't figure it out before-hand because there isn't any traffic going
  on.  They will see each others' REGISTER_NUM response.

  The requestor will process whichever REGISTER_NUM response comes first
  but the nodes themselves will likely see each other's response and 
  the one with the lower unique ID will have to re-join.

  The requestor, will then likely send GET_REGISTER_NAME requests
  to the first-responding node while the other rejoins. Since there
  is only one node 2 now, everything will proceed as usual. 

  Additionally, the requestor will save the unique ID of the node that
  responded to REGISTER_NUM to use for filtering GET_REGISTER_NAME 
  responses as well.

## Register Name Collision ## 
  Obviously, 2 nodes should not be allowed to serve the same register.
  However, it may happen that somebody does this by accident.  Each
  node needs to monitor all packets for responses from any node that
  is not itself claiming to serve a register that the current not serves.
  
  If found, a CONFLICT_REGISTER packet must be sent and the receiving
  node must remove itself from the mesh. The packet looks like this:
  
  CONFLICT_REGISTER | HOP/SEQ | NODE_ID | UNIQUE_ID | TARGET_NODE | TARGET_INQUE_ID | REGISTER_NAME_LEN |REGISTER_NAME
 
## Joining ##
  To join, a node must broadcast a "JOIN" packet at a sub-second frequency
  for multiple seconds before it can declare itself "JOINED." 
  
  The JOIN packet looks like this:
    *JOIN | HOP/SEQ | NODE_ID | UNIQUE_ID | 

  During the time it broadcasts, it must honor "CONFLICT" packets from other
  nodes. These should only come from nodes with the same NODE_ID being 
  requested but having a different UNIQUE_ID to indicate that NODE_ID is
  already taken.

  If the joining node receives a CONFLICT, it can only join by restarting
  the process with a different NODE_ID.

## Ping ##
  Detects presence prior to global operation, has timeout after which
  the Presense Table is considered accurate for the next operation.

  The PING packet looks like this:
    * PING | HOP/SEQ | NODE_ID | UNIQUE_ID

  All nodes on the network respond (after a node-id delay) with:
    * PONG | HOP/SEQ | NODE_ID | UNIQUE_ID 

## Get Register Names (directed, retry) ##

  Getting register names is avaiable to make the mesh discoverable
  with interactive terminals or more powerful hardware. The process
  of getting all registers on the network is:
    1. Ping the mesh to ge presence info
    2. Iterate each found node and send a GET_REGISTERS packet
    3. The node responds with the number of registers
    4. Iterate the number of registers and GET_REGISTER_NAME for each
    5. The node responds with the name of each register 


  The GET_NUM_REGISTERS packet looks like this:
    * GET_NUM_REGISTERS | HOP/SEQ | NODE_ID | UNIQUE_ID | TARGET_NODE_NUM 
  
  The NUM_REGISTERS packet response looks like this:
    * NUM_REGISTERS | HOP/SEQ | NODE_ID | UNIQUE_ID | NUM_REGISTERS

  the GET_REGISTER_NAME packet looks like this:
    * GET_REGISTER_NAME | HOP/SEQ | NODE_ID | UNIQUE_ID | TARGET_NODE_NUM | REGISTER_INDEX

  The response REGISTER_NAME packet looks like this:
    * REGISTER_NAME | HOP/SEQ | NODE_ID | UNIQUE_ID | REGISTER_NAME_LEN |REGISTER_NAME

  Note: If the register index is invalid, the response should just 
        have a REGISTER_NAME_LEN of 0

## Get Register ##

  The process of getting a register value is accomplished by 
  the requestor sending out a broadcast packet to request
  the value of a register. The owner of that register then responds
  with a directed packet to the original requstor.

  The GET_REGISTER packet looks like this:
    * GET_REGISTER | HOP/SEQ | NODE_ID | UNIQUE_ID | REGISTER_NAME_LEN | REGISTER_NAME
   
  The responder (if there is one), sends back a REGISTER_VALUE packet like this:
    * REGISTER_VALUE | HOP/SEQ | NODE_ID | UNIQUE_ID | REGISTER_NAME_LEN | REGISTER_NAME | REGISTER_VALUE_LEN | REGISTER_VALUE

    *NOTE: If NODE_ID = 0, the REGISTER_VALUE packet is considered to be a broadcast. Unique_ID must still be specified

## Set Register ##

  Setting a register is similar to getting a register value, except
  it's all in one packet so there are limits on the size of the register name/value.
  
  The SET_REGISTER packet looks like this:
    * SET_REGISTER | HOP/SEQ | NODE_ID | UNIQUE_ID |REGISTER_NAME_LEN | REGISTER_NAME | REGISTER_VALUE_LEN |REGISTER_VALUE

  If there is a node servicing that register (and the register can be set), it responds like this:
    * SET_REGISTER_ACK | HOP/SEQ | NODE_ID | UNIQUE_ID | TARGET_NODE_NUM | ERR_MSG_DATA | NULL_BYTE

    *NOTE: If there is an error setting a register and the managing node
           wants it known (always a good idea), they should include an ascii 
           error message at the tail (null terminated) explaining what happened
           so the developer can easily diagnose issues.

           If there is ERR_MSG_DATA before the NULL_BYTE, the register was set successfully.
