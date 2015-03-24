#include  "init.h"
/***********************************************
函 数:	Init_Timer1
功 能:	初始化MCU时间计数器
输 入:	/
输 出:	/
描 述:	1ms
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
	
        /* 初始化timer1定时1ms */
        Init_Timer1();
        
        /* 初始化RF */
        Initialisation_RF();

        /* 标志寄存器为0 */
        R_flag0           = 0x00;
}
