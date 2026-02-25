/************************************************************************************/
/** \file PinDefs.h
/** \Author redstoner_35
/** \Project Xtern Ripper Laser Edition 
/** \Description 这个文件为底层的硬件定义文件，定义了整个工程内所有外设对应的硬件PIN和模
拟输入ADC通道的映射关系。

**	History: Initial Release
**	
/************************************************************************************/
#ifndef PINDEFS
#define PINDEFS
/************************************************************************************/
/* Include files */
/************************************************************************************/
#include "GPIOCfg.h"

/************************************************************************************
以下是系统的常规GPIO以及部分特殊的数字功能引脚，用于控制外部外设切换功能
************************************************************************************/
#define DCDCENIOP GPIO_PORT_3
#define DCDCENIOG 3
#define DCDCENIOx GPIO_PIN_0 //LD的DCDC电源使能引脚(P3.0)

#define LDMOSENIOP GPIO_PORT_1
#define LDMOSENIOG 1
#define LDMOSENIOx GPIO_PIN_4 //LD路径MOS的使能(P1.4)

#define DCDCSCLGPIOP GPIO_PORT_2
#define DCDCSCLGPIOG 2
#define DCDCSCLGPIOx GPIO_PIN_5		//DCDCSCL引脚(P2.5)

#define DCDCSDAGPIOP GPIO_PORT_2
#define DCDCSDAGPIOG 2
#define DCDCSDAGPIOx GPIO_PIN_6		//DCDCSDA引脚(P2.6)

/************************************************************************************
以下是系统的PWM输出引脚，用于对外输出PWM控制DCDC输出电压和电流等等
************************************************************************************/

#define PWMDACIOP GPIO_PORT_3
#define PWMDACIOG 3
#define PWMDACIOx GPIO_PIN_1		//恒流(CC)整定PWMDAC输出(P3.1)

#define PreChargeDACIOP GPIO_PORT_0
#define PreChargeDACIOG 0
#define PreChargeDACIOx GPIO_PIN_0		//恒压预充(CV)整定PWMDAC输出(P0.0)

/************************************************************************************
以下是系统的模拟输入引脚，例如电池电压测量等
************************************************************************************/
#define NTCInputIOG 0
#define NTCInputIOx GPIO_PIN_5 
#define NTCInputAIN 5						//NTC输入(P0.5,AN5)

#define VOUTFBIOG 2
#define VOUTFBIOx GPIO_PIN_2
#define VOUTFBAIN 8						//输出电压反馈引脚(P2.2,AN8)

#define VBATInputIOG 3
#define VBATInputIOx GPIO_PIN_2
#define VBATInputAIN 14					//电池电压检测引脚(P3.2,AN14)

/************************************************************************************
以下是系统的按键模块的GPIO引脚，负责驱动负责按键小板部分(包括指示灯和按键本身) 
************************************************************************************/
#define SideKeyGPIOP GPIO_PORT_1
#define SideKeyGPIOG 1
#define SideKeyGPIOx GPIO_PIN_3 	//侧按按键(P1.3)


#define RedLEDIOP GPIO_PORT_0
#define RedLEDIOG 0
#define RedLEDIOx GPIO_PIN_3		//红色指示灯(P0.3)	


#define GreenLEDIOP GPIO_PORT_0
#define GreenLEDIOG 0
#define GreenLEDIOx GPIO_PIN_1		//绿色指示灯(P0.1)

#endif /* PINDEFS */
