#ifndef _A7105_H_
#define _A7105_H_

#include <stdint.h>

#define A7105_ENABLE_4WIRE 0x19
#define A7105_GPIO_WTR 0x01 //Code to set a GIO pin to do WTR activity (be high during transmit/receive and low otherwise)
#define A7105_SPI_CLOCK 500000 //Clock rate for SPI communications with A7105's

#define A7105_HIGHEST_CHANNEL 0xA8 //Defined in datasheet pg. 59

#define A7105_CALIBRATION_TIMEOUT 500 //milliseconds before we call autocalibrating a failure

//Status codes for returns from the various functions in this library
//See individual functions below for meanings
enum A7105_Status_Code{
  A7105_STATUS_OK,
  A7105_INIT_ERROR,
  A7105_CALIBRATION_ERROR,
  A7105_INVALID_CHANNEL,
  A7105_INVALID_FIFO_LENGTH,
  A7105_RX_DATA_WAITING,
  A7105_NO_DATA,
  A7105_NO_WTR_INTERRUPT_SET,
  A7105_BUSY,
};

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
  A7105_INT_IGNORE_ONE = 0,
  A7105_INT_NULL = 1,
  A7105_INT_TRIGGERED = 2
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

struct A7105
{
  int _CS_PIN; //chip select pin (arduino number) so we can have multipe radios per microcontroller
  int _STATE; //Uses the values of the A7105 Strobe states for tracking interrupts and such 
  int _INTERRUPT_PIN;  //The pin mapped to GIO2 that get's interrupts on TX/RX (used for tracking data being available)
                       //-1 if no interrupt pin specified 
};

//Basic Hardware Functions (basically hardware calls)

void A7105_Initialize(struct A7105* radio, int chip_select_pin);
void A7105_Initialize(struct A7105* radio, int chip_select_pin, int reset);
void A7105_WriteReg(struct A7105* radio, byte addr, byte value);
void A7105_WriteReg(struct A7105* radio, byte addr, uint32_t value);

/*
A7105_Status_Code A7105_Easy_Send_Packet:
  * radio: Pointer to a valid A7105 structure for state tracking. This 
           should be a radio that was either previously initialized with
           A7105_Easy_Setup_Radio() or another function that set up and 
           calibrated all the registers for the radio.
  * data:  A valid byte array that is at least 'length' bytes long
  * length: The length of the packet to send. This must be a multiple of            8 between 1 and 64 (1 is not a multiple of 8 but is valid).

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
byte A7105_ReadReg(struct A7105* radio, byte addr);

/*
NOTE: This function will try to read data always if no interrupt pin was specified.

  Returns:
    * A7105_NO_DATA: If an interrupt wtr pin was specified for the radio and data was detected via that interrupt pin.
    * A8105_STATUS_OK: If there was data avaiable and waiting *OR*
                       if no interrupt pin was set, it will just take
                       whatever is sitting in the radio FIFO register.
*/
A7105_Status_Code A7105_ReadData(struct A7105* radio, byte *dpbuffer, byte len);
A7105_Status_Code A7105_CheckRXWaiting(struct A7105* radio);
void A7105_Reset(struct A7105* radio);
void A7105_SetPower(struct A7105* radio, A7105_TxPower power);
void A7105_Strobe(struct A7105* radio, enum A7105_State);

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

  Side Effects/Notes:
    The A7105 does not obey a high-Z state on the GPIO1 line we're 
    using for MISO so if you are using multiple radios
    on a single bus, be sure to implement tri-state buffers on their 
    GPIO1 lines (or use schottkey diodes and a pull-down resistor 
    like I did) and initalize all radios before using any of them 
    to avoid interfering with the SPI bus.

    Also, be sure to call this method from the setup() function since
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
                                    A7105_TxPower power);

A7105_Status_Code A7105_Easy_Listen_For_Packets(struct A7105* radio,
                                                byte length);


void _A7105_Pin_Interrupt_Callback();
#endif
