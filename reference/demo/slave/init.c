#include  "init.h"
/***********************************************
函 数:	Init_Ports
功 能:	初始化IO状态
输 入:	/
输 出:	/
描 述:	/
***********************************************/
void    Init_Ports(void)
{     
        direction_rxd             = direction_input;        
        direction_txd             = direction_output;
}
/***********************************************
函 数:	Init_Rs232
功 能:	初始化MCU串口状态
输 入:	/
输 出:	/
描 述:	/
***********************************************/
void    Init_Rs232(void)
{
        /* U2X0 =1 */
        UCSR0A                 = 0x02;
        
        /* TXEN0 = 1,RXEN0 = 1,enable USART0 TX and USART0 RX,disable USART0 interrupt */
        UCSR0B                 = 0x18;

        /* asynchronism,8 bit data,none parity,1 stop bit */
        UCSR0C                 = 0x06;
 	
        /* 串口速率设置 */
        UBRR0H                 = ((crystal/9600/8)-1)/256;
        UBRR0L                 = ((crystal/9600/8)-1)%256;                 
}
/***********************************************
函 数:	PowerOn_Initialisation
功 能:	上电初始化MCU内部资源和RF
输 入:	/
输 出:	/
描 述:	/
***********************************************/
void    PowerOn_Initialisation(void)
{
        uint  i;

        /* 上电稳定时间延迟 */
        for(i = 0;i < 0xffff;i++)
        {
            asm("nop");
            asm("nop");
            asm("nop");
            asm("nop");
        }

        /* 初始化MCU端口状态 */
        Init_Ports();
        
        /* 初始化UART */
        Init_Rs232();
        
        /* 初始化RF */
        Initialisation_RF();

        /* 标志寄存器为0 */
        R_flag0           = 0x00;
}
