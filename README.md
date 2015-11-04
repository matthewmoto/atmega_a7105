# ATMega A7105 Mesh Network Library #

# Introduction #

As of the time of this writing, there isn't a ton of code out there to
support Arduino development or the A7105 radio chip (the rcgroups.com guys
are probably the most mature with several stale github projects behind 
them). I mistakenly bought a bunch of these thinking they were NRF24L01's
in the beginning of 2015 and decided to start building a software stack
for them.  

Coupled with some Pro Mini's and patience, I built a mesh network stack 
based on these little radios. 

# Building The Code #
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

# Mesh Network Structure #
Check out /notes/mesh.markdown for an overview of the mesh network features and packet structures.
