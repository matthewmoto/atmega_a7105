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
        
        /* 使能全局中断 */
        __enable_interrupt();
        
        while(true)
        {     
            if(bFlag_Packet_Transimit)
            { 
                Event_Transimit_Packet(send);
                
                send_count            = 1000;
                bFlag_Packet_Transimit= false;                
            }
        }        
}
/***********************************************
函 数:	Timer1_CompareA_Entry
功 能:	时间比较中断处理
输 入:	/
输 出:	/
描 述:	/
***********************************************/
#pragma      vector = TIMER1_COMPA_vect
__interrupt  void  Timer1_CompareA_Entry(void)
{
        if(send_count)
        {
            if((--send_count) == 0x00)
                bFlag_Packet_Transimit    = true;
        }
}

