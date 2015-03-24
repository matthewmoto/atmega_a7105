#include    "mac-A7105.h"
/***********************************************
函 数:	Event_Transimit_Packet()
功 能:	发射RF数据包处理
输 入:	/
输 出:	/
描 述:	/
***********************************************/
void    Event_Transimit_Packet(uchar *write_buffer)
{
        /* 写入A7105内部FIFO需要发射的数据 */
        A7105_Fifo_Write(&write_buffer[0]);
                
        /* 进入发射状态,将由硬件自动发射数据 */
        RF_Setup_transmiter();
                
        /* 等待发射数据完成,死循环查询IO方式,客户可根据需要自己修改为中断等方式 */
        while(GPIO1_WTR){};
                
        /* 完成当前数据帧的发射后,硬件自动进入了StandBy模式,客户可根据自己需要修改为sleep或者receiver等,当前选择进入接收状态 */
        RF_Setup_receiver();
}
