#ifndef _OCH_
#define _OCH_

//输出欠压参数配置
#define BoostChipUVLO 2.6f     //驱动内部boost芯片所能维持运行的最小UVLO电压(V)，低于此电压后系统强制关闭

//外部参考
extern xdata int Current; //电流值
extern xdata int CurrentBuf; //存储当前已经上传的电流值 

//DCDC状态声明
typedef enum
	{
	DCDC_Normal, //DCDC正常状态
	DCDC_OutputShort,  //输出短路过流打嗝
	DCDC_INTILIM,   //触发内部恒流保护限流
	DCDC_ThermalShutDown, //触发内部过温关闭
	DCDC_Warn_CBCOCP,     //触发芯片内部限流
	DCDC_StatuUnknown,   //DCDC状态未知
	}
DCDCStateDef;

//函数
void OutputChannel_Init(void);
void OutputChannel_DeInit(void);
void OutputChannel_Calc(void);
void OutputChannelFSM_TIMHandler(void);

//获取系统状态的函数
DCDCStateDef OutputChannel_GetDCDCState(void);
bit GetIfOutputEnabled(void);

#endif
