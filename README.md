# ATMega A7105 Mesh Network Library #

# What Is This? #
The ATMega A7105 project is a radio library and mesh network stack built for the 
Arduino environment (currently only tested on ATMega328p). The radio library and
mesh library are split so you can use the radios without the mesh logic if you like.

The primary goal for this project is to allow a simple lightweight self-healing network
of microcontrollers to be able to share data easily and overcome the limitations of 
the platform (namely very little RAM and short radio range). For more details on the 
mesh topology and design, see the [Mesh Layout Section](#Mesh-Layout) below.

# Background #

In early 2015, I  bought a handful of what I thought were NRF24L01 radio breakouts
in order to make [my 2015 Halloween Decorations](https://youtu.be/kfExGzUOxbg) wireless. 
Having run 150' of security wire the previous year, accidentally cut a critical line 
partway into the set-up and collecting it all back up the next day, I had a pretty strong motivation to 
make the switch.

In my haste to buy radios, I didn't analyze the Engrish on the product page (I was buying through
DX) and ended up with a bunch of A7105 breakouts (Also known as XL7105-SY-B).

There wasn't a bunch of useful code out there for these chips (the rcgroups.com guys
were probably the most mature but they were only really using a simple protocol for 
controlling mini quadcopters).

Lamenting my poor selection and thinking to just buy the correct radios, I looked 
into the NRF24L01's further and found I didn't really dig what was available for those either (the RF24Mesh
project, cool as it is, required a master node running for all the routing to work).

As a final bad decision (I am not a smart man...), I figured I could make a general purpose lightweight mesh stack that
would solve my problems and put the 10-odd radios that I had purchased to good use.

This code is the result of those bad decisions. After having implemented the mesh, I realize
the radio hardware itself isn't that important as far as the Mesh logic goes 
(it can be adapted to the NRF24L01's or similar at some point). However, I'm using A7105's 
for now. The good news is you can get a pair of radios for under $5USD.

# Hardware #
Because the A7105 is a 3.3V chip and the Pro Mini's I use are 5V, I decided to build shields that had
logic level convertors and a voltage regulator on board so I could just plug the boards in and make them
play nicely together. Additionally, I'm using one of the GIO pins of the radio as a "WTR" pin (it basically
goes low when the radio is done transmitting or receiving so we can use an interrupt to figure out when 
data is ready to read or is done being sent over the air) and the other as the MISO line for the SPI bus. 
Unfortunately, the GIO pins on the radio aren't nice enough to go high-Z when the chip select line is high so 
I also included a tri-state buffer to do that in case this node is used on a bus with other SPI devices.
Finally, the A7105 uses a weird 2MM pitch metric header to mount so the shield also accommodates that.

If you're using 3.3V Arduino's, you won't really need any of this (except possibly the MISO tri-state buffer
if you plan on talking to other SPI devices), but I like 5V stuff since all my other hardware (relay boards, 
MOSFETS, etc) for my Halloween project are 5V.

