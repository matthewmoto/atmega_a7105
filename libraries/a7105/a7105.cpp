/*
Copyright (C) 2015 Matthew Meno

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

#include <stdint.h>
#include <SPI.h>
#include "a7105.h"
#include "PinChangeInt.h"

//HACK: A 328P only has 19 pins, but we oversize this array to try to keep things agnostic to the different AVR chips
volatile byte _A7105_INTERRUPT_COUNTS[64]; //Interrupt tracking for radio RX notifications (0 = ignore for one interrupt, 1 = no interrupts, 2 = interrupt detected)

void A7105_Initialize(struct A7105* radio, int chip_select_pin)
{
  A7105_Initialize(radio,chip_select_pin,1);
}

/*
  NOTE: This function must be called from setup()
        since we're setting pin modes (not sure if this is
        too much a side-effect to have down here).
*/
void A7105_Initialize(struct A7105* radio, int chip_select_pin, int reset)
{
  SPI.begin();
  
  pinMode(chip_select_pin, OUTPUT);

  //Initialize the chip select pin (HACK: Assume the pin is in OUTPUT mode, do we need to worry about doing this outside of setup()?)
  radio->_CS_PIN = chip_select_pin;
  digitalWrite(radio->_CS_PIN,HIGH);

  //Reset the radio, set-up 4-wire SPI communication and use GPIO2 as a WTR pin (high when transmitting/receiving) for interrupts
  A7105_Reset(radio);
  A7105_WriteReg(radio,A7105_0B_GPIO1_PIN,(byte)A7105_ENABLE_4WIRE);
  A7105_WriteReg(radio,A7105_0C_GPIO2_PIN,(byte)A7105_GPIO_WTR);

}

void A7105_WriteReg(struct A7105* radio, byte address, byte data)
{
  digitalWrite(radio->_CS_PIN,LOW);
  //NOTE: The A7105 only speaks MSBFIRST,SPI Mode 0, the variable clock rate is for debugging
  SPI.beginTransaction(SPISettings(A7105_SPI_CLOCK, MSBFIRST, SPI_MODE0));  // gain control of SPI bus

  SPI.transfer(address);
  SPI.transfer(data);

  SPI.endTransaction();          // release the SPI bus
  digitalWrite(radio->_CS_PIN,HIGH);
}

void A7105_WriteReg(struct A7105* radio, byte addr, uint32_t data)
{
  digitalWrite(radio->_CS_PIN,LOW);
  //NOTE: The A7105 only speaks MSBFIRST,SPI Mode 0, the variable clock rate is for debugging
  SPI.beginTransaction(SPISettings(A7105_SPI_CLOCK, MSBFIRST, SPI_MODE0));  // gain control of SPI bus

  SPI.transfer(addr);
  SPI.transfer((data >> 24) & 0xFF);
  SPI.transfer((data >> 16) & 0xFF);
  SPI.transfer((data >> 8) & 0xFF);
  SPI.transfer((data >> 0) & 0xFF);
  
  SPI.endTransaction();          // release the SPI bus
  digitalWrite(radio->_CS_PIN,HIGH);
}

/*
  Returns success (1) or (0).

  NOTE: currently failure only happens on improper length data (must be multiple of 8 between 1 and 64)
*/
A7105_Status_Code A7105_WriteData(struct A7105* radio, byte *dpbuffer, byte len)
{

  //Check to make sure len is a multiple of 8 between 1 and 64
  if (len != 1  &&
      len != 8  &&
      len != 16 &&
      len != 32 &&
      len != 64)
  {
    return A7105_INVALID_FIFO_LENGTH;
  }

  //Ensure we're not in RX mode (go back to standby)
  A7105_Strobe(radio,A7105_STANDBY);

  //Make sure we aren't in the middle of a TX right now (only works
  //if a WTR pin was set, otherwise just hope for the best)
  if (radio->_INTERRUPT_PIN > 0)
  {
    if ( _A7105_INTERRUPT_COUNTS[radio->_INTERRUPT_PIN] == A7105_INT_IGNORE_ONE)
    {
      return A7105_BUSY;
    }

    //If we have interrupts enabled (and arent busy), ignore the next one
    //which will be set when we finish transmitting
    _A7105_INTERRUPT_COUNTS[radio->_INTERRUPT_PIN] = A7105_INT_IGNORE_ONE;
  }

  //Set the length of the FIFO data to send (len - 1 since it's an end-pointer)
  A7105_WriteReg(radio, A7105_03_FIFO_I, (byte)(len-1));

  //Reset the FIFO write pointer
  A7105_Strobe(radio,A7105_RST_WRPTR);

  digitalWrite(radio->_CS_PIN,LOW);
  SPI.beginTransaction(SPISettings(A7105_SPI_CLOCK, MSBFIRST, SPI_MODE0));  // gain control of SPI bus

  SPI.transfer(A7105_05_FIFO_DATA);
  for (int i = 0; i < len; i++)
    SPI.transfer(dpbuffer[i]); // send some data
   
  SPI.endTransaction();          // release the SPI bus
  digitalWrite(radio->_CS_PIN,HIGH);

  //Tell the A7105 to blast the data
  A7105_Strobe(radio,A7105_TX);
 
  return A7105_STATUS_OK;
}

