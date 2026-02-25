#ifndef _ModeControl_
#define _ModeControl_

#include "stdbool.h"
#include "FastOp.h"

typedef enum
	{
	LVPROT_Disable=0,  //该挡位关闭低电量保护
	LVPROT_Enable_Jump=1, //该挡位低电量保护开启，当电量低于阈值后执行跳档
	LVPROT_Enable_OFF=2		//该挡位低电量保护开启，当电量低于阈值后立即执行关机
	}LVProtectTypeDef;	
	
typedef struct
	{
	int RampCurrent;
	int RampBattThres;
	int RampCurrentLimit;
	unsigned char RampLimitReachDisplayTIM;
	unsigned char CfgSavedTIM;
	}SysConfigDef;	
	
typedef enum
	{
	Mode_OFF=0, //关机
	Mode_Fault=1, //出现错误
	//无极调光
	Mode_Ramp=2, //无极调光
	//正常阶梯挡位
	Mode_ExtremeLow=3, //极低亮度
	Mode_Low=4, //低亮
	Mode_Mid=5, //中亮
	Mode_MHigh=6,   //中高亮
	Mode_High=7,   //高亮
	//特殊挡位
	Mode_Turbo=8, //极亮
	Mode_SOS=9, //SOS
	Mode_Focus=10, //对焦专用档
	Mode_Burn=11, //烧东西专用档
	Mode_Breath=12, //呼吸模式
	Mode_Beacon=13,  //间歇闪模式
	Mode_SOS_NoProt=14 //无保护SOS模式
	}ModeIdxDef;
	

typedef struct
	{
  ModeIdxDef ModeIdx;
  int Current; //挡位电流(mA)
	int MinCurrent; //最小电流(mA)，仅无极调光需要
	int LowVoltThres; //低电压检测电压(mV)
	bool IsModeHasMemory; //是否带记忆
	bool IsNeedStepDown; //是否需要降档
	//是否允许进入极亮和爆闪
	bool IsEnterTurboStrobe; 
	//低电量保护设置
  ModeIdxDef ModeWhenLVAutoFall;		//低电量触发保护之后，如果不执行关机则自动跳转的挡位
	LVProtectTypeDef LVConfig;        //低电量保护机制的类型
	//挡位切换设置
  ModeIdxDef ModeTargetWhenH;
	ModeIdxDef ModeTargetWhen1H;	 //模式挡位切换设置，长按和单击+长按切换到的目标挡位
	}ModeStrDef; 

//外部引用
extern xdata unsigned char DisplayLockedTIM; //锁定提示计时器
extern ModeStrDef *CurrentMode; //当前模式结构体
extern xdata ModeIdxDef LastMode; //上一个挡位	
extern xdata SysConfigDef SysCfg; //无极调光配置	
extern bit IsSystemLocked;		//系统是否已锁定
extern bit IsEnableIdleLED;	//是否开启待机提示	
extern bit IsEnable2SMode;    //是否开启双锂模式
extern bit IsSystemEnteredAutoLocked; //系统是否已经进入自动锁定	
	
/************************************************
根据LD激光最高输出电流的自动定义，该函数决定系统的
最大功率限制，为了确保激光驱动正常工作，不得擅自
修改或调整！！！	
************************************************/	

//自动定义
#define TurboLDICCMAX 3000 //激光二极管的极限输出电流(mA)
	
//特殊宏定义
#define QueryCurrentGearILED() CurrentMode->Current //获取当前挡位的电流函数
#define ModeTotalDepth 15 //系统一共有几个挡位			
	
//函数
ModeStrDef *FindTargetMode(ModeIdxDef Mode,bool *IsResultOK);//输入指定的Index，从index里面找到目标模式结构体并返回指针
void ModeFSMTIMHandler(void);//挡位状态机所需的软件定时器处理
void ModeSwitchFSM();//挡位状态机
void SwitchToGear(ModeIdxDef TargetMode);//换到指定挡位
void ReturnToOFFState(void);//关机	
void HoldSwitchGearCmdHandler(void); //换挡间隔生成	
void ModeFSMInit(void); //初始化状态机	

#endif
