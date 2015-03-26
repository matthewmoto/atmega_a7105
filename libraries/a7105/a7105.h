#ifndef _A7105_H_
#define _A7105_H_

#include <stdint.h>

#define A7105_ENABLE_4WIRE 0x19
#define A7105_GPIO_WTR 0x01 //Code to set a GIO pin to do WTR activity (be high during transmit/receive and low otherwise)
#define A7105_SPI_CLOCK 500000 //Clock rate for SPI communications with A7105's

#define HIGHEST_CHANNEL 0xA8 //Defined in datasheet pg. 59

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

enum {
    A7105_00_MODE         = 0x00,
    A7105_01_MODE_CONTROL = 0x01,
    A7105_02_CALC         = 0x02,
    A7105_03_FIFOI        = 0x03,
    A7105_04_FIFOII       = 0x04,
    A7105_05_FIFO_DATA    = 0x05,
    A7105_06_ID_DATA      = 0x06,
    A7105_07_RC_OSC_I     = 0x07,
    A7105_08_RC_OSC_II    = 0x08,
    A7105_09_RC_OSC_III   = 0x09,
    A7105_0A_CK0_PIN      = 0x0A,
    A7105_0B_GPIO1_PIN1   = 0x0B,
    A7105_0C_GPIO2_PIN_II = 0x0C,
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

enum A7105_DataRate {
  A7105_DATA_RATE_250Kbps = 0x01,
  A7105_DATA_RATE_125Kbps = 0x03,
  A7105_DATA_RATE_100Kbps = 0x04,
  A7105_DATA_RATE_50Kbps = 0x09,
  A7105_DATA_RATE_25Kbps = 0x13,
  A7105_DATA_RATE_10Kbps = 0x31,
  A7105_DATA_RATE_2Kbps = 0xF9,
};

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

enum A7105_ProtoCmds {
    A7105_PROTOCMD_INIT,
    A7105_PROTOCMD_DEINIT,
    A7105_PROTOCMD_BIND,
    A7105_PROTOCMD_CHECK_AUTOBIND,
    A7105_PROTOCMD_NUMCHAN,
    A7105_PROTOCMD_DEFAULT_NUMCHAN,
    A7105_PROTOCMD_CURRENT_ID,
    A7105_PROTOCMD_SET_TXPOWER,
    A7105_PROTOCMD_GETOPTIONS,
    A7105_PROTOCMD_SETOPTIONS,
    A7105_PROTOCMD_TELEMETRYSTATE,
};

void PROTOCOL_SetBindState(uint32_t msec);

#define A7105_0F_CHANNEL A7105_0F_PLL_I

struct A7105
{
  int _CS_PIN; //chip select pin (arduino number) so we can have multipe radios per microcontroller
  int _STATE; //Uses the values of the A7105 Strobe states for tracking interrupts and such 
  int _INTERRUPT_PIN;  //The pin mapped to GIO2 that get's interrupts on TX/RX (used for tracking data being available)
                       //-1 if no interrupt pin specified 
};

void A7105_Initialize(struct A7105* radio, int chip_select_pin);
void A7105_Initialize(struct A7105* radio, int chip_select_pin, int reset);
void A7105_WriteReg(struct A7105* radio, byte addr, byte value);
void A7105_WriteReg(struct A7105* radio, byte addr, uint32_t value);
int A7105_WriteData(struct A7105* radio, byte *dpbuffer, byte len);
byte A7105_ReadReg(struct A7105* radio, byte addr);
void A7105_ReadData(struct A7105* radio, byte *dpbuffer, byte len);
void A7105_Reset(struct A7105* radio);
void A7105_SetPower(struct A7105* radio, A7105_TxPower power);
void A7105_Strobe(struct A7105* radio, enum A7105_State);


#endif
