/*******************************************************************************
* Copyright (C) 2019 China Micro Semiconductor Limited Company. All Rights Reserved.
*
* This software is owned and published by:
* CMS LLC, No 2609-10, Taurus Plaza, TaoyuanRoad, NanshanDistrict, Shenzhen, China.
*
* BY DOWNLOADING, INSTALLING OR USING THIS SOFTWARE, YOU AGREE TO BE BOUND
* BY ALL THE TERMS AND CONDITIONS OF THIS AGREEMENT.
*
* This software contains source code for use with CMS
* components. This software is licensed by CMS to be adapted only
* for use in systems utilizing CMS components. CMS shall not be
* responsible for misuse or illegal use of this software for devices not
* supported herein. CMS is providing this software "AS IS" and will
* not be responsible for issues arising from incorrect user implementation
* of the software.
*
* This software may be replicated in part or whole for the licensed use,
* with the restriction that this Disclaimer and Copyright notice must be
* included with each copy of this software, whether used in part or whole,
* at all times.
*/

/****************************************************************************/
/** \file i2c.c
**
** 
**
**	History:
**	
*****************************************************************************/
/****************************************************************************/
/*	include files
*****************************************************************************/
#include "i2c.h"

/****************************************************************************/
/*	Local pre-processor symbols/macros('#define')
****************************************************************************/

/****************************************************************************/
/*	Global variable definitions(declared in header file with 'extern')
****************************************************************************/

/****************************************************************************/
/*	Local type definitions('typedef')
****************************************************************************/

/****************************************************************************/
/*	Local variable  definitions('static')
****************************************************************************/

/****************************************************************************/
/*	Function implementation - local ('static')
****************************************************************************/
/*****************************************************************************
 ** \brief	I2C_WaitACK
 **			主控模式下，写入数据后等待从机通信
 ** \param [in] none
 ** \return  bit：如果通信成功结束，返回0，否则返回1
*****************************************************************************/
static bit I2C_WaitACK(void)
	{
	int i=500,j=500;
	while(!(I2CMSR & I2C_I2CMSR_I2CMIF_Msk))
		{
		if(!i) //通信超时
			{
		  I2CMCR=0x80; //强制复位总线
			_nop_();
			_nop_();
      _nop_(); //复位寄存器之后需要等待				
			I2C_EnableMasterMode(); //启用主控发送模式
			I2C_ConfigCLK(0x05); //Fsclk=Fsys/(2*10*(5+1))=400KHz		
			return 1;	
			}
		//倒计时
		j--;
		if(j)continue;
		i--;
		j=500;
		}
	I2CMSR=0x00; //清除I2C Flag		
	//出错啦
	if(I2CMSR & I2C_I2CMSR_ERROR_Msk)
		{
		I2CMCR=0x80; //强制复位总线
		_nop_();
		_nop_();
    _nop_(); //复位寄存器之后需要等待				
		I2C_EnableMasterMode(); //启用主控发送模式
		I2C_ConfigCLK(0x05); //Fsclk=Fsys/(2*10*(5+1))=400KHz		
	  return 1;
		}
	//顺利结束通信
	return 0;
	}
	
/****************************************************************************/
/*	Function implementation - global ('extern')
****************************************************************************/
	
/*****************************************************************************
 ** \brief	I2C_SendByte
 **			主控模式下，向从机写入指定长度的数据
 ** \param [in] SlaveAddr: 0x0~0x7f(从机地址，不需要读写位)
 **             Reg : 目标需要写入的从机地址
 **             Data : 需要输出的数组
 **             Len : 数组的长度
 ** \return  bit：如果通信成功结束，返回0，否则返回1
*****************************************************************************/
//bit I2C_SendByte(unsigned char SlaveAddr,char *Data,char Reg,unsigned char Len)
//	{
//	//设置地址
//	I2CMSA=(SlaveAddr<<1)&0xFE;
//	I2CMBUF=Reg;
//	I2CMCR=I2C_MASTER_START_SEND; //发送起始位，从机地址和第一个数据
//	if(I2C_WaitACK())return 1;
//	//发送数据
//	if(Len)do
//		{
//		I2CMBUF=*Data;				
//		I2CMCR=I2C_MASTER_SEND;		//发送数据
//		if(I2C_WaitACK())return 1;
//		//数据发送完毕，指向下一个数据
//		if(Len>1)Data++;
//		}	
//	while(--Len);
//	//通信结束
//	I2CMCR=I2C_MASTER_STOP;
//	while(!(I2CMSR & I2C_I2CMSR_IDLE_Msk));	//设置Stop命令后等待IDLE位置起
//	return 0;
//	}

