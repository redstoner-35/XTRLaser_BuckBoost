/****************************************************************************/
/** \file OutputChannel.c
/** \Author redstoner_35
/** \Project Xtern Ripper Laser Edition 
/** \Description 这个文件为中层设备驱动文件，负责根据上层逻辑层反馈的目标输出电流
值计算并操控PWMDAC输出指定的LD电流并完成LD的软起动保护功能。

**	History: Initial Release
**	
*****************************************************************************/
/****************************************************************************/
/*	include files
*****************************************************************************/
#include "cms8s6990.h"
#include "PinDefs.h"
#include "GPIO.h"
#include "PWMCfg.h"
#include "delay.h"
#include "OutputChannel.h"
#include "ModeControl.h"
#include "ADCCfg.h"
#include "SelfTest.h"
#include "TempControl.h"
#include "i2c.h"
#include "SC8721_REG.h"

/****************************************************************************/
/*	Local pre-processor symbols/macros('#define')
****************************************************************************/

//PWMDAC参数配置
#define CurrentOffset 99.0 //高电流通道下的电流偏差值(单位%)

//DCDC I2C配置
#define DCDCADDR 0x62   //DCDC芯片的7bit地址
#define DCDCSWEnReg SC8721_REG_GLOBALCTRL  //DCDC芯片软件使能的寄存器地址
#define DCDCSWEnPos SC8721_SWEnCmd_BitPos  //DCDC芯片软件使能的bit位置
#define DCDCSWEnPolar 0x00  //DCDC芯片软件使能的极性，1=高有效，0=低有效(对于SC8721A来说是低有效，填0)

//DCDC 初始化配置
#define DCDCInitStrDepth 8  //DCDC初始化步骤深度
#define DCDCPostCfgStrDepth 2 //DCDC后配置步骤深度

//启动状态机参数配置
#define DCDCInitialCurrent 35     //DCDC初始启动电流值(LSB=1mA) 
#define DCDCTestVoltOffset 1015     //DCDC输出测试的offset(LSB=0.1%)
#define DCDCTestVolt 285          //DCDC在启动阶段时设置的初始输出电压(LSB=0.1V)
#define DCDCStartUpMinVolt 2.70
#define DCDCStartUpMaxVolt 3.15   //驱动进行DCDC输出检查时的电压最大和最小允许值
#define WaitDACSettleTime 15      //启动时等待PWMDAC就绪的时间，一般15mS就OK

/****************************************************************************/
/*	Local pre-processor symbols/macros for Parameter processing('#define')
****************************************************************************/
#define _PreChargeDACCalc(x) ((39992000UL-(64706UL*x))/10000UL)
#define PreChargeDACCalc(x)  ((_PreChargeDACCalc(x)*DCDCTestVoltOffset)/1000UL)  //魔法算式，使用节点电流法计算并赋值PWMDAC参数

/****************************************************************************/
/*	Global variable definitions(declared in header file with 'extern')
****************************************************************************/
xdata int Current; //目标电流(mA)
xdata int CurrentBuf; //存储当前已经上传的电流值 

