#ifndef uchar
        #define     uchar   unsigned char
        #define     uint    unsigned int

        #define     true    1
        #define     false   0

        #define     on      0
        #define     off     1                              

        #define     enable  1
        #define     disable 0                              

        #define     direction_output  1
        #define     direction_input   0                    
#endif
/****************************************/
#include  "intrinsics.h"
#include  "iom48.h"
#include  "sbit.h"
#include  "A7105.h"
/****************************************/
#define    direction_rxd        ddrDbit0
#define    direction_txd        ddrDbit1
/****************************************/
    /* declare const */    
#define    crystal              4000000    
#define    cFIFOlength          0x08               /* FIFO发送和接收一次的字节擿*/
/****************************************/
    /* declare flag */
__regvar __no_init volatile union{
                                  uchar R_flag0;
                                  struct{
                                         uchar
                                            bFlag_Packet_Receive:1;
                                        };
                                 }@ 0x0f;    
/****************************************/
     /* declare extern variable */
/* main.h */

/* init.h */
/****************************************/
     /* declare extern founction */

/* init.c */
extern  void    PowerOn_Initialisation(void);

/* phy-A7105.c */
extern	void    Initialisation_RF(void);
extern  void    RF_Setup_StandBy(void);
extern  void    RF_Setup_receiver(void);
extern  void    RF_Setup_transmiter(void);
extern  void    A7105_Fifo_Read(uchar *read_buff);
extern  void    A7105_Fifo_Write(uchar *write_buff);
extern  unsigned char A7105_Register_Read(unsigned char address);

/* mac-A7105.c */
extern  void    Event_Wireless_Receive(uchar *read_buffer);
