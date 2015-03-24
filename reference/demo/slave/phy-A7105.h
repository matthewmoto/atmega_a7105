    /* declare header file */
#include        "global.h"
/****************************************/
    /* declare const */
#define    positive             1
#define    negative             0

#define    cNumber_4bit         0x04
#define    cNumber_8bit         0x08

#define    cID_Code0            0x12
#define    cID_Code1            0x34
#define    cID_Code2            0x56
#define    cID_Code3            0x78

#define    cCommand_StandBy     0xa0
#define    cCommand_WPll        0xb0
#define    cCommand_RX          0xc0
#define    cCommand_TX          0xd0
#define    cCommand_RxReset     0xe0
#define    cCommand_TxReset     0xf0      

__flash unsigned char A7105_Default_Parameter[] =
{
	0x00, /* RESET register : not use on config */
	
	0x02, /* MODE register: FIFO mode */
	
	0x00, /* CALIBRATION register */
	
	cFIFOlength-1, /* FIFO1 register : packet length*/
	
	0xc0, /* FIFO2 register : FIFO pointer margin threshold 16/48bytes(TX/RX)*/
	
	0x00, /* FIFO register :not use on config */
	
	0x00, /* IDDATA register :not use on config */ 
	
	0x00, /* RCOSC1 register */
	0x00, /* RCOSC2 register */
	0x00, /* RCOSC3 register */
	
	0x00, /* CKO register : clkout enable,bit clock */
	
	0x01, /* GPIO1 register :WTR output,enable GPIO1 output */
	0x09, /* GPIO2 register :CD carrier detect,enable GPIO2 output */
	
	0x05, /* CLOCK register :
	                           clock generation disable,crystal oscillator(16MHZ) enable 
	                           system clock    = master clock divider by 2,master clock = crystal*2(DBL = 1)
	                           system clock    = 16Mhz
	                           */
	
	0x01, /* DATARATE register : 
	                            rate = system clock/(32*SDR[7:0]+1),SDR = 0000 0001;
	                            rate = 250kbps
	                            */
	0x04, /* PLL1 register: channel = 0x04,2400MHZ + 500k * channel = 2402MHZ */
	
	0x9E, /* PLL2 register : 
	                         DBL =1,double select crystal
	                         RRC[1:0] = 00,Fpfd = 32MHZ
	                         channel step frequency = 500khz,CHR = 15
	                         */
	0x4B, /* PLL3 register : 
	                         BIP = 75
	                         */
	                         
	0x00, /* PLL4 register : BFP[15:8] = 00 */
	0x00, /* PLL5 register :
	                         BFP[7:0]  = 00
	                         base frequency = Fpfd*(BIP +BFP/2^16) = 2400MHZ
	                         */
	                         
	0x16, /* TX1 register : modulation enable, frequency deviation power setting [110] */
	
	0x23, /* TX2 register :
	                        Fpfd = 32MHZ,PDV[1:0] = 01,SDR[7:0] = 0000 0001
	                        
	                        Tx rate = Fpfd / (32 * (PDV[1:0]+1) * (SDR[7:0]+1)) = 250kbps
	                        
	                        TX frequency deviation = Fpfd * 127 * 2^FDP[2:0] * (FD[4:0]+1) /2^24 = 62KHZ
	                        */
	
	0x14, /* DELAY1 register :  TDL = 10,PDL = 100, delay = 60us + 80us = 140us */
	0xF8, /* DELAY2 register :  crystal turn on delay 2.5ms,AGC delay 40us,RSSI measurement delay 10us */
	
	0x26, /* RX register : AFC manual,demodulator gain =x3,BPF =500khz, up side band */
	
	0x80, /* RXGAIN1 register : VGA manual,PGA gain 12db,Mixer gain 24db,LNA gain 24db */
	0x80, /* RXGAIN2 register : VGA calibrate upper limit 0x80 */
	0x00, /* RXGAIN3 register : VGA calibrate lower limit 0x80 */
	0x0E, /* RXGAIN4 register : VGC calibrate continues until ID code is received,Mixer current 0.6mA,LNA current 2mA */
	
	0x96, /* RSSI register : RSSI 255 */
	
        0xC3, /* ADC register */
        
        0x5f, /* CODE1 register : Manchest enable,FEC enable,CRC enable,ID length 4 bytes,Preamble length 4 bytes */
	0x12, /* CODE2 register : DC value hold at 16 bit after preamble detect,ID code 0 bit error */
	0x00, /* CODE3 register : data whitening setting */
	
	0x00, /* IFCAL1 register : calibration */
	0x00, /* IFCAL2 register : only read */
	
	0x00, /* VCOCCAL register : VCO current */
	0x00, /* VCOCAL1 register : VCO band */
	0x3A, /* VCOCAL2 register : REGA = 1.1V,VCO tuning voltage  =0.3V */
	
	0x00, /* BATTERY register : default */
	
	0x17, /* TXTEST register : default */
	
	0x47, /* RXDEM1 register : default */
	0x80, /* RXDEM2 register : default */
	
	0x01, /* CPC register : charge pump current 1.0mA */
	
	0x05, /* CRYSTAL register : default */
	
	0x45, /* PLLTEST register : default */
	
	0x18, /* VCOTEST1 register : default */
	0x00, /* VCOTEST2 register : default */
	
	0x01, /* IFAT register : default */
	0x0F, /* RSCALE register : default */
	0x00  /* FILTERTEST : default */
};


