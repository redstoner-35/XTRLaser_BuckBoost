#ifndef _SelfTest_H_
#define _SelfTest_H_

//错误类型枚举
typedef enum
	{
	Fault_None=0,    //没有错误发生
	Fault_NTCFailed=1, //NTC故障 ID:1
	Fault_OverHeat=2, //过热故障 ID:2	
	Fault_InputOVP=3, //输入过压保护 ID:3
	Fault_DCDC_I2C_CommFault=4,	//系统无法和DCDC的数字逻辑核进行通信 ID:4		
	Fault_DCDCVOUTTestError=5, //DCDC芯片输出电压参数测试失败 ID:5	
	Fault_InvalidLoad=6,       //驱动检测到连接的负载存在问题 ID:6
	Fault_DCDCShort=7, 				 //DCDC输出短路  ID:7	
	Fault_DCDCOpen=8,  				 //LED开路 ID:8	
	Fault_DCDC_TSD=9,          //系统检测到DCDC芯片过热关机 ID:9
	Fault_MCUVDD_Error=10,		 //系统检测到MCU供电出现异常 ID:10
	Fault_RampConfigError=11   //系统无法找到无极调光配置 ID:11		
	}FaultCodeDef;

//外部引用
extern xdata FaultCodeDef ErrCode; //错误代码
	
//函数
void ReportError(FaultCodeDef Code); //报告错误
void ClearError(void); //消除错误
void DisplayErrorTIMHandler(void); //显示错误时候用到的计时器处理
void DisplayErrorIDHandler(void); //根据错误ID进行显示的处理
void OutputFaultDetect(void); //输出故障监测函数	
void MCUVDDFaultDetect(void); //MCUVDD故障监视
bit IsErrorFatal(void);	//查询错误是否致命
	
#endif
