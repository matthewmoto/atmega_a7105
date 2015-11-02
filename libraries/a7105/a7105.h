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

----

  This library sits right on top of the A7105 Hardware and has all the low-level SPI utilities
  to access registers/FIFO's and whatnot on the chip. This is the most powerful, but least valuable
  aspect of this library. The more important things here are:
    * The Status/Register/Strobe enums that mask some of the hex soup associated with this chip
    * The A7105 structure definition that encapsulates the chip-select in and states so one AVR can talk to multiple A7105's
    * The *_Easy_* functions and ReadData, WriteData that hide some of the complexity for using the A7105 without knowing 
      the gritty details of the hardware and RF comms like sideband selection, etc.
    * The pin interrupt functionality that let's the user see RX data waiting by using the GPIO2 pin on the A7105 as a WTR pin
      that goes high only during TX/RX activity.

  Software Notes:
    I used the PinChangeInt() library to trigger the RX data waiting interrupts and this has only been tested using
    an ATMEGA328P. The use of other chips is untested.

    This library was written using the Arduino-1.6.1 distribution (includes the transactional SPI library).

  Hardware Notes:
    This library was written/tested using an Arduino Pro Mini and the XL7105-SY-B breakouts from DX.com. these
    breakouts are basically the reference implementation (16Mhz crystal, etc) so this library should work with 
    pretty much any A7105 as long as it's using a 16Mhz crystal. 


    SPI Communication Notes:
      Additionally, I used a SN74LVC245A unidirectional level-shifter chip to talk with the radio using my 5V
      Ardiuno. In doing so, I noticed the GPIO1 pin used as the MISO for SPI comms does *NOT* obey the high-Z
      state when the chip select isn't enabled. This means that if you're using multiple A7105's on the same SPI 
      bus, you'll need a tri-state buffer to let them talk back or the non-talking one will sink current and you'll 
      miss the info. 

      As a quick hack, (since I didn't have a tri-state buffer hanging around), I used a pair of schottkey diodes
      and a 10K pull-down resistor to make the MISO line. I used schottkey diodes since I was pumping the 3v3 line 
      from the radios into an input pin on a 5V arduino. A regular diode might not have allowed me to see the
      3v3 as a logical 1 due to the 0.7V voltage drop. The pull down keeps the MISO line low but doesn't sink too much 
      current so either radio can chat.

  -Matt Meno (matthew.meno@gmail.com), March 27 2015
*/
#ifndef _A7105_H_
#define _A7105_H_

#include <stdint.h>

#define A7105_ENABLE_4WIRE 0x19 //GPIO register value to make the pin a MISO pin (for 4-wire SPI comms)
#define A7105_GPIO_WTR 0x01 //Code to set a GIO pin to do WTR activity (be high during transmit/receive and low otherwise)

//NOTE: This is 1/2 the maximum value specified in the datasheet. 10000000 was 
//      giving me issues with my ghetto breadboard test harness so feel free to alter 
//      this if you wish, but be careful pushing it too fast.
#define A7105_SPI_CLOCK 5000000 //Clock rate for SPI communications with A7105's

#define CRC_ENABLE_MASK 0x08 //Mask to enable CRC on register 1F (code register 1)
#define FEC_ENABLE_MASK 0x10 //Mask to enable FEC on register 1F (code register 1)
#define CRC_CHECK_MASK 0x20 //check CRC bit on register 00 (mode)
#define FEC_CHECK_MASK 0x40 

#define A7105_HIGHEST_CHANNEL 0xA8 //Defined in datasheet pg. 59

#define A7105_CALIBRATION_TIMEOUT 500 //milliseconds before we call autocalibrating a failure 
                                      //(this is *way* more time that the datasheet calls out, maybe we should reduce this?)
#define A7105_MAX_FIFO_SIZE 64 //largest amount of data that will fit in the FIFO in Easy mode

//Status codes for returns from the various functions in this library
//These don't have any representation in the datasheet; they're just
//for ease of use with this library (see individual functions for the meanings in context).
enum A7105_Status_Code{
  A7105_STATUS_OK,
  A7105_INIT_ERROR,
  A7105_CALIBRATION_ERROR,
  A7105_INVALID_CHANNEL,
  A7105_INVALID_FIFO_LENGTH,
  A7105_RX_DATA_WAITING,
  A7105_RX_DATA_INTEGRITY_ERROR,  
  A7105_NO_DATA,
  A7105_NO_WTR_INTERRUPT_SET,
  A7105_BUSY,
};

//The states from the A7105. These are the "Strobe" commands
//used to switch the modes of the radio.
enum A7105_State {
    A7105_SLEEP     = 0x80,
    A7105_IDLE      = 0x90,
    A7105_STANDBY   = 0xA0,
    A7105_PLL       = 0xB0,
    A7105_RX        = 0xC0,
    A7105_TX        = 0xD0,
    A7105_RST_WRPTR = 0xE0,
    A7105_RST_RDPTR = 0xF0,
};

//Internal states for the WTR pin interrupt tracking
//these aren't visible outside the library.
enum {
  A7105_INT_IGNORE_ONE = 0,
  A7105_INT_NULL = 1,
  A7105_INT_TRIGGERED = 2
};

//All the registers available in the A7105. These 
//make up the bulk of the datasheet. check out the 
//code of this library for some examples.
enum {
    A7105_00_MODE         = 0x00,
    A7105_01_MODE_CONTROL = 0x01,
    A7105_02_CALC         = 0x02,
    A7105_03_FIFO_I       = 0x03,
    A7105_04_FIFO_II      = 0x04,
    A7105_05_FIFO_DATA    = 0x05,
    A7105_06_ID_DATA      = 0x06,
    A7105_07_RC_OSC_I     = 0x07,
    A7105_08_RC_OSC_II    = 0x08,
    A7105_09_RC_OSC_III   = 0x09,
    A7105_0A_CK0_PIN      = 0x0A,
    A7105_0B_GPIO1_PIN    = 0x0B,
    A7105_0C_GPIO2_PIN    = 0x0C,
    A7105_0D_CLOCK        = 0x0D,
    A7105_0E_DATA_RATE    = 0x0E,
    A7105_0F_PLL_I        = 0x0F,
    A7105_10_PLL_II       = 0x10,
    A7105_11_PLL_III      = 0x11,
    A7105_12_PLL_IV       = 0x12,
    A7105_13_PLL_V        = 0x13,
    A7105_14_TX_I         = 0x14,
    A7105_15_TX_II        = 0x15,
    A7105_16_DELAY_I      = 0x16,
    A7105_17_DELAY_II     = 0x17,
    A7105_18_RX           = 0x18,
    A7105_19_RX_GAIN_I    = 0x19,
    A7105_1A_RX_GAIN_II   = 0x1A,
    A7105_1B_RX_GAIN_III  = 0x1B,
    A7105_1C_RX_GAIN_IV   = 0x1C,
    A7105_1D_RSSI_THOLD   = 0x1D,
    A7105_1E_ADC          = 0x1E,
    A7105_1F_CODE_I       = 0x1F,
    A7105_20_CODE_II      = 0x20,
    A7105_21_CODE_III     = 0x21,
    A7105_22_IF_CALIB_I   = 0x22,
    A7105_23_IF_CALIB_II  = 0x23,
    A7105_24_VCO_CURCAL   = 0x24,
    A7105_25_VCO_SBCAL_I  = 0x25,
    A7105_26_VCO_SBCAL_II = 0x26,
    A7105_27_BATTERY_DET  = 0x27,
    A7105_28_TX_TEST      = 0x28,
    A7105_29_RX_DEM_TEST_I  = 0x29,
    A7105_2A_RX_DEM_TEST_II = 0x2A,
    A7105_2B_CPC          = 0x2B,
    A7105_2C_XTAL_TEST    = 0x2C,
    A7105_2D_PLL_TEST     = 0x2D,
    A7105_2E_VCO_TEST_I   = 0x2E,
    A7105_2F_VCO_TEST_II  = 0x2F,
    A7105_30_IFAT         = 0x30,
    A7105_31_RSCALE       = 0x31,
    A7105_32_FILTER_TEST  = 0x32,
};

//Logical mapping of speeds for use with the 0E Data Rate register.
enum A7105_DataRate {
  A7105_DATA_RATE_250Kbps = 0x01,
  A7105_DATA_RATE_125Kbps = 0x03,
  A7105_DATA_RATE_100Kbps = 0x04,
  A7105_DATA_RATE_50Kbps = 0x09,
  A7105_DATA_RATE_25Kbps = 0x13,
  A7105_DATA_RATE_10Kbps = 0x31,
  A7105_DATA_RATE_2Kbps = 0xF9,
};

//Logical mapping of transmit powers for use with the 28 TX Test register.
enum A7105_TxPower {
    A7105_TXPOWER_100uW,
    A7105_TXPOWER_300uW,
    A7105_TXPOWER_1mW,
    A7105_TXPOWER_3mW,
    A7105_TXPOWER_10mW,
    A7105_TXPOWER_30mW,
    A7105_TXPOWER_100mW,
    A7105_TXPOWER_150mW,
    A7105_TXPOWER_LAST,
};

/*
  A7105 Context structure, this is passed to basically all functions in this
  library to keep track of the radio state (pins, etc). It's opaque to the 
  calling code.
*/
struct A7105
{
  int _CS_PIN; //chip select pin (arduino number) so we can have multipe radios per microcontroller
  int _STATE; //Uses the values of the A7105 Strobe states for tracking interrupts and such 
  int _INTERRUPT_PIN;  //The pin mapped to GIO2 that get's interrupts on TX/RX (used for tracking data being available)
                       //-1 if no interrupt pin specified 
  int _USE_CRC; //0/1 if CRC is disabled/enabled on the radio
  int _USE_FEC; //0/1 if FEC is disabled/enabled on the radio
  byte _DATA_RATE; //Remember our current data rate setting
};

void mm_debug(struct A7105* radio);

//Basic Hardware Functions (basically hardware calls)

/*
void A7105_Initialize:
  * radio: this should just point to an existing radio struct (no special values needed)
  * chip_select_pin: the Arduino pin used as the SPI chip select 
  * reset: Perform a reset command on the radio before initializing (default: true). This is a good idea unless you
           know *exactly* what you're doing.

  Side Effects/Notes:
    * This function calls pinMode() for the chip_select_pin.

  This function initializes the A7105 structure, sets up the chip-select pin, 
  and configures the A7105 radio to use the GPIO1/2 pins for MISO and WTR functions
  respectively. This function does *not* guarantee correct connections or the
  required radio calibration and set-up before you can start pushing data around. 
  Check out A7105_Easy_Setup_Radio() for a function that handles all that.
*/
void A7105_Initialize(struct A7105* radio, int chip_select_pin);
void A7105_Initialize(struct A7105* radio, int chip_select_pin, int reset);

/*
  void A7105_WriteReg:
    * radio: A A7105 structure that has been previously passed to A7105_Initialize (i.e. it's configured)
    * addr: The register to write. This should be one of the enum's at the top of this file 
    * value: The byte (or uint32_t) value to write to the register

  This function writes values to the A7105 registers using the transactional SPI library 
  from the Arduino source.
*/
void A7105_WriteReg(struct A7105* radio, byte addr, byte value);
void A7105_WriteReg(struct A7105* radio, byte addr, uint32_t value);

/*
A7105_Status_Code A7105_WriteData:
  * radio: Pointer to a valid A7105 structure for state tracking. This 
           should be a radio that was either previously initialized with
           A7105_Easy_Setup_Radio() or another function that set up and 
           calibrated all the registers for the radio.
  * data:  A valid byte array that is at least 'length' bytes long
  * length: The length of the packet to send. This must be a multiple of            
            8 between 1 and 64 (1 is not a multiple of 8 but is valid).

  Side-Effects/Notes: 
    This function fills the FIFO register of the radio and tells it to transmit.
    However, when this function returns, the data has not necessarily been 
    fully transmitted yet (depending on data rate). We set up GIO2 as a
    WTR pin in A7105_Easy_Setup_Radio() so you can wait until that goes 
    low to be sure if you use that function.

  Returns:
    * A7105_STATUS_OK: If the packet was set up for transmission successfully and sending was started.
    * A7105_INVALID_FIFO_LENGTH: If the length specified was not a legitimate value (multiple of 8 in 1-64)
*/
A7105_Status_Code A7105_WriteData(struct A7105* radio, byte *dpbuffer, byte len);

/*
  void A7105_ReadReg:
    * radio: A A7105 structure that has been previously passed to A7105_Initialize (i.e. it's configured)
    * addr: The register to read. This should be one of the enum's at the top of this file 

  This function reads values to the A7105 registers using the transactional SPI library 
  from the Arduino source. We currently only implement reading one byte right now. We might 
  need a uint32_t version in the future...

  Returns:  The contents of the register.
*/
byte A7105_ReadReg(struct A7105* radio, byte addr);

/*
A7105_Status_Code A7105_ReadData:
  * radio: Pointer to a valid A7105 structure for state tracking. This 
           should be a radio that was either previously initialized with
           A7105_Easy_Setup_Radio() or another function that set up and 
           calibrated all the registers for the radio.
  * dpbuffer: A byte array of *at least* 'len' length.
  * len: the number of bytes to read. The maximum value for this is 64.

  Side-Effects/Notes:
    This function will try to read data always if no interrupt pin was specified. If you aren't using interrupts,
    make sure you wait long enough to have guaranteed delivery.

  Returns:
    * A7105_NO_DATA: If an interrupt wtr pin was specified for the radio and data was detected via that interrupt pin.
    * A7105_INVALID_FIFO_LENGTH: If 'len' was not a valid size to read (<=64 bytes).
    * A7105_STATUS_OK: If there was data avaiable and waiting *OR*
                       if no interrupt pin was set, it will just take
                       whatever is sitting in the radio FIFO register.
    * A7105_RX_DATA_INTEGRITY_ERROR: If CRC/FEC or both are enabled on the radio
                                     and we detect a fault with either, this is 
                                     returned. It means there *was* data, but it 
                                     has been corrupted.  The data is *still* read
                                     into 'dpbuffer' but do *NOT* trust it's integrity.
*/
A7105_Status_Code A7105_ReadData(struct A7105* radio, byte *dpbuffer, byte len);

/*
A7105_Status_Code A7105_CheckTXFinished:
  * radio: Pointer to a valid A7105 structure for state tracking. This 
           should be a radio that was either previously initialized with
           A7105_Easy_Setup_Radio() or another function that set up and 
           calibrated all the registers for the radio.

  This function checks if the TX data previously sent has finished transmission.
  This checks the MODE (00h) register of the radio and doesn't depend on 
  interrupts to work.

  Returns:
    A7105_STATUS_OK: If the radio TRX circuitry is idle (i.e. done sending)
    A7105_BUSY: If the radio is still sending data
*/

A7105_Status_Code A7105_CheckTXFinished(struct A7105* radio);

/*
A7105_Status_Code A7105_CheckRXWaiting:
  * radio: Pointer to a valid A7105 structure for state tracking. This 
           should be a radio that was either previously initialized with
           A7105_Easy_Setup_Radio() or another function that set up and 
           calibrated all the registers for the radio.

  This function checks if there is RX data waiting in the radio FIFO buffer.
  NOTE: This only works if an interrupt pin was specified with A7105_Easy_Setup_Radio()
        to be used as connected to the GPIO2 pin of the A7105 that acts as a WTR pin.

  Returns:
    A7105_NO_DATA: If interrupt pin was configured, but no data is waiting to be read
    A7105_RX_DATA_WAITING: If interrupt pin was configured and data is waiting to be read
    A7105_NO_WTR_INTERRUPT_SET: If no interrupt pin was configured.
    A7105_RX_DATA_INTEGRITY_ERROR: If CRC/FEC or both are enabled on the radio
                                     and we detect a fault with either, this is 
                                     returned. It means there *was* data, but it 
                                     has been corrupted. 
 */
A7105_Status_Code A7105_CheckRXWaiting(struct A7105* radio);

/*
void A7105_Reset:
  * radio: A A7105 structure that has been previously passed to A7105_Initialize (i.e. it's configured)

  This function issues a reset command to the A7105 chip. This resets
  all radio registers to their datasheet default values and GPIO pins
  to their default functions.  Calibration is required after calling
  this function.
*/
void A7105_Reset(struct A7105* radio);

/*
void A7105_SetPower:
  * radio: A A7105 structure that has been previously passed to A7105_Initialize (i.e. it's configured)
  * power: One of the A7105_TxPower settings specified at the beginning of this file to use 
           as a transmission power setting.

  Side-Effects/Notes: Please make sure the radio power supply is 
                      beefy enough to support the setting you specify.
                        
*/
void A7105_SetPower(struct A7105* radio, A7105_TxPower power);

/*
void A7105_Strobe:
  * radio: A A7105 structure that has been previously passed to A7105_Initialize (i.e. it's configured)
  * state: One of the A7105_States to change the radio mode to. Please
           refer to the datasheet to select the proper state, no validation
           is done in this function.
*/
void A7105_Strobe(struct A7105* radio, enum A7105_State state);

//Nice Radio Interface (wraps some of the complexity so you don't have to care 
//about horrible A7105 details...)

/*
This function is a high-level setup to use radios to talk to each other.
It sets them up assuming they're XL7105 breakouts with a 16Mhz clock to 
talk on a specific channel at a specific power. Also, we set up 
the chips GIO1 pin to be the MISO for 4-wire SPI communication (since 
the original implementer didn't have a bi-directional logic level switcher
to translate the 3v3 to 5V for his arduino). Also GIO2 is set as a 
WTR pin (goes high during TX/RX activity so we can optionally use
interrupts to see where there is RX data waiting.

See the implementation for all the gritty details.

A7105_Status_Code A7105_Setup_Radio:
  * radio: Pointer to a A7105 structure for state tracking (no fields need be set)
  * cs_pin: The arduino pin to use as a chip_select for the radio.
            NOTE: pinMode() is called in this function for this pin.
  * wtr_pin: The arduino pin to use that is connected to the GPIO2 
             pin on the radio. The WTR function (see the datasheet) 
             makes this pin go high when the radio is active TX or 
             RX. Specify -1 to ignore. Otherwise, a pin interrupt 
             will be set for this pin and it will be specified as 
             input.
  * radio_id: 4-byte ID to use for communicating radios to recognize
              each-other's traffic. All radios that talk to each other
              should have the same 'radio_id'
  * data_rate: The data rate for communications. Slower = longer-range
  * channel: 0 - A8. The channel to use for communications. 
  * power: The power setting to use for TX. Higher is better but 
           make sure you have a big enough power supply for this.
  * use_CRC: 0 or 1 to disable/enable the use of CRC for transmitted packets.
             This is all done on the A7105 and affects transmission time.
  * use_FEC: 0 or 1 to disable/enable the use of Foward Error Correction 
             (that is 1 bit auto correction for every 4-bit nibble) transmitted.
             This is done on the A7105 chip and affects transmission time.

  Side Effects/Notes:
    Be sure to call this method from the setup() function since
    we do some pinMode() stuff in here and SPI.begin() calls.


  Returns:
    * A7105_STATUS_OK: If the radio is successfully initialized
    * A7105_INIT_ERROR: If we can't get any data back from the radio.
                        This usually means a SPI error since we just
                        read back the clock settings as a sanity check 
                        to make sure the radio isn't just a black hole.
    * A7105_CALIBRATION_ERROR: If the auto-calibration fails for the 
                               chip. A7105's have to be auto-calibrated
                               every time they're reset or powered on
                               and that is done in this function.
    * A7105_INVALID_CHANNEL: If the specified channel is outside
                             the allowable range of 0-A8 (given our
                             pre-set and datasheet recommended channel
                             steps).
*/
A7105_Status_Code A7105_Easy_Setup_Radio(struct A7105* radio, 
                                    int cs_pin, 
                                    int wtr_pin, 
                                    uint32_t radio_id, 
                                    A7105_DataRate data_rate,
                                    byte channel,
                                    A7105_TxPower power,
                                    int use_CRC,
                                    int use_FEC);
/*
A7105_Status_Code A7105_Easy_Listen_For_Packets:
  * radio: Pointer to a valid A7105 structure for state tracking. This 
           should be a radio that was either previously initialized with
           A7105_Easy_Setup_Radio() or another function that set up and 
           calibrated all the registers for the radio.
  * length: The expected length of packets to receive. This sets-up the 
            FIFO configuration of the radio so it will know when it is 
            done reading a packet.

  Returns:
    * A7105_INVALID_FIFO_LENGTH: If "length" is not a valid packet length (1-64 multiples of 8)
    * A7105_STATUS_OK: If the packet length was set and the radio strobed to the "RX" state.
*/
A7105_Status_Code A7105_Easy_Listen_For_Packets(struct A7105* radio,
                                                byte length);


/*
void _A7105_Pin_Interrupt_Callback():
  This is an internal function that is the registered callback with 
  the PinChangeInterrupt library that we use to detect FALLING edges
  on the WTR pin that signal the end of either a TX or an RX operation.
*/
void _A7105_Pin_Interrupt_Callback();
#endif
