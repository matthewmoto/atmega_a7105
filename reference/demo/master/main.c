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
        
        /* ʹ��ȫ���ж� */
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
�� ��:	Timer1_CompareA_Entry
�� ��:	ʱ��Ƚ��жϴ���
�� ��:	/
�� ��:	/
�� ��:	/
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