/****************************************************************************/
/*	Local type definitions('typedef')
****************************************************************************/
typedef enum
	{
	OCFSM_Idle,  //输出通道待机

  //软关机		
	OCFSM_WaitVoutDecay,     //等待输出电压下降
	OCFSM_GraceShutOFF,      //输出通道软关机流程（除能I2C并彻底关闭DCDC）
	
	//启动流程	
	OCFSM_PWMDACPreCharge,   //输出通道启动步骤1，开启CV/CC注入器的PWMDAC，并进行PWMDAC的预充电
	OCFSM_EnableDCDC,        //输出通道启动步骤2，使能DCDC芯片并初始化主控端I2C Host
	OCFSM_PushDCDCConfig,    //输出通道启动步骤3，通过软件使能暂时禁用DCDC之后下发DCDC的I2C配置（首次下发采用安全配置避免炸负载）

	OCFSM_WaitVOUTReady,     /**********************************************************************************************
		                       输出通道启动步骤4，重新使能DCDC开始输出并等待输出电压就绪。这步是为了确认DCDC芯片正常后再接通输出，
													 确保即使DCDC输出失控也不会炸掉负载。
													 **********************************************************************************************/	

	OCFSM_LoadDetect, 			 /**********************************************************************************************
	                         输出通道启动步骤5，此时系统接通负载后进行负载状态识别，确保用户连接的是正常LD才会下发全功率输出的
		                       DCDC配置避免炸掉用户的LD。		
													 **********************************************************************************************/

	OCFSM_PostDCDCConfig,    //输出通道启动步骤6，DCDC顺利启动之后且负载识别正常后，二次下发最终的DCDC配置，允许DCDC以全功率输出
		
	OCFSM_IncreaseVOUT,	     /**********************************************************************************************
													 输出通道启动步骤7，开始以受控斜率逐步减小CV注入器使得输出电压抬升，系统从CV缓慢过渡到CC阶段。这个阶
													 段下LD电流会由0V逐步抬升至初始值，实现0电流过冲
													 **********************************************************************************************/
	      
	OCFSM_RaiseCurrent,      //输出通道启动步骤8，系统开始执行输出恒流软起动，抬升电流值到挡位目标的电流参数
	//正常运行
	OCFSM_NormalOperation,   		 //输出通道正常运行中
	OCFSM_ReadyEnterIdleMode, 	 //输出通道尝试进入Idle状态
	OCFSM_IdleMode,              //输出通道待机
	OCFSM_BackToNormalOperation, //输出通道返回正常操作状态

	}OCFSMStateDef;

typedef struct
	{
	char RegAddr;    //寄存器地址
	char AndMask;
	char ORMask;     //对寄存器进行mask0和mask1的mask位（AND自带取反，对应位=1表示令寄存器对应位置0）
	}DCDCConfigDef;	
	
/****************************************************************************/
/*	Local constant definitions('static code')
****************************************************************************/	
static code DCDCConfigDef DCDCInitSeq[DCDCInitStrDepth]= //DCDC初始化的发包指令
	{
	//Step 1，写GLOBAL_CTRL寄存器禁止DCDC运行
		{
	  SC8721_REG_GLOBALCTRL,
		SC8721_GLOBALCTRL_CLR,
	  SC8721_Cmd_DisableDCDC
		},
	//Step 2，写CSO寄存器，把CSO设置为50(安全值，避免炸掉LD)
	  {
		SC8721_REG_CSOSET,
		SC8721_CSOSET_CLR,
		0x0B,    //令CSO[7:0]=11（0x0B）
		},
	//Step 3，写SLOPE_COMP寄存器，关闭系统的自带线损补偿功能
	  {
		SC8721_REG_SLOPECOMP,
		SC8721_SLOPECOMP_CLR,
		SC8721_SLOPECOMP_Disable    //关闭线损补偿
		},
  //Step 4，写VOUT_SET_MSB寄存器重置OFFSET=0
		{
		SC8721_REG_VOUTSETMSB,
		SC8721_VOUTSETMSB_CLR,
		SC8721_SET_VOUTMSB(0)
		},
	//Step 5，写VOUT_SET寄存器，配置芯片的输出电压控制模式
		{
		SC8721_REG_VOUTSETLSB,
		SC8721_VOUTSETLSB_CLR,
		//选择外部反馈，禁止内部寄存器修改输出
		SC8721_USING_ExtFB|SC8721_IntFB_ADJOFF|SC8721_IntFB_PosOFFSET|SC8721_SET_VOUTLSB(0)   
		},
	//Step 6，写SYS_SET寄存器设置FCCM/DCM、死区以及是否开启VINREG
		{
		SC8721_REG_SYSSET,
		SC8721_SYSSET_CLR,
		//激光驱动会经常低负载所以要开启FPWM，关闭输入自适应，死区用默认值就绪
		SC8721_CtrlMode_PFM|SC8721_DrvDT_20nS|SC8721_VINREG_Disable
		},
	//Step 7，写FREQ_SET寄存器，设置开关频率
	  {
		SC8721_REG_FREQSET,
		SC8721_FREQSET_CLR,
		//低输入输出电压，使用的500KHz频率
		SC8721_FSW_500KHz  
		},
	//Step 8，写GLOBAL_CTRL寄存器,设置Load bit应用所选配置
	  {
		SC8721_REG_GLOBALCTRL,
		0x00,
		SC8721_Cmd_ApplyDCDCSetting,  //令EN_LOAD=1，加载上面下发的init code的配置
		},
	};

