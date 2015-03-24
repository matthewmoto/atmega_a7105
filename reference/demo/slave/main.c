		/***********************************************
		*	copyrite (c) 2003
		*公司:		  深圳科技有限公司
		*文件名: 	  main.c(IAR Embedded Workbench IDE 4.10 for AVR)
		*作者:		  larry
		*当前版本:        v1.0
		*开始时间:        2008-3-12 10:13
		*完成日期:	  
		*摘要:		  晶振8Mhz,mega48,+5v supply
		*修改:		
		************************************************/				
#include	"main.h"
/***********************************************
函 数:	main
功 能:	程序入口
输 入:	/
输 出:	/
描 述:	/
***********************************************/
void main(void)
{
        /* 上电初始化系统 */
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