byte A7105_ReadReg(struct A7105* radio, byte addr)
{
//digitalWrite(RADIO_SCK,LOW);
  byte command = addr | 0x40;
  digitalWrite(radio->_CS_PIN,LOW);
  SPI.beginTransaction(SPISettings(A7105_SPI_CLOCK, MSBFIRST, SPI_MODE0));  // gain control of SPI bus

  //shiftOut(RADIO_MOSI,RADIO_SCK, MSBFIRST, command);
  //byte read_byte = shiftIn(RADIO_MISO,RADIO_SCK,MSBFIRST);
  SPI.transfer(command);
  byte read_byte = SPI.transfer(0x00);

  /*
  Serial.print("Read: ");
  Serial.print(command,HEX);
  Serial.print(" : ");
  Serial.println(read_byte,HEX);
  */
  SPI.endTransaction();          // release the SPI bus
  digitalWrite(radio->_CS_PIN,HIGH);
  return read_byte;
}
  


A7105_Status_Code A7105_ReadData(struct A7105* radio, byte *dpbuffer, byte len)
{
    //ensure len is a valid value
    if (len > A7105_MAX_FIFO_SIZE)
    {
      return A7105_INVALID_FIFO_LENGTH;
    }


    //If the radio specified a WTR pin interrupt, use that data
    //to determine if there is any data waiting
    if (radio->_INTERRUPT_PIN > 0 &&
        _A7105_INTERRUPT_COUNTS[radio->_INTERRUPT_PIN] != A7105_INT_TRIGGERED)
    {
      return A7105_NO_DATA;
    }
    
    else
    {
      //clear the interrupt counter since we're reading
      _A7105_INTERRUPT_COUNTS[radio->_INTERRUPT_PIN] = A7105_INT_NULL;
    }

    //Reset the FIFO read pointer
    A7105_Strobe(radio, A7105_RST_RDPTR); 

    for(int i = 0; i < len; i++) {
        dpbuffer[i] = A7105_ReadReg(radio, A7105_05_FIFO_DATA);
    }

    //check for CRC/FEC (if it's enabled)
    //NOTE: We read the data either way since the user
    //can decide if they have use for the data
    if (radio->_USE_CRC || radio->_USE_FEC)
    {
      byte crc_fec_check = A7105_ReadReg(radio, A7105_00_MODE);
      if ((crc_fec_check & CRC_CHECK_MASK) ||
          (crc_fec_check & FEC_CHECK_MASK))
      {
        return A7105_RX_DATA_INTEGRITY_ERROR;
      }
    }

    return A7105_STATUS_OK;
}

A7105_Status_Code A7105_CheckTXFinished(struct A7105* radio)
{

  //Serial.println("DEBUG: CheckTxFinished");
  if (A7105_ReadReg(radio, A7105_00_MODE) & 0x01)
    return  A7105_BUSY;

  return A7105_STATUS_OK;
}