static code DCDCConfigDef DCDCPostCfgSeq[DCDCPostCfgStrDepth]= //DCDC后配置的发包指令	
	{
	//Step 1，写CSO寄存器，把CSO设置为255(系统工作的正常值)
	  {
		SC8721_REG_CSOSET,
		SC8721_CSOSET_CLR,
		0xFF    //令CSO[7:0]=0xFF
		},	
	//Step 2，写GLOBAL_CTRL寄存器,设置Load bit应用所选配置
	  {
		SC8721_REG_GLOBALCTRL,
		0x00,
		SC8721_Cmd_ApplyDCDCSetting,  //令EN_LOAD=1，加载上面下发的init code的配置
		},
	};

/****************************************************************************/
/*	Local variable and SFR definitions('static and sfr')
****************************************************************************/
static bit IsSlowRamp;
static xdata unsigned char OCFSMTimer;	
static xdata unsigned char OCFSMCounter; //用于内部使用的计数变量
static OCFSMStateDef OCFSMState;         //输出通达状态机的状态
	
sbit LDMOSEN=LDMOSENIOP^LDMOSENIOx;      //LED路径MOS
sbit DCDCSDA=DCDCSDAGPIOP^DCDCSDAGPIOx;  //DCDC_SDA
sbit DCDCSCL=DCDCSCLGPIOP^DCDCSCLGPIOx;  //DCDC_SCL	
sbit DCDCEN=DCDCENIOP^DCDCENIOx; //DCDC使能Pin
/****************************************************************************/
/*	Function implementation - local('static')
****************************************************************************/
//发送软件使能和除能命令的指令
static bit OutputChannel_SentDCDCSwEnCmd(bit EN)	
	{
	char buf;
	//Bit Mask自动定义
	#define DCDCENBitMask (1 << DCDCSWEnPos)
	//读取数据
	if(I2C_ReadOneByte(DCDCADDR,&buf,DCDCSWEnReg))return 0;
	#if (DCDCSWEnPolar == 0)
		if(EN)buf&=(~DCDCENBitMask);
		else buf|=DCDCENBitMask;
	#else
 		if(!EN)buf&=(~DCDCENBitMask);
		else buf|=DCDCENBitMask;		
	#endif
	//进行mask完毕，写入数据	
	if(I2C_SendOneByte(DCDCADDR,buf,DCDCSWEnReg))return 0;
	//通信成功，返回1
	return 1;
	#undef DCDCENBitMask
	}

//内部函数，处理系统异常发生时延时等待的保护
static void OCFSMErrorHandler(FaultCodeDef Error)
	{
	//反复延时，如果延时后问题仍未解决，则报错
	if(OCFSMCounter)
		{
		delay_ms(1);
		OCFSMCounter--;
		}
	//延时时间到，报错处理
	else
		{
		//立即关断输出MOS避免负载损坏
		LDMOSEN=0;
		//报告错误并并进入安全关机的等待衰减状态
		ReportError(Error);
		OCFSMTimer=0;
		OCFSMState=OCFSM_WaitVoutDecay;		
		}
	}	

//内部函数，负责向DCDC下发配置
static bit PushDCDCCfg(DCDCConfigDef *Cfg)
	{
	char buf;
	//读取数据
	if(I2C_ReadOneByte(DCDCADDR,&buf,Cfg[OCFSMCounter].RegAddr))return 0;
	buf&=(~Cfg[OCFSMCounter].AndMask);
	buf|=Cfg[OCFSMCounter].ORMask;
	//数据处理完毕，回写
	if(I2C_SendOneByte(DCDCADDR,buf,Cfg[OCFSMCounter].RegAddr))return 0;
	//成功写入，返回1
	return 1;
	}	

