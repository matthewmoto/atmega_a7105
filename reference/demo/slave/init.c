#include  "init.h"
/***********************************************
�� ��:	Init_Ports
�� ��:	��ʼ��IO״̬
�� ��:	/
�� ��:	/
�� ��:	/
***********************************************/
void    Init_Ports(void)
{     
        direction_rxd             = direction_input;        
        direction_txd             = direction_output;
}
/***********************************************
�� ��:	Init_Rs232
�� ��:	��ʼ��MCU����״̬
�� ��:	/
�� ��:	/
�� ��:	/
***********************************************/
void    Init_Rs232(void)
{
        /* U2X0 =1 */
        UCSR0A                 = 0x02;
        
        /* TXEN0 = 1,RXEN0 = 1,enable USART0 TX and USART0 RX,disable USART0 interrupt */
        UCSR0B                 = 0x18;

        /* asynchronism,8 bit data,none parity,1 stop bit */
        UCSR0C                 = 0x06;
 	
        /* ������������ */
        UBRR0H                 = ((crystal/9600/8)-1)/256;
        UBRR0L                 = ((crystal/9600/8)-1)%256;                 
}
/***********************************************
�� ��:	PowerOn_Initialisation
�� ��:	�ϵ��ʼ��MCU�ڲ���Դ��RF
�� ��:	/
�� ��:	/
�� ��:	/
***********************************************/
void    PowerOn_Initialisation(void)
{
        uint  i;

        /* �ϵ��ȶ�ʱ���ӳ� */
        for(i = 0;i < 0xffff;i++)
        {
            asm("nop");
            asm("nop");
            asm("nop");
            asm("nop");
        }

        /* ��ʼ��MCU�˿�״̬ */
        Init_Ports();
        
        /* ��ʼ��UART */
        Init_Rs232();
        
        /* ��ʼ��RF */
        Initialisation_RF();

        /* ��־�Ĵ���Ϊ0 */
        R_flag0           = 0x00;
}
