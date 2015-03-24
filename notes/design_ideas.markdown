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