//内部函数，根据MCUVDD计算预充DAC占空比并填写数值
static void SetPreChargeDAC(void)
	{
	#define PrechargeDACMagicNum (CVPWMDACFullScale*PreChargeDACCalc(DCDCTestVolt))
	unsigned long buf;
	//注入PWM=电压值*MCU供电/CVPWMDACFullScale
	buf=PrechargeDACMagicNum;
	buf/=(unsigned long)(Data.MCUVDD*1000);
	//填写算出的PWM值并赋值
  PreChargeDACDuty=buf&0xFFFF; //这里算出来大于允许值也没关系，因为PWM函数自己会钳位
  IsNeedToUploadPWM=1;
	//去除局部函数变量
	#undef PrechargeDACMagicNum
	}	
	
//内部用于计算PWMDAC占空比的函数
static float Duty_Calc(int CurrentInput)			
	{
	float buf;
	//根据电流值计算整定
  buf=((float)CurrentInput*(float)59)/(float)200;  //根据SC8721A Vcso对应输出电流的公式计算出Vcso两端的电压(mV)
	buf*=(float)CurrentOffset/(float)100; //乘以矫正系数修正电流
	buf*=(float)3;                        //因为硬件端PWMDAC的参数是正好1/3，所以乘以3得到最终电压
		
	//根据整定计算PWMDAC的占空比	
	buf/=Data.MCUVDD*(float)1000; //计算出目标DAC输出电压和PWMDAC缓冲器供电电压(MCUVDD)之间的比值
	buf*=100; //转换为百分比

	//结果输出（这里输出没有钳位也没关系，因为PWM函数会强制对输入整形，没关系的）
	return buf;
	}

//重置输出通道状态机的变量
static void OutputChannel_ResetSysVar(void)
	{
	//系统上电时电流配置为0
	Current=0;
	CurrentBuf=0;
	//初始化其余标志位
	OCFSMTimer=0;
	OCFSMCounter=0;
  OCFSMState=OCFSM_Idle;
	IsSlowRamp=0;	
	}	
	
//进行DCDC的I2C配置
static void OutputChannel_DCDCI2CCfg(bit IsEnable)
	{
	GPIOCfgDef OCInitCfg;
	//配置斜率
	OCInitCfg.Mode=IsEnable?GPIO_IPU:GPIO_Out_PP;
  OCInitCfg.Slew=GPIO_Fast_Slew;		
	OCInitCfg.DRVCurrent=GPIO_High_Current; 
	
	//配置GPIO(SCL)
	GPIO_SetMUXMode(DCDCSCLGPIOG,DCDCSCLGPIOx,IsEnable?GPIO_AF_SCL:GPIO_AF_GPIO); //配置为SCL
	GPIO_ConfigGPIOMode(DCDCSCLGPIOG,GPIOMask(DCDCSCLGPIOx),&OCInitCfg); 
	 
	//配置GPIO(SDA) 
	GPIO_SetMUXMode(DCDCSDAGPIOG,DCDCSDAGPIOx,IsEnable?GPIO_AF_SDA:GPIO_AF_GPIO); //配置为SDA
	GPIO_ConfigGPIOMode(DCDCSDAGPIOG,GPIOMask(DCDCSDAGPIOx),&OCInitCfg); 
	
	//配置I2C
	if(IsEnable)
		{
		I2C_EnableMasterMode(); //启用主控发送模式
		I2C_ConfigCLK(0x05); //Fsclk=Fsys/(2*10*(5+1))=400KHz 		
		}
	else I2C_DeInit();  //执行关闭指令
	}	
	
