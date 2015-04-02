% A7105 Mesh Network Design Notes
% Matt Meno
% March 1, 2015

# Introduction #

I was originally looking for NRF24L01 boards to use for the Halloween 2015 projects (smoke machines, motion sensors, etc), but accidentally bought A7105 breakouts.  After initially being disconcerted, I'm actually kinda stoked about building some software to use with these things. They seem to have a longer range than the NRF24L01 and nobody is really using them outside quadcopter controls at the moment.

# Goal #

My goal is to have AVR software that will let me use the A7105 boards to pass data around robustly in a mesh-type network to (at first) control my haloween decorations for 2015. These include:
  * RGB LED lighting panels
  * Smoke machines
  * Sound Effects (via a RPi) 
  * (maybe) Projected video (also via a RPi)
  * door open switch (front door to disable the scary stuff)

The secondary requirements of this setup are:
  * Messages must pass between nodes since the scene controller hardware will not be in RF range of the furthest effects.

# Not Re-Inventing The Wheel #
  There are a bunch of projects out there for low complexity mesh-networks and I'd really like to avoid building my own custom one if I can avoid it. 

Some of the things I've reviewed are detailed below.

## 802.15.4 ##
This specification is the IEEE approach to WPAN networks and, while reasonably comprehensive, it is quite above and beyond what I need or want to implement with my hardware and microcontrollers. Amtel does make AVR controllers that implement this protocol, but for me it seems a little overly-complicated. That isn't to say I can't use some of the concepts from it to make my software better; these guys have obviously put some thought into the requirements for this kind of network.

## RF24Nework ##
Written buy a guy called ManiacBug and forked by TMRh20, this is a tree-type library for NRF24L01 libraries to talk to each other. The implementation is non-standard, but the guy has some interesting ideas on how to make these little modules talk to each other. I originally wanted to implemented a hardware layer at the bottom and use his network code on top, however the tree-organzation and routing-table approach doesn't appeal to me.  My network nodes should be able to move around so the routing stuff is really kinda fruitless.

## SWAP ##
Part of the PanStamp project, this is a very simple lynda-space type of deal where there are register maintainers (registers being things like a global variable maintained by the network) that hold state that is modified and used by the nodes on the network. Very simple routing is implemented without static tables and is is pretty simple overall.

The restriction of this approach is that high data throughput is limted. I'm definitely still looking at this one. It might be the project I extend to use my radio modules.

## API Interface ##

There is always a decision about how to present data and control for an API. Since we're in a single-threaded C environment with 
limited resources, this presents some limitations to guide our choices.

Here is the list of things we know:
  * RAM is limited. State is expensive so it must be kept to a minimum.
  * Mesh operations can sometimes take over a second with slow data rates and shoddy connectivity.  
  * The client code is doing other things, sometimes time-sensitive things that cannot wait for a long operation to finish.
  * Mesh operations often need to steal some cycles to push data around not related to the client code (repeats, pings, etc).
  * Sometimes the client code cannot do anything until the mesh returns a value anyway.
  
From this, it becomes clear that a non-blocking solution (while harder to use) would be preferrable for time sensitive clients.
However, for debugging and simple appliances, a blocking client might be helpful to keep the high-level code easy to read. 
Perhaps making the back-end non blocking but allowing the interface to have a wait option since our Nodes are already state-machines
that assume they're not in control of execution flow.

The basic operations of the mesh are broken down below with their expected behavior. Due to the limitations of RAM, the architecture
of the mesh library is organized so only one operation can be requested at a time (Ping, Get Register Name, etc). This means that
multi-operation activities will require additional state maintenance, but it keeps the core library small so RAM doesn't get
spent unless we absolutely need it.

### Multiple Radios ###
For some unforseen reason a microcontroller wants to run two independent nodes with two A7105 radio modules, it certainly can.
However, there is one important thing to remember with the library. If the calling code does not specify a callback and 
uses operations on one Mesh node with the blocking interface, the other node will *NOT* process any radio traffic during 
that operation. In 99% of cases, this is bad.

The solution is to specify your own finished() callbacks for any operations and call A7105_Mesh_Update() on both nodes in
your main loop. This is the format used for the test harness used while developing this library.

### Calling A7105_Mesh_Update ###
It's important to remember that, if you're using the non-blocking API for the Mesh, you'll need to regularly call A7105_Mesh_Update()
to let the library shuffle packets around and complete operations. Of course, regularly should mean "whenever your code isn't actively
doing something else." 

Failure to call A7105_Mesh_Update() (even during idle time) will result in decreased Mesh performance and reliability since 
the Mesh requires all nodes to act as repeaters to get packets where they need to go and won't allow your node to do this or 
respond to requests during this time.

Also, a node in the Mesh may be required to Re-Join at any time if it is discovered to be in conflict of node-id's with another
node. The node with the lower randomly selected Unique ID will be the one to rejoin if both were already on the mesh (and didn't 
notice each other's ID due to connectivity or other issues) or during a Join operation with a node trying to become a node-ID that
already exists on the mesh.

The library hides this re-join operation to aleviate the complexity from the client code, however, this will have an unseen 
time penalty for the currently running operation as a re-join can take a while.

### Join ###
This operation is guaranteed to take at least a second (with current constants). If you're code needs to be doing anything 
during that time, it's better to pass your own join finished callback and call A7105_Mesh_Update() intermittently rather
than blocking the whole time.

    A7105_Mesh_Status A7105_Mesh_Join(struct A7105_Mesh* node, void (*join_finished_callback)(A7105_Mesh_Status))
      * node: Must be a node successfully initialized with A7105_Mesh_Initialize()
      * join_finished_callback: The function called when the JOIN operation completes or times out. If this is NULL,
                                A7105_Mesh_Join will use an internal callback and block until the operation completes 
                                or times out. 

### Ping ###
The client code wants to see how many nodes are on the Mesh. This is typically done for debugging purposes or 
for iteration efficiency to poll never node on the Mesh. Like Join, this operation will likely be long running 
since the library waits a set amount of time for all nodes to respond.

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
                                

### GetNumRegisters ###
This request queries a particular node ID for the number of registers they have. As long as the target node is joined 
to a Mesh, this number *cannot* change.
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
  
### GetRegisterName ###
This requests the name of a register served by a node-id by index number [0-(length-1)] where "length" is retrieved 
from the node previously by calling GetNumRegisters.

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
 
### GetRegister ###
This is a global request for a register value from any node serving it. This can be the node that services the value or it can be 
a node that stores all values and serves them to the network.

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
 
### SetRegister ###
This is a global request to set a register value for any node serving it. This can be the node that services the value or it can be 
a node that stores all values and serves them to the network.

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
 
