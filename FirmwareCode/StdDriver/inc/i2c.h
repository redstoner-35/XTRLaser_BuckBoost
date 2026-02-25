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

/*****************************************************************************/
/** \file i2c.h
**
** 
**
** History:
** 
*****************************************************************************/
#ifndef __I2C_H__
#define __I2C_H__
/*****************************************************************************/
/* Include files */
/*****************************************************************************/
#include "cms8s6990.h"
/*****************************************************************************/
/* Global pre-processor symbols/macros ('#define') */
/*****************************************************************************/

/*----I2C主控模式命令-------------------------------------------------------*/
#define I2C_MASTER_START_SEND				(I2C_I2CMCR_START_Msk | I2C_I2CMCR_RUN_Msk)		/*起始位+从机地址+写+1Byte数据*/
#define I2C_MASTER_SEND						(I2C_I2CMCR_RUN_Msk)							/*发送1Byte数据*/
#define I2C_MASTER_STOP						(I2C_I2CMCR_STOP_Msk)							/*发送停止位*/
#define I2C_MASTER_START_RECEIVE_NACK		(I2C_I2CMCR_START_Msk | I2C_I2CMCR_RUN_Msk)		/*起始位+从机地址+读+1Byte时钟+NACK*/
#define I2C_MASTER_START_RECEIVE_ACK		(I2C_I2CMCR_START_Msk | I2C_I2CMCR_RUN_Msk | I2C_I2CMCR_ACK_Msk) /*起始位+从机地址+读+1Byte时钟+ACK*/
#define I2C_MASTER_RECEIVE_NACK				(I2C_I2CMCR_RUN_Msk)							/*发送读1Byte时钟+NACK*/
#define I2C_MASTER_RECEIVE_ACK				(I2C_I2CMCR_RUN_Msk | I2C_I2CMCR_ACK_Msk)		/*发送读1Byte时钟+ACK*/


/*****************************************************************************/
/* Global type definitions ('typedef') */
/*****************************************************************************/


/*****************************************************************************/
/* Global variable declarations ('extern', definition in C source) */
/*****************************************************************************/


/*****************************************************************************/
/* Global function prototypes ('extern', definition in C source) */
/*****************************************************************************/
/*****************************************************************************
 ** \brief	I2C_WaitACK
 **			主控模式下，写入数据后等待从机通信
 ** \param [in] none
 ** \return  bit：如果通信成功结束，返回0，否则返回1
*****************************************************************************/
bit I2C_WaitACK(void);
/*****************************************************************************
 ** \brief	I2C_DeInit
 **			复位整个I2C模块
 ** \param [in] none
 ** \return  none
 ** \note	 
*****************************************************************************/
void I2C_DeInit(void);
/*****************************************************************************
 ** \brief	I2C_SendByte
 **			主控模式下，向从机写入指定长度的数据
 ** \param [in] SlaveAddr: 0x0~0x7f(从机地址，不需要读写位)
 **             Reg : 目标需要写入的从机地址
 **             Data : 需要输出的数组
 **             Len : 数组的长度
 ** \return  bit：如果通信成功结束，返回0，否则返回1
*****************************************************************************/
bit I2C_SendByte(unsigned char SlaveAddr,char *Data,char Reg,unsigned char Len);

/*****************************************************************************
 ** \brief	I2C_SendOneByte
 **			主控模式下，向从机写入1字节长度的数据
 ** \param [in] SlaveAddr: 0x0~0x7f(从机地址，不需要读写位)
 **             Reg : 目标需要写入的从机地址
 **             Data : 需要发走的1字节数据
 ** \return  bit：如果通信成功结束，返回0，否则返回1
*****************************************************************************/	
bit I2C_SendOneByte(unsigned char SlaveAddr,char Data,char Reg);

/*****************************************************************************
 ** \brief	I2C_SendByte
 **			主控模式下，从从机中读取指定长度的数据
 ** \param [in] SlaveAddr: 0x0~0x7f(从机地址，不需要读写位)
 **             Reg : 目标需要写入的从机地址
 **             Data : 需要输出的数组
 **             Len : 数组的长度
 ** \return  bit：如果通信成功结束，返回0，否则返回1
*****************************************************************************/
bit I2C_ReadByte(unsigned char SlaveAddr,char *Data,char Reg,unsigned char Len);