//监测DCDC状态
static void OutputChannel_DetectDCDCState(void)	
	{
	switch(OutputChannel_GetDCDCState())
		{
		//输出负载过重触发短路过流打嗝
		case DCDC_OutputShort:									 
			if(OCFSMTimer&0x08||Data.OutputVoltage>1.0)break;             //输出短路消隐（只有输出电压小于1V才视为短路）
			OCFSMErrorHandler(Fault_DCDCShort);
			break;
		case DCDC_Normal:		  
			//DCDC状态正常，监测输出电压是否在默认范围内，在的话就累加
			if(Data.OutputVoltage<DCDCStartUpMinVolt||Data.OutputVoltage>DCDCStartUpMaxVolt)
				{
				OCFSMCounter=50;
				if(!IsNeedToUploadPWM)SetPreChargeDAC();  //如果监测不通过则尝试更新PWMDAC占空比
				}
			else OCFSMCounter++; 
			delay_ms(1);
		  break;
		case DCDC_Warn_CBCOCP:
		case DCDC_INTILIM:
			//触发内部恒流保护限流，根据当前系统启动的状态决定报错结果
			if(OCFSMState==OCFSM_WaitVOUTReady)
				OCFSMErrorHandler(Fault_DCDCVOUTTestError);
		  else 
				{
				//开启负载的时候，如果出现这些保护为了避免炸负载，立即关断输出
				OCFSMCounter=0;
				OCFSMErrorHandler(Fault_InvalidLoad);
				}
		  break;						 																		 	 
		case DCDC_ThermalShutDown:
			OCFSMCounter=0;
			OCFSMErrorHandler(Fault_DCDC_TSD);
		  break; 						 																		 //触发DCDC芯片过温关机
		case DCDC_StatuUnknown:OCFSMErrorHandler(Fault_DCDC_I2C_CommFault);break; //通信异常
		}	
	}

/****************************************************************************/
/*	Function implementation - global ('extern')
****************************************************************************/

//获取DCDC状态并反馈给系统（需要自己实现，样例是SC8721的代码）
DCDCStateDef OutputChannel_GetDCDCState(void)
	{
	char buf;
	//读取STATUS1寄存器
	if(I2C_ReadOneByte(DCDCADDR,&buf,SC8721_REG_STATUS1))return DCDC_StatuUnknown;
	//判断是否触发hiccup和过温保护
	if(buf&SC8721_Fault_VOUTShort_Msk)return DCDC_OutputShort; //输出短路保护
	if(buf&SC8721_Fault_TSD_Msk)return DCDC_ThermalShutDown; //THD=1，IC过温保护
	if(buf&SC8721_Warning_CBCOCP_Msk)return DCDC_Warn_CBCOCP; //触发片内CBC OCP
	//读取0x0A寄存器
	if(I2C_ReadOneByte(DCDCADDR,&buf,SC8721_REG_STATUS2))return DCDC_StatuUnknown;
	if(buf&SC8721_STATU_BUSILIM_Msk)return DCDC_INTILIM;   //触发芯片内部限流
  //系统正常，返回正常结果
	return DCDC_Normal;
	}	

	
//获取输出是否开启
bit GetIfOutputEnabled(void)
	{
	//DCDC已经在正常输出了
	if(OCFSMState==OCFSM_RaiseCurrent)return 1;
	if(OCFSMState==OCFSM_NormalOperation)return 1;
	//其余情况返回0
	return 0;
	}
	
//输出通道状态机的计时器
void OutputChannelFSM_TIMHandler(void)
	{
	if(OCFSMTimer)OCFSMTimer--;
	}	
	
//输出通道复位
void OutputChannel_DeInit(void)
	{
	DCDCEN=0;	
	delay_ms(2);
	LDMOSEN=0;
  OutputChannel_DCDCI2CCfg(0);
	OutputChannel_ResetSysVar();
	}
	
//初始化函数
void OutputChannel_Init(void)
	{
	GPIOCfgDef OCInitCfg;
	//设置结构体
	OCInitCfg.Mode=GPIO_Out_PP;
  OCInitCfg.Slew=GPIO_Fast_Slew;		
	OCInitCfg.DRVCurrent=GPIO_High_Current; //推MOSFET,需要高上升斜率
	//初始化GPIO的sbit
	DCDCEN=0;	
	DCDCSDA=0;
	DCDCSCL=0;
	LDMOSEN=0;
	//配置GPIO(DCDCEN和路径MOS)
	GPIO_ConfigGPIOMode(LDMOSENIOG,GPIOMask(LDMOSENIOx),&OCInitCfg);
	GPIO_ConfigGPIOMode(DCDCENIOG,GPIOMask(DCDCENIOx),&OCInitCfg);
	//重置系统变量
  OutputChannel_DeInit();
	}
	
