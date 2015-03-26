#include <Arduino.h>
#include <stdint.h>
#include <SPI.h>
#include "a7105.h"



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
  A7105_WriteReg(radio,A7105_0B_GPIO1_PIN1,(byte)A7105_ENABLE_4WIRE);
  A7105_WriteReg(radio,A7105_0C_GPIO2_PIN_II,(byte)A7105_GPIO_WTR);

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


void A7105_WriteData(struct A7105* radio, byte *dpbuffer, byte len)
{

  //Reset the FIFO write pointer
  A7105_Strobe(radio,A7105_RST_WRPTR);

  digitalWrite(radio->_CS_PIN,LOW);
  SPI.beginTransaction(SPISettings(A7105_SPI_CLOCK, MSBFIRST, SPI_MODE0));  // gain control of SPI bus

  SPI.transfer(A7105_05_FIFO_DATA);
  for (int i = 0; i < len; i++)
    SPI.transfer(dpbuffer[i]); // send some data
   
  SPI.endTransaction();          // release the SPI bus
  digitalWrite(radio->_CS_PIN,HIGH);

  // set the channel
  //NOTE: double check this logic
  //A7105_WriteReg(radio, A7105_0F_PLL_I, channel);

  //Tell the A7105 to blast the data
  A7105_Strobe(radio,A7105_TX);


 /*   int i;
    CS_LO();
    SPI.transfer(A7105_RST_WRPTR);    //reset write FIFO PTR
    SPI.transfer(0x05); // FIFO DATA register - about to send data to put into FIFO
    for (i = 0; i < len; i++)
        SPI.transfer(dpbuffer[i]); // send some data
    CS_HI();

    // set the channel
    A7105_WriteReg(0x0F, channel);

    CS_LO();
    SPI.transfer(A7105_TX); // strobe command to actually transmit the daat
    CS_HI();*/
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

  SPI.endTransaction();          // release the SPI bus
  digitalWrite(radio->_CS_PIN,HIGH);
  return read_byte;
}
  


void A7105_ReadData(struct A7105* radio, byte *dpbuffer, byte len)
{
    //Reset the FIFO read pointer
    A7105_Strobe(radio, A7105_RST_RDPTR); 

    for(int i = 0; i < len; i++) {
        dpbuffer[i] = A7105_ReadReg(radio, A7105_05_FIFO_DATA);
    }
 

    /*A7105_Strobe(A7105_RST_RDPTR); //A7105_RST_RDPTR
    for(int i = 0; i < len; i++) {
        dpbuffer[i] = A7105_ReadReg(0x05);
    }
    
    return;*/
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

