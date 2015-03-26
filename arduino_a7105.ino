#include "stdint.h"
#include <SPI.h>
#include <a7105.h> 

/*
This script is a test harness for the 
a7105 library (just the physical bits)

It sets up radios on a specific channel
and bounces data between them using a single
arduino (pro mini in the original case).


TODO: Set up a second radio and try to 
start bouncing data back and forth (no interrupts yet).

Then, after we successfully pass data, add 
an interrupt to notify us of pending data to read.

After everything works, we can start to library-ify the
results into the mesh library.

*/


//Pin used to activate recoding on my logic analyzer...
#define RECORD 4

#define RADIO1_SELECT_PIN 9
#define RADIO2_SELECT_PIN 8

#define RADIO_IDS 0xdb042679 //taken from devation code

#define CALIBRATION_TIMEOUT 500 //milliseconds before we call autocalibrating a failure

A7105 radio1;
A7105 radio2;


int setup_radio(struct A7105* radio, int cs_pin, uint32_t radio_id, A7105_DataRate data_rate, byte channel, A7105_TxPower power)
{

  /*
    Implementor note: The A7105 does not obey a high-Z state on the
    GPIO1 line we're using for MISO so if you are using multiple radios
    on a single bus, be sure to implement tri-state buffers on their GPIO1 lines (or use schottkey diodes and a pull-down resistor like I did) and initalize all radios before using any of them to avoid interfering with the SPI bus.
  */

  //Initialize
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
  A7105_WriteReg(radio,A7105_03_FIFOI, (byte)0x0f);

  //Set the clock to be the crystal on the xl7501 breakout
  A7105_WriteReg(radio,A7105_0D_CLOCK, (byte)0x05);

  //Test proper connectivity of the chip by checking out the register we just wrote
  if (A7105_ReadReg(radio,A7105_0D_CLOCK) != 0x05)
  {
    Serial.println("Could not read clock register...");
    //TODO: Include error reason upward
    return 0;
  }

  //Set the datarate 
  A7105_WriteReg(radio,A7105_0E_DATA_RATE, (byte)data_rate);

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

  //This register controls CRC and filtering options. It sets:
  //  * Data whitening (crypto) off
  //  * FEC Select off (Foward Error Correction)
  //  * Disables CRC Select
  //  * Set's ID code length to 4 bytes
  //  * Set preamble length to 4 bytes (alternating 0101010 or 1010101 to identify packets)
  //  TODO: Experiment with enabling CRC since we are more concerned
  //        with correctness than latency...
  A7105_WriteReg(radio,A7105_1F_CODE_I, (byte)0x07);

  //This register sets more coding options:
  //  * ID code error tolerance of 1-bit (recommended value)
  //  * Preamble pattern detection length (16-bits, recommended for our 2-125KHz)
  A7105_WriteReg(radio,A7105_20_CODE_II, (byte)0x17);

  //RX Demodulator stuff. This register sets:
  //  * Average and hold mode for the DC level (not sure what that is)
  //    NOTE: This might have something to do with filtering by ID...
  A7105_WriteReg(radio,A7105_29_RX_DEM_TEST_I, (byte)0x47);

  //MM
  //Set the channel (need to be here so the VDO calibration works later)
  A7105_WriteReg(radio,A7105_0F_PLL_I, channel);

  //END-MM

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
  while(millis() - ms < CALIBRATION_TIMEOUT)
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
    //TODO: communicate error upwards
    return 0;
  }

  //check for failure code of IF Filter calibration (0x10 bit high)
  if ((A7105_ReadReg(radio,A7105_22_IF_CALIB_I) & 0x10) == 0x10)
  {
    //TODO: communicate error upwards
    return 0;
  }

  //check for failure code of VCO Current calibration (0x10 bit high)
  if ((A7105_ReadReg(radio,A7105_24_VCO_CURCAL) & 0x10) == 0x10)
  {
    //TODO: communicate error upwards
    return 0;
  }

  //check for failure code of VCO Band calibration (0x08 bit high)
  if ((A7105_ReadReg(radio,A7105_25_VCO_SBCAL_I) & 0x80) == 0x80)
  {
    //TODO: communicate error upwards
    return 0;
  }

  //Set the transmit power
  A7105_SetPower(radio,power);

  //Put the radio back in standby mode   
  A7105_Strobe(radio,A7105_STANDBY);

  //If we make it here, we're calling it a success
  return 1;

}

