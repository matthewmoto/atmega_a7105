#include    "mac-A7105.h"
/***********************************************
�� ��:	Event_Transimit_Packet()
�� ��:	����RF���ݰ�����
�� ��:	/
�� ��:	/
�� ��:	/
***********************************************/
void    Event_Transimit_Packet(uchar *write_buffer)
{
        /* д��A7105�ڲ�FIFO��Ҫ��������� */
        A7105_Fifo_Write(&write_buffer[0]);
                
        /* ���뷢��״̬,����Ӳ���Զ��������� */
        RF_Setup_transmiter();
                
        /* �ȴ������������,��ѭ����ѯIO��ʽ,�ͻ��ɸ�����Ҫ�Լ��޸�Ϊ�жϵȷ�ʽ */
        while(GPIO1_WTR){};
                
        /* ��ɵ�ǰ����֡�ķ����,Ӳ���Զ�������StandByģʽ,�ͻ��ɸ����Լ���Ҫ�޸�Ϊsleep����receiver��,��ǰѡ��������״̬ */
        RF_Setup_receiver();
}
