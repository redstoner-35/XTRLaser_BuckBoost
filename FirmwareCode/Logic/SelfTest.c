/****************************************************************************/
/** \file SelfTest.c
/** \Author redstoner_35
/** \Project Xtern Ripper Laser Edition 
/** \Description 这个文件是顶层应用层文件，负责实现驱动对输出的运行监控和错误汇
报系统，在系统出现故障时立即关闭并进入保护模式避免LD损毁。

**	History: Initial Release
**	
*****************************************************************************/
/****************************************************************************/
/*	include files
*****************************************************************************/
#include "LEDMgmt.h"
#include "delay.h"
#include "ADCCfg.h"
#include "BattDisplay.h"
#include "ModeControl.h"
#include "SelfTest.h"
#include "LowVoltProt.h"
#include "OutputChannel.h"

/****************************************************************************/
/*	External Function prototypes definition
****************************************************************************/
void LoadSleepTimer(void);

/****************************************************************************/
/*	Local pre-processor symbols/macros('#define')
****************************************************************************/

//其余的参数
#define FaultBlankingInterval 4 //系统开始运行的时候进行故障报告消隐的时间(每单位0.125S)

/****************************************************************************/
/*	Global variable definitions(declared in header file with 'extern')
****************************************************************************/
xdata FaultCodeDef ErrCode; //错误代码	

/****************************************************************************/
/*	Local constant definitions('static const')
****************************************************************************/

static code FaultCodeDef NonCriticalFault[]={ //非致命的错误代码
	Fault_DCDCOpen,
  Fault_DCDCShort, //开路和短路可能是误报，允许消除
  Fault_InputOVP,
	};

/****************************************************************************/
/*	Local variable  definitions('static')
****************************************************************************/
static xdata unsigned char ErrDisplayIndex; //错误显示计时
static xdata unsigned char ShortDetectTIM=0; //短路监测计时器
static xdata unsigned char ShortBlankTIM; //短路blank定时器

/****************************************************************************/
/*	Function implementation - local('static')
****************************************************************************/

static char ErrTIMCounter(char buf,char Count)	//内部函数，故障计数器
	{
	//累加计数器
	return buf<0x0F?buf+Count:0x0F;
	}

/****************************************************************************/
/*	Function implementation - global ('extern')
****************************************************************************/

bit IsErrorFatal(void)	
	{
	//查询错误是否致命
	unsigned char i;
	for(i=0;i<sizeof(NonCriticalFault);i++)
		if(NonCriticalFault[i]==ErrCode)return 0;
	//寻找了目前已有的错误码发现是致命问题
	return 1;
	}	
	
//报告错误
void ReportError(FaultCodeDef Code)
	{
	ErrCode=Code;
	if(CurrentMode->ModeIdx==Mode_Fault)return;
	SwitchToGear(Mode_Fault);  //指示故障发生
	LoadSleepTimer();
	}

//消除错误
void ClearError(void)
	{
	ErrCode=Fault_None;
	SwitchToGear(Mode_OFF);
	}

//错误ID显示计时函数	
void DisplayErrorTIMHandler(void)	
	{
	//没有错误发生，复位计时器
	if(ErrCode==Fault_None)ErrDisplayIndex=0;
	else //发生错误，开始计时
		{
		ErrDisplayIndex++;
    if(ErrDisplayIndex>=(15+(6*(int)ErrCode)))ErrDisplayIndex=0; //上限到了，开始翻转
		}
	}

//出现错误时显示DCDC的错误ID
void DisplayErrorIDHandler(void)
	{
	unsigned char buf;
	//先导提示红黄绿交替闪
  if(ErrDisplayIndex<5)
		{
		if(ErrDisplayIndex<3)LEDMode=(LEDStateDef)(ErrDisplayIndex+1);	
		else LEDMode=LED_OFF;
		}
	//闪烁指定次数显示Err ID
	else if(ErrDisplayIndex<(5+(6*(int)ErrCode)))
		{
		buf=(ErrDisplayIndex-5)/3; 
		if(!(buf%2))LEDMode=LED_Red;
		else LEDMode=LED_OFF;  //按照错误ID闪烁指定次数
		}
  else LEDMode=LED_OFF; //LED熄灭
	}

//MCUVDD故障监视
void MCUVDDFaultDetect(void)
	{
	//电池电压大于LDO Vdroop但是MCUVDD欠压，触发保护
	if(Data.RawBattVolt>3.2&&Data.MCUVDD<2.9)ReportError(Fault_MCUVDD_Error); 
	//MCUVDD过高，触发保护
	if(Data.MCUVDD>3.1)ReportError(Fault_MCUVDD_Error); 
	}
	
//输出故障检测
void OutputFaultDetect(void)
	{
	char buf,OErrID;
	//输出故障监测
	if(!GetIfOutputEnabled())ShortBlankTIM=0; //DCDC关闭
	else if(ShortBlankTIM<FaultBlankingInterval)ShortBlankTIM++; //时间未到不允许监测
	else  //开始检测
		{		
		buf=ShortDetectTIM&0x1F; //取出定时器值					
		//输入过压保护以及MCU电压监控
		MCUVDDFaultDetect();
		if(Data.BatteryVoltage>4.35)ReportError(Fault_InputOVP);	
		//根据DCDC状态进行监视
		else switch(OutputChannel_GetDCDCState())
			{
			case DCDC_ThermalShutDown:ReportError(Fault_DCDC_TSD);break;  //IC过温关闭
			case DCDC_INTILIM:
			case DCDC_OutputShort:  
				//输出短路过流打嗝以及片内恒流环触发
				buf=ErrTIMCounter(buf,4); 					 //计时器累计
				if(Data.OutputVoltage>1.0)OErrID=3;
				else OErrID=0;                       //输出大于1V，则报告非法负载否则报告LD错误
			  break;
			//DCDC状态未知,通信异常
			case DCDC_StatuUnknown:   
			  buf=ErrTIMCounter(buf,1); //计时器累计
				OErrID=2;			
			  break;
			//DCDC状态正常，执行开路检测
			case DCDC_Normal:
				//输出电压正常，复位计数器
				if(Data.OutputVoltage<5.9)buf=0;
				//输出电压异常，计时器继续累计然后触发保护
				else buf=ErrTIMCounter(buf,2); 
				OErrID=1; //该项错误ID=1
			  break;
		  //没有错误
		  default:buf=0;  //没有问题，计数器复位
			}
		//进行定时器数值的回写
		ShortDetectTIM=buf|(OErrID<<5);
		//状态检测
		if(buf<0x0F)return; //没有故障,跳过执行
		switch((ShortDetectTIM>>5)&0x07)	
			{
			case 0:ReportError(Fault_DCDCShort);break;
			case 1:ReportError(Fault_DCDCOpen);break;
			case 2:ReportError(Fault_DCDC_I2C_CommFault);break;
			case 3:ReportError(Fault_InvalidLoad);break;
			default:break;
			}
		}
	}