/* 跳频查询备用 */
__flash unsigned char RX_Channel_Table[] =
{
        0x02, /* 2401MHZ */
        0x0c, /* 2406MHZ */
        0x16, /* 2411MHZ */
        
        0x20, /* 2416MHZ */
        0x2a, /* 2421MHZ */
        0x34, /* 2426MHZ */
        
        0x3e, /* 2431MHZ */
        0x48, /* 2436MHZ */
        0x52, /* 2441MHZ */
        
        0x5c, /* 2446MHZ */
        0x66, /* 2451MHZ */
        0x70, /* 2456MHZ */
        
        0x7a, /* 2461MHZ */
        0x84, /* 2466MHZ */
        0x8e, /* 2471MHZ */
        
        0x98  /* 2476MHZ */
};

/* 跳频查询备用 */
__flash unsigned char TX_Channel_Table[] =
{
        0x03, /* 2401.5MHZ */
        0x0d, /* 2406.5MHZ */
        0x17, /* 2411.5MHZ */
        
        0x21, /* 2416.5MHZ */
        0x2b, /* 2421.5MHZ */
        0x35, /* 2426.5MHZ */
        
        0x3f, /* 2431.5MHZ */
        0x49, /* 2436.5MHZ */
        0x53, /* 2441.5MHZ */
        
        0x5d, /* 2446.5MHZ */
        0x67, /* 2451.5MHZ */
        0x71, /* 2456.5MHZ */
        
        0x7b, /* 2461.5MHZ */
        0x85, /* 2466.5MHZ */
        0x8f, /* 2471.5MHZ */
        
        0x99  /* 2476.5MHZ */
};
/****************************************/
     /*  declare i/o define */
#define    direction_Sck        ddrDbit6
#define    direction_Sdio       ddrDbit5
#define    direction_Scs        ddrDbit7

#define    direction_GPIO1_WTR  ddrDbit2

#define    direction_Rxen       ddrBbit0
#define    direction_Txen       ddrDbit4
/****************************************/
#define    iSPI_Sck             portDbit6
#define    iSPI_Sdi             portDbit5
#define    iSPI_Sdo             inputDbit5
#define    iSPI_Scs             portDbit7

#define    GPIO1_WTR            inputDbit2
#define    iRxen                portBbit0
#define    iTxen                portDbit4

#define    declare_sdio_input()   direction_Sdio   =  direction_input
#define    declare_sdio_output()  direction_Sdio   =  direction_output
/****************************************/
void    Initialisation_RF(void);
void    RF_Setup_StandBy(void);
void    RF_Setup_receiver(void);
void    RF_Setup_transmiter(void);

void    A7105_Id_Write(void);
void    A7105_Fifo_Read(uchar *read_buff);
void    A7105_Fifo_Write(uchar *write_buff);
void    RF_Setup_Channel(unsigned char channel);
unsigned char A7105_Register_Read(unsigned char address);
void    A7105_Register_Write(unsigned char address,unsigned char parameter);
void    A7105_Config_Chip(void);
void    A7105_Reset_Chip(void);