/*****************************************************************************
 ** \brief	I2C_ReadOneByte
 **			主控模式下，从从机中读1个字节的数据
 ** \param [in] SlaveAddr: 0x0~0x7f(从机地址，不需要读写位)
 **             Reg : 目标需要写入的从机地址
 **             Data : 需要输出的数据
 ** \return  bit：如果通信成功结束，返回0，否则返回1
*****************************************************************************/	
bit I2C_ReadOneByte(unsigned char SlaveAddr,char *Data,char Reg);

/*****************************************************************************
 ** \brief	I2C_ConfigCLK
 **			配置I2C的时钟
 ** \param [in] I2CMtp: 0x0~0x7f
 ** \return  none
 ** \note	(1)I2CMtp = 0 ,SCL = 3*10*Tsys
 **			(2)I2CMtp != 0, SCL = 2*(1+I2CMtp)*10*Tsys
*****************************************************************************/
void I2C_ConfigCLK(uint8_t I2CMtp);

/*****************************************************************************
 ** \brief	I2C_EnableMasterMode
 **			使能主控模式
 ** \param [in] none
 ** \return  none
 ** \note	 
*****************************************************************************/
void I2C_EnableMasterMode(void);
/*****************************************************************************
 ** \brief	I2C_DisableMasterMode
 **			关闭主控模式
 ** \param [in] none
 ** \return  none
 ** \note	 
*****************************************************************************/
void I2C_DisableMasterMode(void);
/*****************************************************************************
 ** \brief	I2C_GetMasterSendAddrFlag
 **			获取主控模式寻址应答标志
 ** \param [in] none
 ** \return 0: 从机有应答  1；从机无应答
 ** \note	 
*****************************************************************************/
uint8_t I2C_GetMasterSendAddrFlag(void);
/*****************************************************************************
 ** \brief	I2C_GetMasterSendDataFlag
 **			获取主控模发送数据应答标志
 ** \param [in] none
 ** \return 0: 从机有应答  1；从机无应答
 ** \note	 
*****************************************************************************/
uint8_t I2C_GetMasterSendDataFlag(void);
/*****************************************************************************
 ** \brief	I2C_GetMasterBusArbitrationFlag
 **			获取主控模总线仲裁标志
 ** \param [in] none
 ** \return 0: 正常  1:丢失了总线控制权
 ** \note	 
*****************************************************************************/
uint8_t I2C_GetMasterBusArbitrationFlag(void);
/*****************************************************************************
 ** \brief	I2C_GetMasterErrorFlag
 **			获取主控模式错误标志
 ** \param [in] none
 ** \return 0: 无错误 1；有错误
 ** \note	错误标志产生条件：(1)寻址从机无应答 (2)发送数据从机无应答
 **							  (3)I2C总线仲裁冲突
*****************************************************************************/
uint8_t I2C_GetMasterErrorFlag(void);
/*****************************************************************************
 ** \brief	I2C_GetMasterBusyFlag
 **			获取主控模块Busy状态标志
 ** \param [in] none
 ** \return  1；正在发送
 ** \note	
*****************************************************************************/
uint8_t I2C_GetMasterBusyFlag(void);

/*****************************************************************************
 ** \brief	I2C_GetMasterIdleFlag
 **			获取主控模式空闲状态标志
 ** \param [in] none
 ** \return 0: 工作 1；空闲
 ** \note	
*****************************************************************************/
uint8_t I2C_GetMasterIdleFlag(void);
/*****************************************************************************
 ** \brief	I2C_GetBusBusyFlag
 **			获取总线忙状态标志
 ** \param [in] none
 ** \return 0: 空闲 1；总线忙
 ** \note	
*****************************************************************************/
uint8_t I2C_GetBusBusyFlag(void);

/*****************************************************************************
 ** \brief	I2C_EnableInt
 **			开启中断
 ** \param [in] none
 ** \return  none
 ** \note	 
*****************************************************************************/
void I2C_EnableInt(void);
/*****************************************************************************
 ** \brief	I2C_DisableInt
 **			关闭中断
 ** \param [in] none
 ** \return  none
 ** \note	 
*****************************************************************************/
void I2C_DisableInt(void);
/*****************************************************************************
 ** \brief	I2C_GetMasterIntFlag
 **			获取主控模式下的中断标志位
 ** \param [in] none
 ** \return  0:无中断 1：有中断
 ** \note	 
*****************************************************************************/
uint8_t I2C_GetMasterIntFlag(void);
/*****************************************************************************
 ** \brief	I2C_ClearMasterIntFlag
 **			清除主控模式下的中断标志位
 ** \param [in] none
 ** \return  none
 ** \note	 
*****************************************************************************/
void I2C_ClearMasterIntFlag(void);











#endif /* __I2C_H__ */
