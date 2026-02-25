/************************************************************************************/
/** \file SC8721_REG.h
/** \Author redstoner_35
/** \Project Xtern Ripper Laser Edition 
/** \Description 这个文件为底层的硬件定义文件，定义了系统所使用的南芯SC8721A芯片I2C寄存
器的名称，部分bit Mask的配置。

**	History: Initial Release
**	
/************************************************************************************/
#ifndef _SC8721_REG_
#define _SC8721_REG_
/************************************************************************************/
/* Include files */
/************************************************************************************/

/************************************************************************************/
/* Register Address */
/************************************************************************************/
#define SC8721_REG_CSOSET 0x01  //CSO_SET
#define SC8721_REG_SLOPECOMP 0x02  //SLOPE_COMP
#define SC8721_REG_VOUTSETMSB 0x03 //VOUTSET_MSB
#define SC8721_REG_VOUTSETLSB 0x04 //VOUTSET_LSB
#define SC8721_REG_GLOBALCTRL 0x05 //GLOBAL_CTRL
#define SC8721_REG_SYSSET 0x06  //SYS_SET
#define SC8721_MFRREG_TERMSET 0x07   //南芯内部使用的特殊保留寄存器，不建议动
#define SC8721_REG_FREQSET 0x08 //FSW_SET
#define SC8721_REG_STATUS1 0x09 //STATUS_0
#define SC8721_REG_STATUS2 0x0A //STATUS_1

/************************************************************************************/
/* Register Clear to all zero before Init new value without corrupting reserved area*/
/************************************************************************************/
#define SC8721_CSOSET_CLR 0xFF  //清除CSOSET
#define SC8721_SLOPECOMP_CLR 0x03 //清除SLOPECOMP
#define SC8721_VOUTSETMSB_CLR 0xFF //清除VOUTSET_MSB
#define SC8721_VOUTSETLSB_CLR 0x1F  //清除VOUTSET_LSB
#define SC8721_GLOBALCTRL_CLR 0x06  //清除GLOBAL_CTRL
#define SC8721_SYSSET_CLR 0xD0  //清除SYSSET寄存器
#define SC8721_FREQSET_CLR 0x03 //设置开关频率

/************************************************************************************/
/* Command for Slope Compensation                                                   */
/************************************************************************************/
#define SC8721_SLOPECOMP_Disable 0x00   //关闭线损补偿
#define SC8721_SLOPECOMP_50mV 0x01      //开启线损补偿，每1A负载输出电压抬高50mV
#define SC8721_SLOPECOMP_100mV 0x02     //开启线损补偿，每1A负载输出电压抬高100mV
#define SC8721_SLOPECOMP_150mV 0x03     //开启线损补偿，每1A负载输出电压抬高150mV

/************************************************************************************/
/* Command for VOUT Setting                                                         */
/************************************************************************************/
#define SC8721_USING_ExtFB  (0 << 4)//使用外部反馈
#define SC8721_USING_IntFB  (1 << 4)//使用内部反馈 (设置FB_SEL)

//以下两个选项如果开启外部FB则无效，怎么填都行

#define SC8721_IntFB_ADJOFF  (0 << 3) //禁止内部FB调整输出电压，此时若开启内部FB则固定输出5V
#define SC8721_IntFB_ADJON  (1 << 3) //允许内部FB调整输出电压（开启内部FB后输出电压可以更改）

#define SC8721_IntFB_PosOFFSET  (0 << 2) //内部输出电压设定功能设置为正偏移（输出电压=5V+偏移量）
#define SC8721_IntFB_NegOFFSET  (1 << 2) //内部输出电压设定功能设置为正偏移（输出电压=5V-偏移量）

#define SC8721_SET_VOUTMSB(x) ((x >> 2UL)&0x00FF) //设置VOUT_MSB
#define SC8721_SET_VOUTLSB(x) (x&0x03UL) //设置VOUT_LSB

/************************************************************************************/
/* Command for DCDC Global Control                                                  */
/************************************************************************************/
#define SC8721_Cmd_EnableDCDC (0 << 2)
#define SC8721_Cmd_DisableDCDC (1 << 2) //软件使能/除能DCDC输出的bit

#define SC8721_SWEnCmd_BitPos 2  //SC8721芯片DCDC使能位的bit位置

#define SC8721_Cmd_ApplyDCDCSetting (1 << 1) //应用系统设置的DCDC参数

/************************************************************************************/
/* Command for DCDC Operation Mode Setting                                          */
/************************************************************************************/

//设置SC8721的控制模式是FPWM还是自动PFM，自动PFM下低负载效率提升但是输出纹波更高
#define SC8721_CtrlMode_PFM (0 << 7)  
#define SC8721_CtrlMode_FPWM (1 << 7)  

//设置SC8721的片内驱动器的死区时间，一般默认就行，死区时间更大会降低效率
#define SC8721_DrvDT_20nS (0 << 6)
#define SC8721_DrvDT_40nS (1 << 6)

//设置SC8721的输入自适应防拉死功能是否启用（当输入电压降低到阈值以下后系统自动限制输出）
#define SC8721_VINREG_Disable (0 << 4)
#define SC8721_VINREG_Enable (1 << 4)

/************************************************************************************/
/* Command for DCDC Switching Frequency Setting                                     */
/************************************************************************************/
#define SC8721_FSW_260KHz 0x00  //设置芯片开关频率为260KHz
#define SC8721_FSW_500KHz 0x01  //设置芯片开关频率为500KHz
#define SC8721_FSW_720KHz 0x02  //设置芯片开关频率为720KHz
#define SC8721_FSW_920KHz 0x03  //设置芯片开关频率为920KHz

/************************************************************************************/
/* Status flag mask for DCDC status byte feedback（For STATUS_1 Register）          */
/************************************************************************************/
#define SC8721_Fault_VOUTShort_Msk (1 << 7)  //输出短路（输出被拉低到小于2.7V）
#define SC8721_Fault_TSD_Msk       (1 << 3)  //芯片本身过温关闭
#define SC8721_Warning_CBCOCP_Msk  (1 << 0)  //芯片内部触发逐周期过流保护
#define SC8721_STATU_ICREGMode_Msk (1 << 6)  //芯片当前的工作状态bit，0=BUCK，1=Boost

/************************************************************************************/
/* Status flag mask for DCDC status byte feedback（For STATUS_2 Register）          */
/************************************************************************************/
#define SC8721_Fault_VINOVP_Msk 	 (1 << 7)  //输入电压超过芯片安全工作阈值
#define SC8721_STATU_VINREG_Msk    (1 << 2)  //芯片当前是否处于输入功率自适应模式
#define SC8721_STATU_BUSILIM_Msk   (1 << 1)  //芯片当前是否处于VBUS限流模式

#endif /* _SC8721_REG_ */