A7105_Status_Code A7105_CheckRXWaiting(struct A7105* radio)
{

  //If the radio specified a WTR pin interrupt, use that data
  //to determine if there is any data waiting
  if (radio->_INTERRUPT_PIN > 0)
  {
    if (_A7105_INTERRUPT_COUNTS[radio->_INTERRUPT_PIN] != A7105_INT_TRIGGERED)
    {
      return A7105_NO_DATA;
    }

    else
    {
      //check for CRC/FEC only if it's enabled (otherwise we're just wasting cycles)
      if (radio->_USE_CRC || radio->_USE_FEC)
      {
        //Serial.println("DEBUG: CheckRXWaiting");
        byte crc_fec_check = A7105_ReadReg(radio, A7105_00_MODE);
        if ((crc_fec_check & CRC_CHECK_MASK) ||
            (crc_fec_check & FEC_CHECK_MASK))
        {
          //Serial.println("DEBUG: Data integrity error");
          return A7105_RX_DATA_INTEGRITY_ERROR;
        }
      }


      return  A7105_RX_DATA_WAITING;
    }
  }
  return A7105_NO_WTR_INTERRUPT_SET;
}

void A7105_Reset(struct A7105* radio)
{
    A7105_WriteReg(radio, 0x00, (byte) 0x00);
    //NOTE: Maybe we should just delay(1) here?
    delayMicroseconds(1000);
}


void A7105_SetPower(struct A7105* radio, A7105_TxPower power)
{
    /*
    Power amp is ~+16dBm so:
    TXPOWER_100uW  = -23dBm == PAC=0 TBG=0
    TXPOWER_300uW  = -20dBm == PAC=0 TBG=1
    TXPOWER_1mW    = -16dBm == PAC=0 TBG=2
    TXPOWER_3mW    = -11dBm == PAC=0 TBG=4
    TXPOWER_10mW   = -6dBm  == PAC=1 TBG=5
    TXPOWER_30mW   = 0dBm   == PAC=2 TBG=7
    TXPOWER_100mW  = 1dBm   == PAC=3 TBG=7
    TXPOWER_150mW  = 1dBm   == PAC=3 TBG=7
    */
    byte pac, tbg;
    switch(power) {
        case 0: pac = 0; tbg = 0; break;
        case 1: pac = 0; tbg = 1; break;
        case 2: pac = 0; tbg = 2; break;
        case 3: pac = 0; tbg = 4; break;
        case 4: pac = 1; tbg = 5; break;
        case 5: pac = 2; tbg = 7; break;
        case 6: pac = 3; tbg = 7; break;
        case 7: pac = 3; tbg = 7; break;
        default: pac = 0; tbg = 0; break;
    };
    A7105_WriteReg(radio, A7105_28_TX_TEST,(byte)( (pac << 3) | tbg));
}

void A7105_Strobe(struct A7105* radio, enum A7105_State state)
{
  digitalWrite(radio->_CS_PIN,LOW);
  //NOTE: The A7105 only speaks MSBFIRST,SPI Mode 0, the variable clock rate is for debugging
  SPI.beginTransaction(SPISettings(A7105_SPI_CLOCK, MSBFIRST, SPI_MODE0));  // gain control of SPI bus

  SPI.transfer(state);

  SPI.endTransaction();          // release the SPI bus
  digitalWrite(radio->_CS_PIN,HIGH);
}