// the setup function runs once when you press reset or power the board
void setup() {
  Serial.begin(9600);

  Serial.println("Starting...");

  //pinMode(RADIO_SELECT_PIN, OUTPUT);
  
  //TODO: Can we nuke this safely?
  pinMode(MOSI,OUTPUT);
  pinMode(MISO,INPUT);
  pinMode(SCK,OUTPUT);
  pinMode(RECORD,OUTPUT);

  digitalWrite(RECORD,LOW);

  //delay(2000);


  //HACK: Setting TXPower to 1mW instead of 150 since it was resetting the shit out of the arduino...  
  int success = setup_radio(&radio1,RADIO1_SELECT_PIN,RADIO_IDS, A7105_DATA_RATE_100Kbps,0,A7105_TXPOWER_100mW);
  int success2 = setup_radio(&radio2,RADIO2_SELECT_PIN,RADIO_IDS, A7105_DATA_RATE_100Kbps,0,A7105_TXPOWER_100mW);
  if (success)
    Serial.println("Successfully initialized Radio 1");
  else
    Serial.println("Radio 1 initialization failed");

  if (success2)
    Serial.println("Successfully initialized Radio 2");
  else
    Serial.println("Radio 2 initialization failed");
}


void send_packet(struct A7105* radio, byte* data, byte length)
{
  //TODO: See if we should put this buffer someplace else...
  byte packet[64];
  memset(packet,0,64);

  for (int x = 0;x<length;x++)
  {
    packet[x] = data[x];
  }

  //Set the length of the FIFO data to send
  //NOTE: This needs to be a multiple of 8
  //HACK: setting to 64 for testing
  A7105_WriteReg(radio, A7105_03_FIFOI, (byte)15);

  //Push the data to the radio's FIFO buffer
  //(the radio will return to standby mode automagically)
  digitalWrite(RECORD,HIGH);
  A7105_WriteData(radio,packet,16);
  digitalWrite(RECORD,LOW);

}

void send_uint32_packet(struct A7105* radio, uint32_t val)
{
  byte data[4];
  memset(data,0,4);
  data[0] = (val >> 24) & 0xff;
  data[1] = (val >> 16) & 0xff;
  data[2] = (val >> 8) & 0xff;
  data[3] = (val & 0xff);
  send_packet(radio,data,4);
}

void recv_packet(struct A7105* radio, byte* data, byte length)
{

  //TODO: Stateful checking to see if there is actually a packet waiting

  A7105_ReadData(radio,data,length);
}

void recv_uint32_packet(struct A7105* radio, uint32_t* val)
{
  byte data[4];
  recv_packet(radio,data,4);
  *val = ((uint32_t)(data[0]) << 24) |
        ((uint32_t)(data[1]) << 16) |
        ((uint32_t)(data[2]) << 8) |
        (uint32_t)(data[3]);
        
}


// the loop function runs over and over again forever
void loop() {

  //Put radio2 in RX mode so it will hear the packet we're sending with radio1
  A7105_Strobe(&radio2,A7105_RX);

  //Write Data from radio 1 to radio 2 (just write the 4-byte long of the millis() counter)
  uint32_t val = millis();
  send_uint32_packet(&radio1,val);
  Serial.print("Sent from radio1: ");
  Serial.println(val);


  //wait for the packet to transmit (since we don't have interrupts working yet)
  delay(100);

  //Read the packet from radio2 (hopefully it's there)
  val = 0;
  recv_uint32_packet(&radio2,&val);
  Serial.print("Radio2's fifo had ");
  Serial.println(val);

  Serial.println("done...");
  delay(5000);
  //Serial.print("RADIO_CLOCK register: ");
  //Serial.print(read_reg(RADIO_CLOCK),HEX);
  //Serial.println("");
  //digitalWrite(13, HIGH);   // turn the LED on (HIGH is the voltage level)
  //delay(100000000);              // wait for a second
  //digitalWrite(13, LOW);    // turn the LED off by making the voltage LOW
  //delay(100000000);              // wait for a second
}