The Eagle files for my shield are included in the code repository in the hardware/ directory and the PCB can
be ordered on [OSH Park](https://oshpark.com/shared_projects/frDcWgtm) This is not a plug, it's just 
where I had my fabricated. I included the original design files and 
Bill of Materials in case you want to make them yourself or use a different fab house. I can post the Gerber 
files if anyone wants those instead.

This is the rendering of the shield:

![Top](http://frozenpoint.github.io/atmega_a7105/images/board_images/a7105_shield_top.png)
![Bottom](http://frozenpoint.github.io/atmega_a7105/images/board_images/a7105_shield_bottom.png)

Here is what the assembled radio node looks like:

![The Goods](http://frozenpoint.github.io/atmega_a7105/images/board_images/assembled_shield.jpg)

And here is what it looks like with the Pro Mini and XL7105 removed:

![Split Apart](http://frozenpoint.github.io/atmega_a7105/images/board_images/split_shield.jpg)

# Getting Started #

A single node mesh isn't very interesting. Thusly, to start doing things other than being a 
lonely radio singing to yourself, you'll need a pair of nodes to make a mesh.

I included a few simple example sketches to use when you're getting started with the
code. These include test programs used to develop the mesh code and actual production 
code used in the first project (my Halloween 2015 house decorations).

## Register Host Test ##
This sketch can be found at examples/register\_host\_test/atmega\_a7105.ino.  To build it, simply copy 
it to the root of the repo, and do the usual scons build/upload.

The Register Host Test example is meant to be used with the A7105 Shield, but see the hook-up instructions
in the top of the sketch if you want to rig up your own harness.  This harness is mean to be connected and 
then poked with either the Serial Mesh Interface (see below) or a sketch of your own making.

## Serial Mesh Interface ##
This is the firmware for the A7105 Serial Mesh Interface. This firmware                
runs on a Pro Mini (or other Arduino) connected to a host machine via in FTDI
serial interface.

The Serial interface runs at 115200 baud and the serial\_mon.sh script can be used
with an argument (e.g. /dev/ttyUSB0) to start communicating with the node. This is the 
most useful example and is the actual bridge firmware used for my Halloween 2015 Prop Controller
script (there is a Python library that sits on the serial port and talks to the mesh).

See the Hook-Up instructions in the comments of the sketch for help getting this connected if you aren't
using the Pro Mini shield.
                                                                                       
NOTE: All Serial commands return either the specified "Response:" response defined below or 
the universal error response that looks like this: `"ERROR,ERROR_ID,MSG\r\n"`

Where `ERROR_ID` is a non-zero integer defined below and `MSG` is a contextually helpful
plaintext ascii string to aid in identifying the issue.

All responses or interrupting data will be one or more comma delimited lines ending in `/r/n`.

Serial Commands:                                                                       
```  
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
    
    NOTE: REGISTER_VALUE is interpreted differently depending on FORMAT. If FORMAT is:
      * STRING: REGISTER_VALUE is sent as a string whose value is terminated by /r/n on
                the serial terminal (not included in value sent).
      * BINARY: REGISTER_VALUE is taken to be a byte-aligned ASCII_HEX string whose value is sent
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
  and will be pushed to the serial terminal at the time of arrival, however it will not precede the 
  current operation. 
```
## Building The Code ##
The example sketches are all based on building with [arscons](https://github.com/suapapa/arscons).

However, there are a few caveats. Namely, the code expects an Arduino distribution in the resources/
directory. One is included, but needs to be unpacked first:
` cd resources/ ; unzip arduino-1.6.1-linux32.tar.xz`

Also, arscons expects the sketch name to be the same as the directory it is in (the reason while all examples
are named atmega-a7105.ino in their respective directories).

To build an example (e.g. the serial_mesh_interface example) and push it to an arduino:
```
  (from the repo root)
  $ cp examples/serial_mesh_interface/atmega_a7105.ino .
  scons ARDUINO_PORT=/dev/ttyUSB0 upload #substitute your serial port name where applicable
```

To build the current sketch (atmega_a7105.ino), type:
`scons`

To clean:
`scons -c`

To upload to your board:
`scons ARDUINO_PORT=YOUR_SERIAL_PORT upload`

Note: check out the arscons page and update your arscons.json configuration file if
you aren't running a 5V 16Mhz Pro Mini via a FTDI programmer as I am.

Also, if you are using the graphical Arduino IDE, feel free to just copy the libraries from their directories
and place in your Arduino library path. I haven't tested this method yet so it may need a little fiddling 
to get it working.

# Debugging Your Nodes #

Sometimes the mesh won't do what you want. This might be a bug, it might be your code or (most likely) it might 
be a connector that has a bit of solder on it or is bent or something else that makes me FURIOUS, AAAAAHHHHH!!!


Regardless, the best way to see what your radio is doing is to toggle the  just uncomment the line:
`#define A7105_MESH_DEBUG` in a7105_mesh.h. This will output internal state and received packet dumps to help diagnose problems.

There is also a script (one liner) in the repository called "serial_mon.sh", running it fires 
up screen and archives the output to screenlog.0
` ./serial\_mon.sh /dev/ttyUSB0`  

# Mesh Layout #

## Simplified Explanation ##
In order to ease into what is meant in this project by the word "Mesh," presented here is a 
simplified idea of how things go.

Given N nodes (nodes being ATMega microcontrollers with a radio attached), they all talk to 
each other and serve registers (values with names attached).  For a concrete example, consider
3 nodes: A, B and C.  A serves a register "Motion" that correlates to a motion sensor attached to
it. B doesn't serve any nodes, but responds to certain broadcasts (e.g. "BLOW\_FOG") and C doesn't
serve registers either, but runs a serial interface to a host computer for a program or user to poke 
the mesh.

In this example, for there to be a "Mesh," the nodes need to agree on a few things:
 1. They are on a mesh
 2. What their "node ID" is (a value from 1-255 to identify the node on the mesh).
 3. That they have a 16-bit unique ID

To do this, each node starts broadcasting (at a pseudo random interval), that it wants to join a mesh 
and that it's node ID is 1 (everybody starts at 1), and it's unique ID is whatever it is. As these packets
go out over the radio, any conflicts (duplicated node ID's ) are seen by the nodes and CONFLICT_NAME packets
are sent out as a response.  The node with the lower unique ID increments their requested node ID and starts 
the JOIN process over again. 

If a node get's through the multi-second JOIN broadcasts without having CONFLICTS come back for their selected node ID, it assumes itself 
to be "on the mesh with that ID" It's important to note, that the conflict packets can come at any time and the rules still
apply (the lower unique ID always re-joins as a different ID). It is this property that makes the mesh "self 
healing" where if two groups of nodes exist that can't talk to each other (two separate meshes) suddenly can
communicate (e.g. by moving closer together), they will conflict and re-order themselves to be a single mesh.


So, back to the example, a bunch of JOIN packets fly around. A few CONFLICT's happen and the nodes A, B and C
join as 1,2 and 3 (not necessarily in that order). Now, if node C starts sending requests to get the value of
the "Motion" register, it doesn't care what node actually hosts that value, just that one does and value 
responses come back (i.e. a nodes ID isn't considered a permanent thing).

All nodes respond to PING packets with PONG packets (for simple presence detection). The PING-ing node keeps a 
presence bitmask (255 bit) that can be used to iterate and poke every seen node on the mesh.  For discovering what
registers are available on a mesh, a node performs the following steps:
 1. PING all the nodes to find available nodes
 2. For every found node, query how many registers it serves (GET_NUM_REGISTERS packet)
 3. For the current node, iterate from 0 to N (N being the number of served registers) and query the register name for each (GET_REGISTER_NAME)

If a node knows the name of the register it wants to query, it can simply send a GET_REGISTER packet with the name of the register in question
and expect a REGISTER_VALUE packet in response (assuming the node serving that register is currently available). If a node wants to remotely set a
register value, it can likewise send a SET_REGISTER packet with the name and new value of the register. For SET_REGISTER requests, a 
SET_REGISTER_ACK packet is expected to come back with either success or some error message from the remote node explaining an error (in ascii).

Beyond this, there is only one special case for getting/setting registers: value broadcasts. Since node numbering starts at 0, if a 
REGISTER_VALUE packet is seen with a node ID of 0 in the header, it is assumed to be a broadcast. These broadcasts are repeated through the
mesh by all nodes and are meant for notification of global events. They are not ACK'd by any nodes. Broadcasts can be useful if a node needs to
push a notification to the mesh rather than being polled for a value.


## Primary Canons ##
 1. As stateless as practical. Large state means high RAM usage and inflexible rules. Both are catastrophic.
 2. Requesters are responsible for retries and request state management.
 2. Global registers. Nodes serve registers, it doesn't matter who or where (at the API level).
 3. All non-broadcast packets must be ACK'd (either explicitly or with a response packet)
 4. Max of 255 nodes (0 is a special value meaning broadcast)
 5. Joining is the responsibility of good citizen nodes. There is no gatekeeper.
 6. Easy of use and reliability are the targets, security is not very important (it's a low power mesh network)
 7. Avoid traffic contention whenever possible (don't repeat packets blindly).

## Packet Characteristics ##

All packets should have the following characteristics:

```OPERATION | SEQ-NUM/HOP-COUNT | NODE_ID | UNIQUE_ID | <operation-specific-data>```

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
    `CONFLICT_NAME | HOP/SEQ | NODE_ID | UNIQUE_ID`

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

  The requester will process whichever REGISTER_NUM response comes first
  but the nodes themselves will likely see each other's response and 
  the one with the lower unique ID will have to re-join.

  The requester, will then likely send GET_REGISTER_NAME requests
  to the first-responding node while the other rejoins. Since there
  is only one node 2 now, everything will proceed as usual. 

  Additionally, the requester will save the unique ID of the node that
  responded to REGISTER_NUM to use for filtering GET_REGISTER_NAME 
  responses as well.

## Joining ##
  To join, a node must broadcast a "JOIN" packet at a sub-second frequency
  for multiple seconds before it can declare itself "JOINED." 
  
  The JOIN packet looks like this:
    `JOIN | HOP/SEQ | NODE_ID | UNIQUE_ID |`

  During the time it broadcasts, it must honor "CONFLICT" packets from other
  nodes. These should only come from nodes with the same NODE_ID being 
  requested but having a different UNIQUE_ID to indicate that NODE_ID is
  already taken.

  If the joining node receives a CONFLICT, it can only join by restarting
  the process with a different NODE_ID.

## Ping ##
  Detects presence prior to global operation, has timeout after which
  the Presence Table is considered accurate for the next operation.

  The PING packet looks like this:
    `PING | HOP/SEQ | NODE_ID | UNIQUE_ID`

  All nodes on the network respond (after a node-id delay) with:
    `PONG | HOP/SEQ | NODE_ID | UNIQUE_ID`

## Get Register Names (directed, retry) ##

Getting register names is available to make the mesh discoverable
with interactive terminals or more powerful hardware. The process
of getting all registers on the network is:
 1. Ping the mesh to get presence info
 2. Iterate each found node and send a GET_REGISTERS packet
 3. The node responds with the number of registers
 4. Iterate the number of registers and GET_REGISTER_NAME for each
 5. The node responds with the name of each register 


  The GET_NUM_REGISTERS packet looks like this:
    `GET_NUM_REGISTERS | HOP/SEQ | NODE_ID | UNIQUE_ID | TARGET_NODE_NUM`
  
  The NUM_REGISTERS packet response looks like this:
    `NUM_REGISTERS | HOP/SEQ | NODE_ID | UNIQUE_ID | NUM_REGISTERS`

  The GET_REGISTER_NAME packet looks like this:
    `GET_REGISTER_NAME | HOP/SEQ | NODE_ID | UNIQUE_ID | TARGET_NODE_NUM | REGISTER_INDEX`

  The response REGISTER_NAME packet looks like this:
    `REGISTER_NAME | HOP/SEQ | NODE_ID | UNIQUE_ID | REGISTER_NAME_LEN |REGISTER_NAME | REGISTER_INDEX`

  Note: If the register index is invalid, the response should just 
        have a REGISTER_NAME_LEN of 0

## Get Register ##

  The process of getting a register value is accomplished by 
  the requester sending out a broadcast packet to request
  the value of a register. The owner of that register then responds
  with a directed packet to the original requester.

  The GET_REGISTER packet looks like this:
    `GET_REGISTER | HOP/SEQ | NODE_ID | UNIQUE_ID | REGISTER_NAME_LEN | REGISTER_NAME`
   
  The responder (if there is one), sends back a REGISTER_VALUE packet like this:
    `REGISTER_VALUE | HOP/SEQ | NODE_ID | UNIQUE_ID | REGISTER_NAME_LEN | REGISTER_NAME | REGISTER_VALUE_LEN | REGISTER_VALUE`

NOTE: If NODE_ID = 0, the REGISTER_VALUE packet is considered to be a broadcast. Unique_ID must still be specified

## Set Register ##

  Setting a register is similar to getting a register value, except
  it's all in one packet so there are limits on the size of the register name/value.
  
  The SET_REGISTER packet looks like this:
    `SET_REGISTER | HOP/SEQ | NODE_ID | UNIQUE_ID |REGISTER_NAME_LEN | REGISTER_NAME | REGISTER_VALUE_LEN |REGISTER_VALUE`

  If there is a node servicing that register (and the register can be set), it responds like this:
    `SET_REGISTER_ACK | HOP/SEQ | NODE_ID | UNIQUE_ID | TARGET_NODE_NUM | ERR_MSG_DATA | NULL_BYTE`

NOTE: If there is an error setting a register and the managing node
wants it known (always a good idea), they should include an ascii 
error message at the tail (null terminated) explaining what happened
so the developer can easily diagnose issues.
If there is ERR_MSG_DATA before the NULL_BYTE, the register was set successfully.

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
    
  This is done with a circular buffer at the moment. 

### Target Node Cache ###
  Each node saves the node-id and/or unique-id of a node it is corresponding with
  directly. These values are used to filter response packets a requesting node
  is interested in.

### Handled Request Cache ###
  This is a cache that stores metadata about recently handled requests (operation,
  sequence number, unique ID of requester) used to prevent responding to a request
  more than once as it bounces around the mesh.

### Packet Filtering ###
  Nodes filter packets for requests, responses and repeating. Below are details on the
  algorithm for each.

#### Requests ####
Requests are filtered using the "Handled Request Cache" detailed above. If a
node sees the same request it just handled, it will ignore that request.  Another
identical request from the same requesting node *will* be handled due to the fact that
the new request will almost be guaranteed to have a different sequence number than the one in the cache.    

#### Responses ####
Response packets are filtered out by utilizing the "Target Node Cache" described above.
If either a node-id and/or a unique-id are specified in the cache, only packets from those
id's will be processed by us as responses. This helps us to manage requests in the state machine such
as GET_NUM_REGISTERS where we will then know which NUM_REGISTERS packet to pay attention to
if it comes back (since it will be from the node we originally specified).

Also, this prevents us from having our target node ID swapped in the middle of long-running
operations (like retrieving the names of all registers for a given node). Once we get the 
first NUM_REGISTERS packet back, we can filter on the unique ID of the node also so we won't
get garbage from a different node if a ID swap happens.


### Packet Repeating ###
To overcome the range limitations of the tiny radios used for the Mesh, all nodes on a Mesh 
act as repeaters and repeat the packets they receive. However, this is not done blindly as
simply repeating all the packets would cause a ton of useless traffic.

Packets should be repeated up to a maximum of 16 times (tracked as "hop count"
in the packets themselves); the setting may be lower depending on the implementation. 

All packets will be repeated, *except*:
  * If the node receiving a packet is not Joined to a mesh (either joining or not connected at all).
  * The receiving node originally sent the packet (identified by node-ID and unique-ID in the packet
    header).
  * The packet was a request addressed to the receiving node directly (like GET\_NUM\_REGISTERS) or
    indirectly (like GET_REGISTER for a node that services a particular register). The idea is that
    we're the only ones that are servicing the request, so we don't need to share it further.
  * Packets we previously repeated (identical packets except with a different hop count.

See details of hop count below in the Packet Characteristics description.

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

Each node maintains a pending request cache that saves the packet for any outgoing requests
and repeats them a few times for the same reasons we repeat responses above.

### Ignoring Duplicate Packets ###

With packet repeating functionality comes the necessity of differentiating packets a
node has handled from the delayed repeats of the same packet (so we don't handle the same
packet multiple times).

The solution implemented here is a sequence number included in the second byte of each packet.
This number (0-15) is sent with every request from a node and incremented for every subsequent
request.

Thusly, receiving nodes can differentiate between repeated packets for requests they've already
seen and the original request itself. Additionally, receiving nodes can also differentiate between
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

# Mesh API #

The API and main entry points are all documented in a7105\_mesh.h with explanations of parameters and 
possible return values.  Instead of duplicating it here, the reader is encouraged to study the example
programs provided and the in-code documentation.

Additionally, there are a number of packet-related magic numbers specified in a7105_mesh_packet.h that
may be helpful.

## Callback vs Blocking ##

The A7105_Mesh API is designed to have every entry point optionally have a callback
for asynchronous behavior. The novice programmer who doesn't want or need this feature
can safely pass NULL for these arguments and have the library behave as expected (i.e. the
calling code wait for the successful completion of the operation). However, the developer 
having their code perform other time-critical operations (or simply one using multiple radios
simultaneously) have the option to pass a callback to be called when the current operation
(join, get_register, ping, etc.) completes with a status code so the main event loop isn't 
monopolized by the radio code.