/*****************************************************************************
 ** \brief	I2C_SendOneByte
 **			主控模式下，向从机写入1字节长度的数据
 ** \param [in] SlaveAddr: 0x0~0x7f(从机地址，不需要读写位)
 **             Reg : 目标需要写入的从机地址
 **             Data : 需要发走的1字节数据
 ** \return  bit：如果通信成功结束，返回0，否则返回1
*****************************************************************************/	
bit I2C_SendOneByte(unsigned char SlaveAddr,char Data,char Reg)	
	{
	//设置地址并发送起始位，从机地址和第一个数据（寄存器地址）
	I2CMSA=(SlaveAddr<<1)&0xFE;
	I2CMBUF=Reg;
	I2CMCR=I2C_MASTER_START_SEND;
	if(I2C_WaitACK())return 1;	
	//发送实际的数据
	I2CMBUF=Data;				
	I2CMCR=I2C_MASTER_SEND;		//发送数据
	if(I2C_WaitACK())return 1;
	//通信结束
	I2CMCR=I2C_MASTER_STOP;
	while(!(I2CMSR & I2C_I2CMSR_IDLE_Msk));	//设置Stop命令后等待IDLE位置起
	return 0;	
	}
	
/*****************************************************************************
 ** \brief	I2C_ReadOneByte
 **			主控模式下，从从机中读取指定长度的数据
 ** \param [in] SlaveAddr: 0x0~0x7f(从机地址，不需要读写位)
 **             Reg : 目标需要写入的从机地址
 **             Data : 需要输出的数组
 **             Len : 数组的长度
 ** \return  bit：如果通信成功结束，返回0，否则返回1
*****************************************************************************/	
bit I2C_ReadOneByte(unsigned char SlaveAddr,char *Data,char Reg)
	{
	//设置地址和寄存器
	I2CMSA=(SlaveAddr<<1)&0xFE;
	I2CMBUF=Reg;
	I2CMCR=I2C_MASTER_START_SEND; //发送起始位，从机地址和第一个数据
	if(I2C_WaitACK())return 1;
	//发送读地址并开始接收数据
	I2CMSA=(SlaveAddr<<1)|0x01; //设置读取地址
	I2CMCR=I2C_MASTER_START_RECEIVE_NACK; //读一个字节，发送NACK
	if(I2C_WaitACK())return 1; 
	*Data=I2CMBUF; 														 //等待读取结束后保存第一个字节
//通信结束
	I2CMCR=I2C_MASTER_STOP;
	while(!(I2CMSR & I2C_I2CMSR_IDLE_Msk));	//设置Stop命令后等待IDLE位置起
	return 0;		
	}
	
/*****************************************************************************
 ** \brief	I2C_ReadByte
 **			主控模式下，从从机中读取指定长度的数据
 ** \param [in] SlaveAddr: 0x0~0x7f(从机地址，不需要读写位)
 **             Reg : 目标需要写入的从机地址
 **             Data : 需要输出的数组
 **             Len : 数组的长度
 ** \return  bit：如果通信成功结束，返回0，否则返回1
*****************************************************************************/
//bit I2C_ReadByte(unsigned char SlaveAddr,char *Data,char Reg,unsigned char Len)
//	{
//	//检查传入的读取长度是否合适
//	if(!Len)return 1;
//	//设置地址和寄存器
//	I2CMSA=(SlaveAddr<<1)&0xFE;
//	I2CMBUF=Reg;
//	I2CMCR=I2C_MASTER_START_SEND; //发送起始位，从机地址和第一个数据
//	if(I2C_WaitACK())return 1;
//	//读取完需要等一会
//  _nop_();
//	_nop_();	
//	//发送读地址并开始接收数据
//	I2CMSA=(SlaveAddr<<1)|0x01; //设置读取地址
//	Len--;                      
//	if(Len)I2CMCR=I2C_MASTER_START_RECEIVE_ACK; //如果Len>1,减了之后仍然大于1的话则说明需要读多个字节，发送ACK继续读
//	else I2CMCR=I2C_MASTER_START_RECEIVE_NACK; //只需要读1字节，读完之后发送NACK
//	if(I2C_WaitACK())return 1; 
//	*Data=I2CMBUF; 														 //等待读取结束后保存第一个字节
//	//如果需要多字节读取，则循环接受剩下的数据
//	while(Len)
//		{	
//		Data++;           					//指向下一个数据
//		I2CMCR=Len>1?I2C_MASTER_RECEIVE_ACK:I2C_MASTER_RECEIVE_NACK; //设置I2C的操作
//		if(I2C_WaitACK())return 1; //等待读取
//		*Data=I2CMBUF; 						 //将收到的数据放入寄存器内
//		//一个数据读取完毕，接下来到下一个	
//		Len--;
//		}

//	//通信结束
//	I2CMCR=I2C_MASTER_STOP;
//	while(!(I2CMSR & I2C_I2CMSR_IDLE_Msk));	//设置Stop命令后等待IDLE位置起
//	return 0;
//	}	
	