//输出通道计算
void OutputChannel_Calc(void)
	{
	int TargetCurrent;
	extern bit IsBurnMode;
	//读取目标电流并应用温控加权数据
	if(Current>0)
		{
		//取出温控限流数据
		TargetCurrent=ThermalILIMCalc();
		//如果目标电流小于当前挡位的温控限制值，则应用当前设置的电流值
		if(Current<TargetCurrent)TargetCurrent=Current;
		}
	//电流值为0或者-1，直接读取目标电流值
	else TargetCurrent=Current;
	//如果当前系统处于其余任意暂态，且输出电流=0表示需要系统关闭	
	if(OCFSMState>OCFSM_GraceShutOFF&&!TargetCurrent)
		{
		//输出通道状态机回到安全关闭阶段
		if(OCFSMState==OCFSM_IdleMode)LDMOSEN=1;                       //系统处于输出暂停阶段，安全关机需要放电
		OCFSMState=OCFSM_GraceShutOFF;
		}
	//正常执行状态机
	switch(OCFSMState)	
		{
		//执行软件关闭状态
		case OCFSM_GraceShutOFF:
			 //令CVDAC满摆幅，并通过软件发送关闭DCDC指令，强制关闭输出
			 PWMDuty=0;
		   PreChargeDACDuty=CVPWMDACFullScale;
			 IsNeedToUploadPWM=1;
		   OutputChannel_SentDCDCSwEnCmd(0);
			 //设置定时器确保该暂态不会卡死
		   OCFSMTimer=12;
	     //等待输出电压下降
		   OCFSMState=OCFSM_WaitVoutDecay;
			 break;
		//等待输出电压往下走
		case OCFSM_WaitVoutDecay:
			 //如果当前计时器没结束计时且输出电压还没降下来，则继续等待
			 if(OCFSMTimer&&Data.OutputVoltage>2.5)break;
		   //输出电压成功降下，首先复位I2C IP，延时2mS后关闭EN
		   OutputChannel_DCDCI2CCfg(0);
		   delay_ms(2);
		   DCDCEN=0;
		   //EN下电后经过1mS再断开负载，保护负载避免承受电流冲击
		   delay_ms(1);
	     LDMOSEN=0;	
		   //返回到初始状态
		   OCFSMState=OCFSM_Idle;
		   break;
		//输出通道处于待机状态
		case OCFSM_Idle:
			//复位系统变量
			OCFSMTimer=0;
			OCFSMCounter=0;
		  //输出电流大于0，进入启动流程，否则保持
		  if(TargetCurrent>0)OCFSMState=OCFSM_PWMDACPreCharge;
			break;
		//系统开始启动，启动步骤0，送出PWMDAC数据
	  case OCFSM_PWMDACPreCharge:
       //初始化PWMDAC参数
		   CurrentBuf=DCDCInitialCurrent;   //DCDC初始电流值
			 PWMDuty=Duty_Calc(CurrentBuf);  //根据目标输出的电流值设置参数
       SetPreChargeDAC();              //设置预充PWMDAC
			 //进行15mS的等待让PWMDAC就绪才能到下一步
		   OCFSMCounter=WaitDACSettleTime;
		   OCFSMState=OCFSM_EnableDCDC;
		   break;
	  //系统开始启动，等待20mS后使能EN并初始化I2C
		case OCFSM_EnableDCDC:
		   //系统仍在等待PWMDAC初始化
		   delay_ms(1);
		   if(IsNeedToUploadPWM||--OCFSMCounter)break;
		   //等待结束，使能EN，1mS后初始化I2C开始通信
		   DCDCEN=1;
		   delay_ms(1);
       OutputChannel_DCDCI2CCfg(1);
       OCFSMCounter=0;		
		   OCFSMTimer=8;                     //整个push过程最多等待1秒
		   OCFSMState=OCFSM_PushDCDCConfig;
		   break;
		//DCDC芯片已经启动，系统立即开始Push配置
		case OCFSM_PushDCDCConfig:
			 //倒计时结束仍未完成配置下发，系统故障，跳转到故障状态
		   if(!OCFSMTimer)OCFSMErrorHandler(Fault_DCDC_I2C_CommFault);
			 //下发配置流程
		   else if(OCFSMCounter==DCDCInitStrDepth)
					{
					//所有配置下发成功，尝试发送开启DCDC运行的指令然后等待输出就绪
					if(!OutputChannel_SentDCDCSwEnCmd(1))break;
					OCFSMTimer=4;                //电压上升最多等待0.5秒
					OCFSMCounter=50;  						//消隐监测
					OCFSMState=OCFSM_WaitVOUTReady;
					}
			 //推送任务还未完毕，继续发送指令
		   else if(PushDCDCCfg(&DCDCInitSeq[0]))OCFSMCounter++;
			 else delay_ms(1);  										//本次下发失败，延时1mS后再试
		   break;
	  //DCDC芯片等待输出就绪
		case OCFSM_WaitVOUTReady:		   
			 //系统倒计时结束，输出电压仍然没有达到规定范围，说明系统故障，报错
			 if(!OCFSMTimer)OCFSMErrorHandler(Fault_DCDCVOUTTestError);	
		   //系统正常运行时长达到足够时间，继续启动流程
		   else if(OCFSMCounter==65)
					{
					LDMOSEN=1;           //令路径管输出=1，接通负载	
					OCFSMState=OCFSM_LoadDetect; //进入负载识别
					OCFSMCounter=50;
					OCFSMTimer=4;        //执行后配置，后配置最多等待0.5秒
					}
			 //等待电压就绪	
		   else OutputChannel_DetectDCDCState();
			 break;
		
    //输出就绪后识别负载接的是不是正常LD					
		case OCFSM_LoadDetect:
			 //系统倒计时结束，仍然无法完成负载识别，说明系统故障，报错
			 if(!OCFSMTimer)OCFSMErrorHandler(Fault_InvalidLoad);	
		   //系统正常运行时长达到足够时间，继续启动流程
		   else if(OCFSMCounter==60)
					{
					OCFSMState=OCFSM_PostDCDCConfig;
					OCFSMCounter=0;
					OCFSMTimer=8;        //执行后配置，后配置最多等待1秒
					}
			 //等待电压就绪	
		   else OutputChannel_DetectDCDCState();			
       break;	
		//执行DCDC后配置
		case OCFSM_PostDCDCConfig:
			 //倒计时结束仍未完成配置下发，系统故障，跳转到故障状态
		   if(!OCFSMTimer)OCFSMErrorHandler(Fault_DCDC_I2C_CommFault);
			 //下发配置流程
		   else if(OCFSMCounter==DCDCPostCfgStrDepth)
					{
					//所有后配置下发成功，跳转到抬升输出电压的阶段开始往上升输出电压
					OCFSMState=OCFSM_IncreaseVOUT;
					}
			 //推送任务还未完毕，继续发送指令
		   else if(PushDCDCCfg(&DCDCPostCfgSeq[0]))OCFSMCounter++;
			 else delay_ms(1);  										//本次下发失败，延时1mS后再试
		   break;			 
		//开始抬升输出电压到目标值，LD正常工作
    case OCFSM_IncreaseVOUT:
		  //开始逐步下调预充占空比把输出电压调到额定值
		  if(!IsNeedToUploadPWM)
				{
				//预充PWMDAC输出=0，说明预充完成,此时跳转到正常输出状态
				if(!PreChargeDACDuty)OCFSMState=OCFSM_NormalOperation;	
				//继续进行调整，下调占空比
				else
					{
					//根据输出电流值计算下调斜率（斜率等于1+(额定电流/8)*1mA per Cycle）
					TargetCurrent=10+(TargetCurrent/2);
					if(TargetCurrent>250)TargetCurrent=250; 
					//根据指定的下调斜率值应用调整
					if(PreChargeDACDuty<TargetCurrent)PreChargeDACDuty=0;
					else PreChargeDACDuty-=TargetCurrent;	                 //PWMDAC在接近末尾的时候直接clear掉，否则进行逐次递减
					}					
				//标记占空比已更新，需要上传最新值	
				IsNeedToUploadPWM=1;
				}	
		  break;
		//开始抬升输出电流
		case OCFSM_RaiseCurrent:
			if(IsNeedToUploadPWM)break; //PWM正在应用中，等待
		  
			if((TargetCurrent-CurrentBuf)>600)IsSlowRamp=1; //监测到非常大的电流瞬态，避免冲爆灯珠采用软起
			if(IsSlowRamp)
				{
				//开始线性增加电流
				if(CurrentBuf==0)
					{
					//当前系统电流为0，判断传入的电流值并从低电流开始输出
					if(TargetCurrent<300)CurrentBuf=TargetCurrent;
					else CurrentBuf=300;
					}
			  //系统电流不为0，立即开始增加电流
			  else if(IsBurnMode)CurrentBuf+=50;    //烧灼模式开启，迅速增加电流提高烧灼效果
        else switch(CurrentMode->ModeIdx)
					{
					case Mode_Ramp:CurrentBuf+=2;break;      //无极调光使用2倍速度倍增
					case Mode_Beacon:CurrentBuf+=1000;break; //信标模式令电流快速增加
					case Mode_SOS:
					case Mode_SOS_NoProt:CurrentBuf+=400;break;  //SOS模式下快速增加电流避免拖尾影响判断
					default:CurrentBuf++;											 	 //其余挡位电流默认缓慢增加
					}
				if(CurrentBuf>=TargetCurrent)
					{
					IsSlowRamp=0;
					CurrentBuf=TargetCurrent; //限幅，不允许目标电流大于允许值
					}
				}
			else CurrentBuf=TargetCurrent; //直接同步				
			//更新占空比
			IsNeedToUploadPWM=1;
			PWMDuty=Duty_Calc(CurrentBuf);
			//占空比已同步，跳转到正常运行阶段
			if(TargetCurrent==CurrentBuf)OCFSMState=OCFSM_NormalOperation;	  
	    break;	
    //输出通道正常运行阶段
		case OCFSM_NormalOperation:
		  if(TargetCurrent==-1)
				{
				//系统电流配置为-1，说明需要暂停LED电流，跳转到暂停流程
				OCFSMState=OCFSM_ReadyEnterIdleMode;	
				OCFSMCounter=100;         //最多重试100次
				}
			if(TargetCurrent!=CurrentBuf)OCFSMState=OCFSM_RaiseCurrent; //占空比发生变更，开始进行处理
			break;
		//系统进入IDLE模式
		case OCFSM_ReadyEnterIdleMode:
			 //令预充DAC占空比立即拉满，关闭输出
			 SetPreChargeDAC();
		   //执行软件关闭DCDC命令，如果成功则进入睡眠状态
		   if(OutputChannel_SentDCDCSwEnCmd(0))OCFSMState=OCFSM_IdleMode;
		   //指令执行失败，重新尝试，如果尝试仍未成功100次则报错
			 else OCFSMErrorHandler(Fault_DCDC_I2C_CommFault);
		   break;
		//系统已经关闭DCDC，保持睡眠模式
	  case OCFSM_IdleMode:
			 //输出电压跌落至预充阈值以下，关闭路径MOS截断LD电流
		   if(!(Data.OutputVoltage>DCDCStartUpMaxVolt))LDMOSEN=0;
		   //还在睡大觉
			 if(TargetCurrent==-1)break;
		   //睡醒了，立即准备发送指令唤醒DCDC并接通路径MOS，让LD准备上电
		   LDMOSEN=1;                         
		   OCFSMCounter=100;         					//最多重试100次 
		   OCFSMState=OCFSM_BackToNormalOperation; //准备唤醒
		   break;
	  //开始唤醒流程
		case OCFSM_BackToNormalOperation:
			 //发送软件使能DCDC命令
			 if(OutputChannel_SentDCDCSwEnCmd(1))
				 {
				 //成功唤醒DCDC，等1mS让DCDC软起后，再重置PWMDAC立即开启输出
				 delay_ms(5); 
				 PreChargeDACDuty=0;
				 IsNeedToUploadPWM=1;
				 //跳转到正常执行阶段
				 OCFSMState=OCFSM_NormalOperation;
				 }
			 //指令执行失败，重新尝试，如果100次尝试仍未成功则报错
			 else OCFSMErrorHandler(Fault_DCDC_I2C_CommFault);
		   break;		   
		}
	}