A7105_Status_Code A7105_Easy_Setup_Radio(struct A7105* radio, 
                                         int cs_pin, 
                                         int wtr_pin, 
                                         uint32_t radio_id, 
                                         A7105_DataRate data_rate, 
                                         byte channel, 
                                         A7105_TxPower power,
                                         int use_CRC,
                                         int use_FEC)
{

  /*
    Implementor note: The A7105 does not obey a high-Z state on the
    GPIO1 line we're using for MISO so if you are using multiple radios
    on a single bus, be sure to implement tri-state buffers on their GPIO1 lines (or use schottkey diodes and a pull-down resistor like I did) an
d initalize all radios before using any of them to avoid interfering with the SPI bus.
  */
  
  //Initialize (this sets the CS pin option and 4-wire spi bus options
  //if it wasn't already, resets the radio, etc).
  A7105_Initialize(radio,cs_pin);
  
  //Set ID (for packet filtering)
  A7105_WriteReg(radio,A7105_06_ID_DATA,radio_id);
  
  //This set's a bunch of options for the chip:
  //  * Disable direct mode on the SDIO pin (that's just MOSI for us)
  //  * Enable auto RSSI measurement entering RX (???)
  //  * Enable auto IF offset for RX (???)
  //  * Disable CD filtering (CD = carrier detector I think)
  //  * Enable FIFO mode (instead of direct mode)
  //  * Enable ADC measurement (not sure what this actually does...)
  A7105_WriteReg(radio,A7105_01_MODE_CONTROL, (byte)0x63);
  
  //Set FIFO mode (instead of direct mode)
  A7105_WriteReg(radio,A7105_03_FIFO_I, (byte)0x0f);

  //Ensure we're using Easy FIFO mode (FPM, PSA both are zeroed)
  A7105_WriteReg(radio,A7105_04_FIFO_II, (byte)0x00);
  
  //Set the clock to be the crystal on the xl7501 breakout
  A7105_WriteReg(radio,A7105_0D_CLOCK, (byte)0x05);

  //Test proper connectivity of the chip by checking out the register we just wrote
  if (A7105_ReadReg(radio,A7105_0D_CLOCK) != 0x05)
  {
    return A7105_INIT_ERROR;
  }

  //Set the datarate 
  A7105_WriteReg(radio,A7105_0E_DATA_RATE, (byte)data_rate);
  radio->_DATA_RATE = data_rate;

  //This set's a frequency deviation setting.
  //There is a table on pg. 21, not sure exactly what this is for...
  //The value (0x0B, upper nibble is constant) corresponds with 186KHz
  A7105_WriteReg(radio,A7105_15_TX_II, (byte)0x2b);

  //This sets RX options to:
  //  * Disable frequency compensation
  //  * have non inverted output
  //  * Have 500KHz bandwidth
  //  * Use "up side band" for the frequency offet (set above with MODE_CONTROL)
  A7105_WriteReg(radio,A7105_18_RX, (byte)0x62);

  //This sets the amplifier gain for receving
  //to 24dB for the mixer and LNA (I think this means Low Noise Amplifcation)
  //NOTE: These are the highest setttings (and recommended in the data sheet)
  A7105_WriteReg(radio,A7105_19_RX_GAIN_I, (byte)0x80);

  //This is a reserved register and just specifies the values
  //from the datasheet, we *should* be able to remove this.
  A7105_WriteReg(radio,A7105_1C_RX_GAIN_IV, (byte)0x0A);

  //Store the CRC/FEC settings and calculate the register value to push below
  byte default_code = 0x07;
  radio->_USE_FEC = 0;
  radio->_USE_CRC = 0;
  if (use_CRC)
  {
    radio->_USE_CRC = 1;
    default_code |= CRC_ENABLE_MASK;
  }
  if (use_FEC)
  {
    radio->_USE_FEC = 1;
    default_code |= FEC_ENABLE_MASK;
  }
   //This register controls CRC and filtering options. It sets:
  //  * Data whitening (crypto) off
  //  * FEC Select off (Foward Error Correction)
  //  * Disables CRC Select
  //  * Set's ID code length to 4 bytes
  //  * Set preamble length to 4 bytes (alternating 0101010 or 1010101 to identify packets)
  //  TODO: Experiment with enabling CRC since we are more concerned
  //        with correctness than latency...
 A7105_WriteReg(radio,A7105_1F_CODE_I, default_code);

  //This register sets more coding options:
  //  * ID code error tolerance of 1-bit (recommended value)
  //  * Preamble pattern detection length (16-bits, recommended for our 2-125KHz)
  A7105_WriteReg(radio,A7105_20_CODE_II, (byte)0x17);

  //RX Demodulator stuff. This register sets:
  //  * Average and hold mode for the DC level (not sure what that is)
  //    NOTE: This might have something to do with filtering by ID...
  A7105_WriteReg(radio,A7105_29_RX_DEM_TEST_I, (byte)0x47);


  //Verify the channel is in the legit range
  if (channel > A7105_HIGHEST_CHANNEL)
  {
    return A7105_INVALID_CHANNEL;
  }
  //Set the channel (need to be here so the VDO calibration works later)
  A7105_WriteReg(radio,A7105_0F_PLL_I, channel);

  //Calibrate the radio (required after reset/power-on)
  //Check out pg. 63 of the data sheet

  //This set's up the chip for auto calibration
  A7105_WriteReg(radio,A7105_22_IF_CALIB_I,(byte)0x00);

  //Tell the radio to go into PLL mode
  A7105_Strobe(radio,A7105_PLL);

  //Turn on IF Filter, VCO current and VCO bank calibration
  A7105_WriteReg(radio,A7105_02_CALC, (byte)0x07);

  //Wait for the FBC part of the 02_CALC register to clear so we 
  //know calibration is done (if we time-out return a failure code)
  unsigned long ms = millis();
  int calibration_success = 0;
  while(millis() - ms < A7105_CALIBRATION_TIMEOUT)
  {
    //check if the FBC,VCO,VCC bits (lowest 3) are auto-cleared (signals the end of autocalibration)
    if ((A7105_ReadReg(radio,A7105_02_CALC) & 0x07) == 0x00)
    {
      calibration_success = 1;
      break;
    }
  }

  //check for timeout calibrating IF Filter
  if (!calibration_success)
  {
    return A7105_CALIBRATION_ERROR;
  }

  //check for failure code of IF Filter calibration (0x10 bit high)
  if ((A7105_ReadReg(radio,A7105_22_IF_CALIB_I) & 0x10) == 0x10)
  {
    return A7105_CALIBRATION_ERROR;
  }

  //check for failure code of VCO Current calibration (0x10 bit high)
  if ((A7105_ReadReg(radio,A7105_24_VCO_CURCAL) & 0x10) == 0x10)
  {
    return A7105_CALIBRATION_ERROR;
  }

  //check for failure code of VCO Band calibration (0x08 bit high)
  if ((A7105_ReadReg(radio,A7105_25_VCO_SBCAL_I) & 0x80) == 0x80)
  {
    return A7105_CALIBRATION_ERROR;
  }

  //Set the transmit power
  A7105_SetPower(radio,power);

  //Put the radio back in standby mode   
  A7105_Strobe(radio,A7105_STANDBY);

  //Set up the WTR pin change interrupt if it was specified
  if (wtr_pin > 0)  
  {
    _A7105_INTERRUPT_COUNTS[wtr_pin] = A7105_INT_NULL; 
    pinMode(wtr_pin,INPUT);
    attachPinChangeInterrupt(wtr_pin,_A7105_Pin_Interrupt_Callback,FALLING);
  }
  radio->_INTERRUPT_PIN = wtr_pin; //save the pin (or -1 value) for use elsewhere


  //If we make it here, we're calling it a success
  return A7105_STATUS_OK;
}

