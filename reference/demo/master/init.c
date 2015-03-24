#include  "init.h"
/***********************************************
�� ��:	Init_Timer1
�� ��:	��ʼ��MCUʱ�������
�� ��:	/
�� ��:	/
�� ��:	1ms
***********************************************/
void    Init_Timer1(void)
{
        /* timer1 time 1ms */
        OCR1AH       = 0x0F;
        OCR1AL       = 0xA0;

        TCNT1H       = 0x00;
        TCNT1L       = 0x00;

        /* CLKi/o = 1,no prescaling,CTC mode */
        TCCR1A       = 0x00;
        TCCR1B       = 0x09;
        TCCR1C       = 0x00;

        /* disable compareB,overflow interrupt,enable compareA */
        TIFR1        = 0x27;      
        TIMSK1       = 0x02;
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
	
        /* ��ʼ��timer1��ʱ1ms */
        Init_Timer1();
        
        /* ��ʼ��RF */
        Initialisation_RF();

        /* ��־�Ĵ���Ϊ0 */
        R_flag0           = 0x00;
}
