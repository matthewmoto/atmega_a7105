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
be ordered on [Osh Park](https://oshpark.com/shared_projects/frDcWgtm) This is not a plug, it's just 
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
` cd resources/ ; unzip arduino-1.6.1-linux32.tar.xz

To build the current sketch (atmega_a7105.ino), type:
` scons

To clean:
` scons -c

To upload to your board:
` scons upload

Note, check out the arscons page and update your arscons.json configuration file if
you aren't running a 5V 16Mhz Pro Mini via a FTDI programmer as I am.

# Debugging Your Nodes #

To enable serial debugging output in the library, just uncomment the line:
` //#define A7105_MESH_DEBUG

This will output internal state and received packet dumps to help diagnose problems.

There is also a script (one liner) in the repository called "serial_mon.sh", running it fires 
up screen and archives the output to screenlog.0
` ./serial\_mon.sh /dev/ttyUSB0  
 
# Examples #
I included a few simple example sketches to use when you're getting started with the
code. These are pretty simple and probably not very useful.

# Mesh Layout #
<include doc/>

