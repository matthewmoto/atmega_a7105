#include    "mac-A7105.h"
/***********************************************
函 数:	Event_Wireless_Receive()
功 能:	接收RF状态处理
输 入:	/
输 出:	/
描 述:	主程序采用调用此子程序查询RF状态
***********************************************/
void    Event_Wireless_Receive(uchar *read_buffer)
{
        uchar temp;
        
        if(!GPIO1_WTR)
        {
            temp    = A7105_Register_Read(0x00);
            
            if((temp&0x20) == 0x00) 
            {/* CRC校验正确,读出FIFO数据 */
                A7105_Fifo_Read(read_buffer);              
                

                /* 客户添加自己程序,RF数据校验正确,处理数据 */
                bFlag_Packet_Receive  = true;
                
                
            }             
            
            /* 重新进入接收状态 */    
            RF_Setup_receiver();
        }
}
