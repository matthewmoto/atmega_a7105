#include    "mac-A7105.h"
/***********************************************
�� ��:	Event_Wireless_Receive()
�� ��:	����RF״̬����
�� ��:	/
�� ��:	/
�� ��:	��������õ��ô��ӳ����ѯRF״̬
***********************************************/
void    Event_Wireless_Receive(uchar *read_buffer)
{
        uchar temp;
        
        if(!GPIO1_WTR)
        {
            temp    = A7105_Register_Read(0x00);
            
            if((temp&0x20) == 0x00) 
            {/* CRCУ����ȷ,����FIFO���� */
                A7105_Fifo_Read(read_buffer);              
                

                /* �ͻ�����Լ�����,RF����У����ȷ,�������� */
                bFlag_Packet_Receive  = true;
                
                
            }             
            
            /* ���½������״̬ */    
            RF_Setup_receiver();
        }
}