/*****************************************************************************
 ** \brief	I2C_ConfigCLK
 **			配置I2C的时钟
 ** \param [in] I2CMtp: 0x0~0x7f
 ** \return  none
 ** \note	(1)I2CMtp = 0 ,SCL = 3*10*Tsys
 **			(2)I2CMtp != 0, SCL = 2*(1+I2CMtp)*10*Tsys
*****************************************************************************/
void I2C_ConfigCLK(uint8_t I2CMtp)
{
	I2CMTP = I2CMtp;
}
/*****************************************************************************
 ** \brief	I2C_EnableMasterMode
 **			使能主控模式
 ** \param [in] none
 ** \return  none
 ** \note	 
*****************************************************************************/
void I2C_EnableMasterMode(void)
{
	I2CMCR = 0x00;
	I2CSCR  = 0x00;
}

/*****************************************************************************
 ** \brief	I2C_DeInit
 **			复位整个I2C模块
 ** \param [in] none
 ** \return  none
 ** \note	 
*****************************************************************************/
void I2C_DeInit(void)
	{
	I2CMCR=0x80;
	_nop_();
	_nop_();
  I2CSCR=0x80;
	}


///*****************************************************************************
// ** \brief	I2C_GetMasterSendAddrFlag
// **			获取主控模式寻址应答标志
// ** \param [in] none
// ** \return 0: 从机有应答  1；从机无应答
// ** \note	 
//*****************************************************************************/
//uint8_t I2C_GetMasterSendAddrFlag(void)
//{
//	return ((I2CMSR & I2C_I2CMSR_ADD_ACK_Msk)? 1:0);
//}

///*****************************************************************************
// ** \brief	I2C_GetMasterErrorFlag
// **			获取主控模式错误标志
// ** \param [in] none
// ** \return 0: 无错误 1；有错误
// ** \note	错误标志产生条件：(1)寻址从机无应答 (2)发送数据从机无应答
// **							  (3)I2C总线仲裁冲突
//*****************************************************************************/
//uint8_t I2C_GetMasterErrorFlag(void)
//{
//	return ();	
//}
/*****************************************************************************
 ** \brief	I2C_GetMasterBusyFlag
 **			获取主控模块Busy状态标志
 ** \param [in] none
 ** \return  1；正在发送
 ** \note	
*****************************************************************************/
//uint8_t I2C_GetMasterBusyFlag(void)
//{
//	return ((I2CMSR & I2C_I2CMSR_BUSY_Msk)? 1:0);	
//}
///*****************************************************************************
// ** \brief	I2C_GetI2CBusBusyFlag
// **			获取总线忙状态标志
// ** \param [in] none
// ** \return 0: 空闲 1；总线忙
// ** \note	
//*****************************************************************************/
//uint8_t I2C_GetI2CBusBusyFlag(void)
//{
//	return ((I2CMSR & I2C_I2CMSR_BUS_BUSY_Msk)? 1:0);	
//}
///*****************************************************************************
// ** \brief	I2C_GetMasterIdleFlag
// **			获取主控模式空闲状态标志
// ** \param [in] none
// ** \return 0: 工作 1；空闲
// ** \note	
//*****************************************************************************/
//uint8_t I2C_GetMasterIdleFlag(void)
//{
//	return ((I2CMSR & I2C_I2CMSR_IDLE_Msk)? 1:0);	
//}

///*****************************************************************************
// ** \brief	I2C_EnableInt
// **			开启中断
// ** \param [in] none
// ** \return  none
// ** \note	 
//*****************************************************************************/
//void I2C_EnableInt(void)
//{
//	EIE2 |= IRQ_EIE2_I2CIE_Msk;
//}
///*****************************************************************************
// ** \brief	I2C_DisableInt
// **			关闭中断
// ** \param [in] none
// ** \return  none
// ** \note	 
//*****************************************************************************/
//void I2C_DisableInt(void)
//{
//	EIE2 &= ~(IRQ_EIE2_I2CIE_Msk);
//}
///*****************************************************************************
// ** \brief	I2C_GetMasterIntFlag
// **			获取主控模式下的中断标志位
// ** \param [in] none
// ** \return  0:无中断 1：有中断
// ** \note	 
//*****************************************************************************/
//uint8_t I2C_GetMasterIntFlag(void)
//{
//	return (()? 1:0);	
//}
///*****************************************************************************
// ** \brief	I2C_ClearMasterIntFlag
// **			清除主控模式下的中断标志位
// ** \param [in] none
// ** \return  none
// ** \note	 
//*****************************************************************************/
//void I2C_ClearMasterIntFlag(void)
//{
//	I2CMSR = 0x00;
//}