A7105_Status_Code A7105_Easy_Listen_For_Packets(struct A7105* radio,
                                                byte length)
{
  //Check to make sure len is a multiple of 8 between 1 and 64
  if (length != 1  &&
      length != 8  &&
      length != 16 &&
      length != 32 &&
      length != 64)
  {
    return A7105_INVALID_FIFO_LENGTH;
  }

  //DEBUG (not sure if this stanza is totally necessary yet)
  A7105_Strobe(radio,A7105_STANDBY);

  if (radio->_INTERRUPT_PIN > 0)
  {
    //clear the interrupt counter since we're reading
    _A7105_INTERRUPT_COUNTS[radio->_INTERRUPT_PIN] = A7105_INT_NULL;
  }
  A7105_Strobe(radio,A7105_RST_WRPTR);
  //DEBUG

  //Send the length of the packet we're expecting (len - 1 since it's an end-pointer)
  //TODO: Maybe we can make this optional so we don't have to do it for 
  //every packet?
  A7105_WriteReg(radio, A7105_03_FIFO_I, (byte)(length-1));

  //Put radio2 in RX mode so it will hear the packet we're sending with radio1
  A7105_Strobe(radio,A7105_RX);


  return A7105_STATUS_OK;
}



void A7105_Pause(struct A7105* radio)
{
  detachInterrupt(radio->_INTERRUPT_PIN); 
}

void A7105_Resume(struct A7105* radio)
{
  attachPinChangeInterrupt(radio->_INTERRUPT_PIN,_A7105_Pin_Interrupt_Callback,FALLING);
}

/*
  Internal use only, This function is the callback for the pin change
  interrupts of the WTR pins for the connected radios.
*/
void _A7105_Pin_Interrupt_Callback()
{

  //NOTE: We don't bother differentating TX vs RX here (both cause the WTR pin to go high during activity) because we set the interrupt counter to IGNORE_ONE before activating the TX circuity. That way it just get's incremented to NULL after it's done.

  if (_A7105_INTERRUPT_COUNTS[PCintPort::arduinoPin] < A7105_INT_TRIGGERED)
    _A7105_INTERRUPT_COUNTS[PCintPort::arduinoPin]++; 
}
