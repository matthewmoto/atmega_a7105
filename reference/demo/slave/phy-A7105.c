#include    "phy-A7105.h"
/***********************************************
�� ��:	A7105_InterFace()
�� ��:	MCU��A7105�Ľӿ�����
�� ��:	/
�� ��:	/
�� ��:	/
***********************************************/
void    A7105_InterFace(void)
{
        iSPI_Scs    = positive;
        iSPI_Sck    = negative;
        iSPI_Sdo    = negative;
        
        iRxen       = positive;
        iTxen       = negative;
        
        direction_Scs    = direction_output;
        direction_Sck    = direction_output;
        direction_Sdio   = direction_output;
        direction_Rxen   = direction_output;
        direction_Txen   = direction_output;
        
        direction_GPIO1_WTR  = direction_input;
}
/***********************************************
�� ��:	Register_Write()
�� ��:	дbit���ݵ�sck��sdio
�� ��:	byte
�� ��:	/
�� ��:	����λ,bit7����,sck����Ե����
***********************************************/
void    Register_Write(unsigned char byte,unsigned char bits)
{
        unsigned char i;

        for(i = 0x00; i < bits; i++)
        {
            if(byte&0x80)
                iSPI_Sdi     = positive;
            else
                iSPI_Sdi     = negative;
                
            iSPI_Sck         = positive;
            iSPI_Sck         = negative;
            
            byte           <<= 0x01;
        }
}
/***********************************************
�� ��:	Register_Read()
�� ��:	��8bit���ݴ�sck��sdio
�� ��:	/
�� ��:	byte
�� ��:	����λ,bit7����,sck����Ե����
***********************************************/
unsigned char   Register_Read(void)
{
        unsigned char i;
        unsigned char byte;

        for(i = 0x00; i < 0x08; i++)
        {
            byte           <<= 0x01;
            
            if(iSPI_Sdo)
                byte        |= 0x01;
            else
                byte        &= 0xfe;

            iSPI_Sck         = positive;
            iSPI_Sck         = negative;
        }

        /* ���ض�ȡ��ֵ */
        return    (byte);
}
/***********************************************
�� ��:	A7105_Command_Write()
�� ��:	д�����A7105�Ĵ���
�� ��:	/
�� ��:	/
�� ��:	/
***********************************************/
void    A7105_Command_Write(unsigned char command)
{
        iSPI_Scs        = negative;
        
        Register_Write(command,cNumber_4bit);
        
        iSPI_Scs        = positive;
}
/***********************************************
�� ��:	A7105_Register_Write()
�� ��:	д�����ݵ�A7105�Ĵ���
�� ��:	/
�� ��:	/
�� ��:	/
***********************************************/
void    A7105_Register_Write(unsigned char address,unsigned char parameter)
{
        iSPI_Scs        = negative;
        
        /* bit7 =0:data register */
        address        &= 0x7f;
        
        Register_Write(address,cNumber_8bit);
        Register_Write(parameter,cNumber_8bit);
        
        iSPI_Scs        = positive;
}
/***********************************************
�� ��:	A7105_Register_Read()
�� ��:	����״̬��A7105�Ĵ���
�� ��:	/
�� ��:	/
�� ��:	/
***********************************************/
unsigned char A7105_Register_Read(unsigned char address)
{
        unsigned char temp;
        
        iSPI_Scs        = negative;
        
        /* bit7 = 0:data register,bit6 = 1:read data from register */
        address        &= 0x7f;
        address        |= 0x40;
        
        Register_Write(address,cNumber_8bit);
        
        declare_sdio_input();
        temp            = Register_Read();        
        declare_sdio_output();

        iSPI_Scs        = positive;
        
        /* ���ض�ȡ��ֵ */
        return    (temp);
}
/***********************************************
�� ��:	A7105_Reset_Chip()
�� ��:	��λA7105оƬ
�� ��:	/
�� ��:	/
�� ��:	/
***********************************************/
void    A7105_Reset_Chip(void)
{
        /* дA7105оƬReset */
        A7105_Register_Write(Reg_Mode,0x00);
}
/***********************************************
�� ��:	RF_Setup_Channel()
�� ��:	A7105����Ƶ��ͨ��
�� ��:	/
�� ��:	/
�� ��:	500khz/ͨ����2400mhz = base frequency
***********************************************/
void    RF_Setup_Channel(unsigned char channel)
{
        A7105_Register_Write(Reg_PLL1,channel);
}
/***********************************************
�� ��:	RF_Setup_StandBy()
�� ��:	A7105����Ϊstby״̬
�� ��:	/
�� ��:	/
�� ��:	/
***********************************************/
void    RF_Setup_StandBy(void)
{
        A7105_Command_Write(cCommand_StandBy);
}
/***********************************************
�� ��:	RF_Setup_receiver()
�� ��:	A7105����Ϊ����״̬
�� ��:	/
�� ��:	/
�� ��:	/
***********************************************/
void    RF_Setup_receiver(void)
{
        RF_Setup_StandBy(); 
        RF_Setup_Channel(0x02);  /* Ĭ��2401MHZ */
        A7105_Command_Write(cCommand_RX);
        iRxen       = positive;
        iTxen       = negative;
}
/***********************************************
�� ��:	RF_Setup_transmiter()
�� ��:	A7105����Ϊ����״̬
�� ��:	/
�� ��:	/
�� ��:	/
***********************************************/
void    RF_Setup_transmiter(void)
{
        iRxen       = negative;
        iTxen       = positive;
        RF_Setup_StandBy(); 
        RF_Setup_Channel(0x03);   /* Ĭ��2401.5MHZ */
        A7105_Command_Write(cCommand_TX);
}
/***********************************************
�� ��:	A7105_Id_Write()
�� ��:	дID��A7105�ڲ��Ĵ���
�� ��:	/
�� ��:	/
�� ��:	дID����һ����д��
***********************************************/
void    A7105_Id_Write(void)
{
        iSPI_Scs        = negative;

        Register_Write(Reg_IdData,cNumber_8bit);
        
        Register_Write(cID_Code0,cNumber_8bit);
        Register_Write(cID_Code1,cNumber_8bit);
        Register_Write(cID_Code2,cNumber_8bit);
        Register_Write(cID_Code3,cNumber_8bit);

        iSPI_Scs        = positive;
}
/***********************************************
�� ��:	A7105_Fifo_Read()
�� ��:	��FIFO����
�� ��:	read_buff
�� ��:	/
�� ��:	/
***********************************************/
void    A7105_Fifo_Read(uchar *read_buff)
{
        uchar i;
        
        iSPI_Scs        = negative;

        /* ������ */
        Register_Write((Reg_FifoData|0x40),cNumber_8bit);

        declare_sdio_input();

        /* ��FIFO���� */
        for(i = 0x00;i < cFIFOlength;i++)
        {
            *read_buff  = Register_Read();

            read_buff ++;
        }

        declare_sdio_output();

        iSPI_Scs        = positive;
}
/***********************************************
�� ��:	A7105_Fifo_Write()
�� ��:	дFIFO����
�� ��:	write_buff
�� ��:	/
�� ��:	/
***********************************************/
void    A7105_Fifo_Write(uchar *write_buff)
{
        uchar i;
        
        iSPI_Scs        = negative;

        /* д���� */
        Register_Write(Reg_FifoData,cNumber_8bit);

        /* дFIFO���� */
        for(i = 0x00;i < cFIFOlength;i++)
        {
            Register_Write(*write_buff,cNumber_8bit);

            write_buff ++;
        }

        iSPI_Scs        = positive;
}
/***********************************************
�� ��:	A7105_Calibration()
�� ��:	Ƶ��У׼IF��VCO,VCO band
�� ��:	/
�� ��:	/
�� ��:	/
***********************************************/
void    A7105_Calibration(void)
{   
        unsigned char temp;        
        
        /* standby״̬��У׼IF */
        RF_Setup_StandBy();
        A7105_Register_Write(Reg_Calibration,0x01);
        do
        {
            temp      = A7105_Register_Read(Reg_Calibration);
            temp      = (temp&0x01);
        }while(temp);
        
        temp          = A7105_Register_Read(Reg_IFcalibration1);
        temp          = (temp&0x10);
        
        if(temp)
        {
            /* У׼���� */
            
        }
        
        /* anual vco current band 3,vco band 1 */
        A7105_Register_Write(Reg_VcoCurrentCal,0x13);
        A7105_Register_Write(Reg_VcoBandCal1,0x09);
        
        RF_Setup_StandBy();        
}
/***********************************************
�� ��:	A7105_Config_Chip()
�� ��:	����A7105�����Ĵ���
�� ��:	/
�� ��:	/
�� ��:	/
***********************************************/
void    A7105_Config_Chip(void)
{
        unsigned char i;

        /* 0x00 mode register, for reset */
        /* 0x05 fifo data register */
        /* 0x06 id code register */
        /* 0x23 IF calibration II, only read */
        /* 0x32 filter test register */

        for (i=0x01; i<=0x04; i++)
            A7105_Register_Write(i,A7105_Default_Parameter[i]);

        for (i=0x07; i<=0x22; i++)
            A7105_Register_Write(i,A7105_Default_Parameter[i]);

        for (i=0x24; i<=0x31; i++)
            A7105_Register_Write(i,A7105_Default_Parameter[i]);
}
/***********************************************
�� ��:	Initialisation_RF()
�� ��:	�ϵ��ʼ��A7105
�� ��:	/
�� ��:	/
�� ��:	/
***********************************************/
void    Initialisation_RF(void)
{
        uint i;

        A7105_InterFace();

        A7105_Reset_Chip();        
        
        for(i = 0;i < 0xffff;i++)
        {/* A7105�ȶ�ʱ���ӳ����� */
            asm("nop");
            asm("nop");
            asm("nop");
            asm("nop");
        } 
        
        A7105_Id_Write();

        A7105_Config_Chip();

        A7105_Calibration();
        
        RF_Setup_receiver();
}
