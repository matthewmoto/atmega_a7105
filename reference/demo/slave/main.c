		/***********************************************
		*	copyrite (c) 2003
		*��˾:		  ���ڿƼ����޹�˾
		*�ļ���: 	  main.c(IAR Embedded Workbench IDE 4.10 for AVR)
		*����:		  larry
		*��ǰ�汾:        v1.0
		*��ʼʱ��:        2008-3-12 10:13
		*�������:	  
		*ժҪ:		  ����8Mhz,mega48,+5v supply
		*�޸�:		
		************************************************/				
#include	"main.h"
/***********************************************
�� ��:	main
�� ��:	�������
�� ��:	/
�� ��:	/
�� ��:	/
***********************************************/
void main(void)
{
        /* �ϵ��ʼ��ϵͳ */
        PowerOn_Initialisation();
        
        while(true)
        {
            Event_Wireless_Receive(rece_buff);
            
            if(bFlag_Packet_Receive)
            {  
                bFlag_Packet_Receive   = false;
                              
                UDR0     = rece_buff[0];
                while(!(UCSR0A&0x20)){};
                UCSR0A      |= 0x20;
                UDR0     = rece_buff[1];
                while(!(UCSR0A&0x20)){};
                UCSR0A      |= 0x20;
                UDR0     = rece_buff[2];
                while(!(UCSR0A&0x20)){};
                UCSR0A      |= 0x20;
                UDR0     = rece_buff[3];
                while(!(UCSR0A&0x20)){};
                UCSR0A      |= 0x20;
                UDR0     = rece_buff[4];
                while(!(UCSR0A&0x20)){};
                UCSR0A      |= 0x20;
                UDR0     = rece_buff[5];
                while(!(UCSR0A&0x20)){};
                UCSR0A      |= 0x20;
                UDR0     = rece_buff[6];
                while(!(UCSR0A&0x20)){};
                UCSR0A      |= 0x20;
                UDR0     = rece_buff[7];
                while(!(UCSR0A&0x20)){};
                UCSR0A      |= 0x20;
            }            
        }        
}

